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

#include "engines/image_engine.h"

#include <propvarutil.h>

#include <stdexcept>
#include <vector>

namespace umbra {

namespace {

Microsoft::WRL::ComPtr<IWICImagingFactory> createWicFactory() {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        throw std::runtime_error("failed to create WIC imaging factory");
    }
    return factory;
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

// Reads a frame's display delay in milliseconds. GIF stores it in the
// graphic control extension (1/100s units); APNG stores it as a
// numerator/denominator pair in its frame control chunk. Falls back to
// FrameScheduler::kMinFrameDelayMs for a metadata reader that has neither
// (e.g. a single-frame still image, where the delay is never consulted).
int readFrameDelayMs(IWICBitmapFrameDecode* frame) {
    Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
    if (FAILED(frame->GetMetadataQueryReader(&reader))) {
        return FrameScheduler::kMinFrameDelayMs;
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(reader->GetMetadataByName(L"/grctlext/Delay", &value))) {
        UINT centiseconds = 0;
        const bool ok = SUCCEEDED(PropVariantToUInt32(value, &centiseconds));
        PropVariantClear(&value);
        if (ok) {
            return static_cast<int>(centiseconds) * 10;
        }
    } else {
        PropVariantClear(&value);
    }

    PropVariantInit(&value);
    UINT32 numerator = 0;
    bool haveNumerator = false;
    if (SUCCEEDED(reader->GetMetadataByName(L"/fctl/DelayNumerator", &value))) {
        haveNumerator = SUCCEEDED(PropVariantToUInt32(value, &numerator));
    }
    PropVariantClear(&value);

    PropVariantInit(&value);
    UINT32 denominator = 0;
    bool haveDenominator = false;
    if (SUCCEEDED(reader->GetMetadataByName(L"/fctl/DelayDenominator", &value))) {
        haveDenominator = SUCCEEDED(PropVariantToUInt32(value, &denominator));
    }
    PropVariantClear(&value);

    if (haveNumerator && haveDenominator && denominator != 0) {
        return static_cast<int>((static_cast<double>(numerator) / denominator) * 1000.0);
    }

    return FrameScheduler::kMinFrameDelayMs;
}

}  // namespace

ImageEngine::ImageEngine(ID3D11Device* device, const std::string& path) : device_(device) {
    decodeFrames(path);
}

void ImageEngine::decodeFrames(const std::string& path) {
    const auto factory = createWicFactory();
    const std::wstring widePath = toWide(path);

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnLoad, &decoder))) {
        throw std::runtime_error("failed to decode image: " + path);
    }

    UINT frameCount = 0;
    if (FAILED(decoder->GetFrameCount(&frameCount)) || frameCount == 0) {
        throw std::runtime_error("image has no frames: " + path);
    }

    std::vector<int> frameDelaysMs;
    frameDelaysMs.reserve(frameCount);
    frames_.reserve(frameCount);

    for (UINT i = 0; i < frameCount; ++i) {
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(i, &frame))) {
            throw std::runtime_error("failed to read image frame: " + path);
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(&converter)) ||
            FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom))) {
            throw std::runtime_error("failed to convert image frame to BGRA: " + path);
        }

        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height))) {
            throw std::runtime_error("failed to read image frame size: " + path);
        }
        if (i == 0) {
            frameSize_ = Size{static_cast<int>(width), static_cast<int>(height)};
        }

        const UINT stride = width * 4;
        std::vector<BYTE> pixels(static_cast<size_t>(stride) * height);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()),
                                         pixels.data()))) {
            throw std::runtime_error("failed to copy image frame pixels: " + path);
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = stride;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        if (FAILED(device_->CreateTexture2D(&desc, &initData, &texture)) ||
            FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr, &view))) {
            throw std::runtime_error("failed to create texture for image frame: " + path);
        }
        frames_.push_back(view);

        frameDelaysMs.push_back(frameCount > 1 ? readFrameDelayMs(frame.Get()) : 0);
    }

    scheduler_ = FrameScheduler(frameCount > 1 ? std::move(frameDelaysMs) : std::vector<int>{});
}

void ImageEngine::advance(double deltaSeconds) { scheduler_.advance(deltaSeconds); }

ID3D11ShaderResourceView* ImageEngine::currentFrame() const {
    return frames_[static_cast<size_t>(scheduler_.currentIndex())].Get();
}

}  // namespace umbra
