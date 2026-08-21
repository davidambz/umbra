// Umbra
// Copyright (C) 2026 David Ambrozio
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "engines/video_engine.h"

#include <mfapi.h>
#include <mferror.h>
#include <propvarutil.h>

#include <atomic>
#include <cstring>
#include <stdexcept>

namespace umbra {

namespace {

// Media Foundation's startup/shutdown is process-wide and ref-counted here
// so that having several VideoEngine instances alive at once (one per
// monitor, per ARCHITECTURE.md) doesn't tear it down out from under a
// sibling instance.
std::atomic<int> g_mfRefCount{0};

void acquireMediaFoundation() {
    if (g_mfRefCount.fetch_add(1) == 0) {
        if (FAILED(MFStartup(MF_VERSION))) {
            g_mfRefCount.store(0);
            throw std::runtime_error("failed to start Media Foundation");
        }
    }
}

void releaseMediaFoundation() {
    if (g_mfRefCount.fetch_sub(1) == 1) {
        MFShutdown();
    }
}

std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), wide.data(),
                        length);
    return wide;
}

}  // namespace

VideoEngine::VideoEngine(ID3D11Device* device, const std::string& path, int fpsCap)
    : device_(device), clock_(0.0, fpsCap) {
    device_->GetImmediateContext(&context_);
    acquireMediaFoundation();
    try {
        openSource(path, fpsCap);
        createTexture();
    } catch (...) {
        releaseMediaFoundation();
        throw;
    }
}

VideoEngine::~VideoEngine() { releaseMediaFoundation(); }

void VideoEngine::openSource(const std::string& path, int fpsCap) {
    const std::wstring widePath = toWide(path);

    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    if (FAILED(MFCreateAttributes(&attributes, 1)) ||
        FAILED(attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, FALSE)) ||
        FAILED(MFCreateSourceReaderFromURL(widePath.c_str(), attributes.Get(), &reader_))) {
        throw std::runtime_error("failed to open video file: " + path);
    }

    // Decode to RGB32 (byte order B,G,R,A — matches DXGI_FORMAT_B8G8R8A8_UNORM)
    // so the decoded buffer can be uploaded straight into a D3D11 texture
    // with no additional color conversion.
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (FAILED(MFCreateMediaType(&outputType)) ||
        FAILED(outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) ||
        FAILED(outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)) ||
        FAILED(reader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr,
                                            outputType.Get())) ||
        FAILED(reader_->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE))) {
        throw std::runtime_error("video file has no usable video stream: " + path);
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    UINT32 width = 0;
    UINT32 height = 0;
    if (FAILED(reader_->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType)) ||
        FAILED(MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &width, &height))) {
        throw std::runtime_error("failed to read video frame size: " + path);
    }
    frameSize_ = Size{static_cast<int>(width), static_cast<int>(height)};

    PROPVARIANT durationVar;
    PropVariantInit(&durationVar);
    double durationSeconds = 0.0;
    if (SUCCEEDED(reader_->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION,
                                                    &durationVar))) {
        UINT64 durationTicks = 0;
        if (SUCCEEDED(PropVariantToUInt64(durationVar, &durationTicks))) {
            // MF_PD_DURATION is in 100-nanosecond units.
            durationSeconds = static_cast<double>(durationTicks) / 10'000'000.0;
        }
    }
    PropVariantClear(&durationVar);

    clock_ = PlaybackClock(durationSeconds, fpsCap);
}

void VideoEngine::createTexture() {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(frameSize_.width);
    desc.Height = static_cast<UINT>(frameSize_.height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device_->CreateTexture2D(&desc, nullptr, &texture_)) ||
        FAILED(device_->CreateShaderResourceView(texture_.Get(), nullptr, &frameView_))) {
        throw std::runtime_error("failed to create video frame texture");
    }
}

void VideoEngine::advance(double deltaSeconds) {
    if (!clock_.advance(deltaSeconds)) {
        return;
    }
    decodeNextFrame();
}

void VideoEngine::seekToStart() {
    PROPVARIANT position;
    PropVariantInit(&position);
    InitPropVariantFromInt64(0, &position);
    reader_->SetCurrentPosition(GUID_NULL, position);
    PropVariantClear(&position);
}

void VideoEngine::decodeNextFrame() {
    DWORD flags = 0;
    Microsoft::WRL::ComPtr<IMFSample> sample;
    const HRESULT hr = reader_->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags,
                                           nullptr, &sample);
    if (FAILED(hr)) {
        return;
    }

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        seekToStart();
        return;
    }

    if (sample) {
        uploadFrame(sample.Get());
    }
}

void VideoEngine::uploadFrame(IMFSample* sample) {
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
        return;
    }

    BYTE* data = nullptr;
    DWORD dataLength = 0;
    if (FAILED(buffer->Lock(&data, nullptr, &dataLength))) {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context_->Map(texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        const UINT srcStride = static_cast<UINT>(frameSize_.width) * 4;
        auto* dst = static_cast<BYTE*>(mapped.pData);
        for (int row = 0; row < frameSize_.height; ++row) {
            std::memcpy(dst + static_cast<size_t>(row) * mapped.RowPitch,
                        data + static_cast<size_t>(row) * srcStride, srcStride);
        }
        context_->Unmap(texture_.Get(), 0);
    }

    buffer->Unlock();
}

}  // namespace umbra
