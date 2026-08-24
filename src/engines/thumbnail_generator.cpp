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

#include "engines/thumbnail_generator.h"

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <vector>

#include "engines/wic_png_encoder.h"
#include "engines/win32_text.h"

namespace umbra {

namespace {

// The longer side a thumbnail is scaled down to (if the source is
// larger) — plenty for the small preview tiles MonitorCard/WallpaperCard
// render (settings-ui/src/components/), without bloating the base64
// data: URL ui_bridge.cpp embeds it as.
constexpr UINT kMaxThumbnailDimension = 480;

// Media Foundation's startup/shutdown is process-wide and ref-counted —
// mirrors video_engine.cpp's g_mfRefCount, kept separate rather than
// shared with it since the two run on different threads at different
// times (this only runs briefly, right after an import) and sharing a
// single counter across translation units would need its own plumbing
// for no real benefit.
std::atomic<int> g_mfRefCount{0};

bool acquireMediaFoundation() {
    if (g_mfRefCount.fetch_add(1) == 0) {
        if (FAILED(MFStartup(MF_VERSION))) {
            g_mfRefCount.store(0);
            return false;
        }
    }
    return true;
}

void releaseMediaFoundation() {
    if (g_mfRefCount.fetch_sub(1) == 1) {
        MFShutdown();
    }
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return value;
}

// video.<ext>/image.<ext> — the exact normalized name LibraryManager::
// import() writes (library_manager.cpp), found by stem rather than a
// fixed extension since the source extension is preserved as-is. Matches
// case-insensitively, same as LibraryManager::list()'s own lookup — a
// case-sensitive comparison here would silently diverge from which file
// list() itself considers "the" video/image for this title.
std::filesystem::path resolveContentFile(const std::filesystem::path& contentDir,
                                         WallpaperType type) {
    const std::string stem = type == WallpaperType::Video ? "video" : "image";
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(contentDir, ec)) {
        if (toLower(entry.path().stem().string()) == stem) {
            return entry.path();
        }
    }
    return {};
}

// Downscales a tightly-packed BGRA buffer to fit within
// kMaxThumbnailDimension (no-op if it already does), then encodes it.
// Shared by both the video and image paths below.
bool scaleAndEncodeBgra(const std::vector<BYTE>& pixels, UINT width, UINT height,
                        const std::filesystem::path& destination) {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return false;
    }

    if (width <= kMaxThumbnailDimension && height <= kMaxThumbnailDimension) {
        return encodeBgraPixelsToPngFile(pixels, width, height, destination);
    }

    Microsoft::WRL::ComPtr<IWICBitmap> source;
    const UINT stride = width * 4;
    if (FAILED(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, stride,
                                               static_cast<UINT>(pixels.size()),
                                               const_cast<BYTE*>(pixels.data()), &source))) {
        return false;
    }

    const double scale = static_cast<double>(kMaxThumbnailDimension) / std::max(width, height);
    const UINT targetWidth = std::max(1u, static_cast<UINT>(width * scale));
    const UINT targetHeight = std::max(1u, static_cast<UINT>(height * scale));

    Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
    if (FAILED(factory->CreateBitmapScaler(&scaler)) ||
        FAILED(scaler->Initialize(source.Get(), targetWidth, targetHeight,
                                  WICBitmapInterpolationModeFant))) {
        return false;
    }

    const UINT scaledStride = targetWidth * 4;
    std::vector<BYTE> scaledPixels(static_cast<size_t>(scaledStride) * targetHeight);
    if (FAILED(scaler->CopyPixels(nullptr, scaledStride, static_cast<UINT>(scaledPixels.size()),
                                  scaledPixels.data()))) {
        return false;
    }

    return encodeBgraPixelsToPngFile(scaledPixels, targetWidth, targetHeight, destination);
}

// Grabs the first decoded frame of the video at path as BGRA pixels.
// Mirrors video_engine.cpp's openSource()/decodeNextFrame(), stripped
// down to a single read — no playback clock, no looping, no D3D11
// texture (this writes straight to disk, never touches a GPU).
bool captureFirstVideoFrameAsBgra(const std::filesystem::path& path, std::vector<BYTE>* outPixels,
                                  UINT* outWidth, UINT* outHeight) {
    if (!acquireMediaFoundation()) {
        return false;
    }
    struct MfGuard {
        ~MfGuard() { releaseMediaFoundation(); }
    } mfGuard;

    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    if (FAILED(MFCreateAttributes(&attributes, 2)) ||
        FAILED(attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, FALSE)) ||
        FAILED(attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE)) ||
        FAILED(MFCreateSourceReaderFromURL(utf8ToWide(path.string()).c_str(), attributes.Get(),
                                           &reader))) {
        return false;
    }

    // RGB32 is byte order B,G,R,A — already BGRA, matching what
    // scaleAndEncodeBgra/encodeBgraPixelsToPngFile expect with no
    // further channel conversion (same choice video_engine.cpp makes,
    // for the same reason there: it maps directly onto a common GPU/WIC
    // format with nothing left to reorder).
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (FAILED(MFCreateMediaType(&outputType)) ||
        FAILED(outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) ||
        FAILED(outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)) ||
        FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr,
                                           outputType.Get())) ||
        FAILED(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    UINT32 width = 0;
    UINT32 height = 0;
    if (FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType)) ||
        FAILED(MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &width, &height)) ||
        width == 0 || height == 0) {
        return false;
    }

    DWORD flags = 0;
    Microsoft::WRL::ComPtr<IMFSample> sample;
    if (FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, nullptr,
                                  &sample)) ||
        sample == nullptr || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
        return false;  // corrupt/empty file, or a stream with zero frames
    }

    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
        return false;
    }

    BYTE* data = nullptr;
    DWORD dataLength = 0;
    if (FAILED(buffer->Lock(&data, nullptr, &dataLength))) {
        return false;
    }

    const UINT stride = width * 4;
    outPixels->assign(static_cast<size_t>(stride) * height, 0);
    const size_t available = std::min<size_t>(dataLength, outPixels->size());
    std::memcpy(outPixels->data(), data, available);
    buffer->Unlock();

    // RGB32's 4th byte is left undefined by Media Foundation's decoder for
    // opaque video (observed as 0, i.e. fully transparent). That alone
    // would already render as a blank thumbnail wherever it's displayed
    // (settings-ui renders thumbnailUrl as a plain <img>, which honors PNG
    // alpha) even through the direct, un-scaled encode path below — and
    // IWICBitmapScaler's WICBitmapInterpolationModeFant compounds it by
    // filtering in premultiplied-alpha space, so a stray 0 alpha zeroes
    // every channel of the *scaled* output too, not just the alpha
    // channel. Video content is always opaque, so forcing 255 here for
    // every decoded frame — not just the ones that end up scaled — is
    // correct, not a workaround for a real transparent source.
    for (size_t i = 3; i < outPixels->size(); i += 4) {
        (*outPixels)[i] = 255;
    }

    *outWidth = width;
    *outHeight = height;
    return true;
}

// Decodes frame 0 of the image at path as BGRA pixels — for an animated
// GIF/APNG this is just that first frame's own region, not the fully
// composited canvas ImageEngine builds for actual playback; close enough
// for a static preview, and far simpler.
bool decodeFirstImageFrameAsBgra(const std::filesystem::path& path, std::vector<BYTE>* outPixels,
                                 UINT* outWidth, UINT* outHeight) {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnDemand, &decoder))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom))) {
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
        return false;
    }

    const UINT stride = width * 4;
    outPixels->resize(static_cast<size_t>(stride) * height);
    if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(outPixels->size()),
                                     outPixels->data()))) {
        return false;
    }

    *outWidth = width;
    *outHeight = height;
    return true;
}

}  // namespace

void ThumbnailGenerator::generate(WallpaperType type, const std::filesystem::path& contentDir,
                                  const std::filesystem::path& destination) {
    if (type == WallpaperType::Web) {
        return;  // no single frame to grab — see the class comment
    }

    const std::filesystem::path contentFile = resolveContentFile(contentDir, type);
    if (contentFile.empty()) {
        return;
    }

    std::vector<BYTE> pixels;
    UINT width = 0;
    UINT height = 0;
    const bool decoded = type == WallpaperType::Video
                             ? captureFirstVideoFrameAsBgra(contentFile, &pixels, &width, &height)
                             : decodeFirstImageFrameAsBgra(contentFile, &pixels, &width, &height);
    if (!decoded) {
        return;
    }

    scaleAndEncodeBgra(pixels, width, height, destination);
}

}  // namespace umbra
