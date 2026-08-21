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

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "engines/win32_text.h"

namespace umbra {

namespace {

enum class Disposal { None, Background, Previous };
enum class Blend { Source, Over };

struct FrameMeta {
    int left = 0;
    int top = 0;
    int delayMs = FrameScheduler::kMinFrameDelayMs;
    Disposal disposal = Disposal::None;
    Blend blend = Blend::Over;
};

Microsoft::WRL::ComPtr<IWICImagingFactory> createWicFactory() {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        throw std::runtime_error("failed to create WIC imaging factory");
    }
    return factory;
}

bool readUInt32(IWICMetadataQueryReader* reader, const wchar_t* name, UINT32* outValue) {
    PROPVARIANT value;
    PropVariantInit(&value);
    const bool ok = SUCCEEDED(reader->GetMetadataByName(name, &value)) &&
                    SUCCEEDED(PropVariantToUInt32(value, outValue));
    PropVariantClear(&value);
    return ok;
}

// GIF's graphic control extension "Disposal" and APNG's frame control
// chunk "DisposeOp" use different numeric encodings for the same three
// concepts (leave as-is / clear to background / restore to what was there
// before this frame), so both are normalized to a single Disposal here.
Disposal gifDisposalFromValue(UINT32 value) {
    switch (value) {
        case 2:
            return Disposal::Background;
        case 3:
            return Disposal::Previous;
        default:
            return Disposal::None;
    }
}

Disposal apngDisposeOpFromValue(UINT32 value) {
    switch (value) {
        case 1:
            return Disposal::Background;
        case 2:
            return Disposal::Previous;
        default:
            return Disposal::None;
    }
}

// Reads the per-frame metadata WIC exposes for animated GIF/APNG: display
// offset within the canvas, delay, disposal, and (APNG only) blend mode.
// GIF has no separate blend mode — its decoded pixels already carry
// alpha=0 for the transparent-color-index pixels, so treating it as an
// "over" blend against the existing canvas is exactly the GIF spec's
// behavior. A frame with neither /grctlext nor /fctl metadata (a plain
// still image) gets the defaults: (0,0) offset, None disposal, Over
// blend — which is also correct for it.
FrameMeta readFrameMeta(IWICBitmapFrameDecode* frame) {
    FrameMeta meta;

    Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
    if (FAILED(frame->GetMetadataQueryReader(&reader))) {
        return meta;
    }

    UINT32 value = 0;
    if (readUInt32(reader.Get(), L"/imgdesc/Left", &value)) {
        meta.left = static_cast<int>(value);
    }
    if (readUInt32(reader.Get(), L"/imgdesc/Top", &value)) {
        meta.top = static_cast<int>(value);
    }
    if (readUInt32(reader.Get(), L"/grctlext/Delay", &value)) {
        meta.delayMs = static_cast<int>(value) * 10;  // GIF delay is in 1/100s units.
    }
    if (readUInt32(reader.Get(), L"/grctlext/Disposal", &value)) {
        meta.disposal = gifDisposalFromValue(value);
    }

    UINT32 xOffset = 0;
    UINT32 yOffset = 0;
    const bool isApngFrame = readUInt32(reader.Get(), L"/fctl/XOffset", &xOffset) &&
                             readUInt32(reader.Get(), L"/fctl/YOffset", &yOffset);
    if (isApngFrame) {
        meta.left = static_cast<int>(xOffset);
        meta.top = static_cast<int>(yOffset);

        UINT32 numerator = 0;
        UINT32 denominator = 0;
        if (readUInt32(reader.Get(), L"/fctl/DelayNumerator", &numerator) &&
            readUInt32(reader.Get(), L"/fctl/DelayDenominator", &denominator) && denominator != 0) {
            meta.delayMs =
                static_cast<int>((static_cast<double>(numerator) / denominator) * 1000.0);
        }
        if (readUInt32(reader.Get(), L"/fctl/DisposeOp", &value)) {
            meta.disposal = apngDisposeOpFromValue(value);
        }
        if (readUInt32(reader.Get(), L"/fctl/BlendOp", &value)) {
            meta.blend = value == 0 ? Blend::Source : Blend::Over;
        }
    }

    return meta;
}

// The full logical canvas a multi-frame image's individual frames (which
// are often just the changed sub-rectangle, not a full redraw) are
// composited onto. GIF exposes this via the global logical screen
// descriptor; when that's unavailable (APNG, or a decoder that doesn't
// expose it), frame 0's own size is used — correct for APNG, whose first
// frame is conventionally the full canvas.
Size readCanvasSize(IWICBitmapDecoder* decoder, Size fallback) {
    Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
    if (FAILED(decoder->GetMetadataQueryReader(&reader))) {
        return fallback;
    }
    UINT32 width = 0;
    UINT32 height = 0;
    if (readUInt32(reader.Get(), L"/logscrdesc/Width", &width) &&
        readUInt32(reader.Get(), L"/logscrdesc/Height", &height) && width > 0 && height > 0) {
        return Size{static_cast<int>(width), static_cast<int>(height)};
    }
    return fallback;
}

// Clamps a frame's own rectangle to fit within the canvas, in case a
// malformed file claims an offset/size that would run off the edge.
Rect clampToCanvas(int left, int top, int width, int height, Size canvas) {
    const int x = std::clamp(left, 0, canvas.width);
    const int y = std::clamp(top, 0, canvas.height);
    const int w = std::clamp(width, 0, canvas.width - x);
    const int h = std::clamp(height, 0, canvas.height - y);
    return Rect{x, y, w, h};
}

std::vector<BYTE> copyCanvasRegion(const std::vector<BYTE>& canvas, int canvasWidth,
                                   const Rect& region) {
    std::vector<BYTE> out(static_cast<size_t>(region.width) * region.height * 4);
    for (int row = 0; row < region.height; ++row) {
        const size_t srcOffset = (static_cast<size_t>(region.y + row) * canvasWidth + region.x) * 4;
        const size_t dstOffset = static_cast<size_t>(row) * region.width * 4;
        std::memcpy(out.data() + dstOffset, canvas.data() + srcOffset,
                    static_cast<size_t>(region.width) * 4);
    }
    return out;
}

void writeCanvasRegion(std::vector<BYTE>& canvas, int canvasWidth, const Rect& region,
                       const BYTE* pixels) {
    for (int row = 0; row < region.height; ++row) {
        const size_t dstOffset = (static_cast<size_t>(region.y + row) * canvasWidth + region.x) * 4;
        const size_t srcOffset = static_cast<size_t>(row) * region.width * 4;
        std::memcpy(canvas.data() + dstOffset, pixels + srcOffset,
                    static_cast<size_t>(region.width) * 4);
    }
}

void clearCanvasRegion(std::vector<BYTE>& canvas, int canvasWidth, const Rect& region) {
    for (int row = 0; row < region.height; ++row) {
        const size_t offset = (static_cast<size_t>(region.y + row) * canvasWidth + region.x) * 4;
        std::memset(canvas.data() + offset, 0, static_cast<size_t>(region.width) * 4);
    }
}

// Composites srcPixels (region.width x region.height, BGRA) onto canvas at
// region's offset. Blend::Source overwrites outright (used by APNG frames
// that explicitly opt out of alpha blending); Blend::Over does standard
// straight-alpha "over" compositing, which is also exactly right for
// GIF's decoded pixels (alpha=0 on the transparent-color-index pixels
// lets the existing canvas content show through unchanged).
void compositeOnto(std::vector<BYTE>& canvas, int canvasWidth, const Rect& region,
                   const BYTE* srcPixels, Blend blend) {
    if (blend == Blend::Source) {
        writeCanvasRegion(canvas, canvasWidth, region, srcPixels);
        return;
    }

    for (int row = 0; row < region.height; ++row) {
        BYTE* dstRow =
            canvas.data() + (static_cast<size_t>(region.y + row) * canvasWidth + region.x) * 4;
        const BYTE* srcRow = srcPixels + static_cast<size_t>(row) * region.width * 4;
        for (int col = 0; col < region.width; ++col) {
            BYTE* dst = dstRow + static_cast<size_t>(col) * 4;
            const BYTE* src = srcRow + static_cast<size_t>(col) * 4;
            const float srcA = src[3] / 255.0f;
            if (srcA >= 1.0f) {
                std::memcpy(dst, src, 4);
                continue;
            }
            if (srcA <= 0.0f) {
                continue;
            }
            const float dstA = dst[3] / 255.0f;
            const float outA = srcA + dstA * (1.0f - srcA);
            for (int channel = 0; channel < 3; ++channel) {
                const float blended = src[channel] * srcA + dst[channel] * dstA * (1.0f - srcA);
                dst[channel] = static_cast<BYTE>(outA > 0.0f ? blended / outA : 0.0f);
            }
            dst[3] = static_cast<BYTE>(outA * 255.0f);
        }
    }
}

}  // namespace

ImageEngine::ImageEngine(ID3D11Device* device, const std::string& path) : device_(device) {
    decodeFrames(path);
}

void ImageEngine::decodeFrames(const std::string& path) {
    const auto factory = createWicFactory();
    const std::wstring widePath = utf8ToWide(path);

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnLoad, &decoder))) {
        throw std::runtime_error("failed to decode image: " + path);
    }

    UINT frameCount = 0;
    if (FAILED(decoder->GetFrameCount(&frameCount)) || frameCount == 0) {
        throw std::runtime_error("image has no frames: " + path);
    }

    // Frame 0's size is decoded up front purely to seed the canvas-size
    // fallback below (readCanvasSize) — the frame itself is re-fetched in
    // the loop, since IWICBitmapFrameDecode instances aren't meant to
    // outlive one pass over the decoder.
    Size frame0Size;
    {
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame0;
        UINT width = 0, height = 0;
        if (FAILED(decoder->GetFrame(0, &frame0)) || FAILED(frame0->GetSize(&width, &height))) {
            throw std::runtime_error("failed to read image frame size: " + path);
        }
        frame0Size = Size{static_cast<int>(width), static_cast<int>(height)};
    }
    frameSize_ = readCanvasSize(decoder.Get(), frame0Size);

    std::vector<int> frameDelaysMs;
    frameDelaysMs.reserve(frameCount);
    frames_.reserve(frameCount);

    std::vector<BYTE> canvas(static_cast<size_t>(frameSize_.width) * frameSize_.height * 4, 0);

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

        UINT frameWidth = 0;
        UINT frameHeight = 0;
        if (FAILED(converter->GetSize(&frameWidth, &frameHeight))) {
            throw std::runtime_error("failed to read image frame size: " + path);
        }

        const UINT stride = frameWidth * 4;
        std::vector<BYTE> framePixels(static_cast<size_t>(stride) * frameHeight);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(framePixels.size()),
                                         framePixels.data()))) {
            throw std::runtime_error("failed to copy image frame pixels: " + path);
        }

        const FrameMeta meta = readFrameMeta(frame.Get());
        const Rect region = clampToCanvas(meta.left, meta.top, static_cast<int>(frameWidth),
                                          static_cast<int>(frameHeight), frameSize_);

        std::vector<BYTE> previousRegionSnapshot;
        if (meta.disposal == Disposal::Previous) {
            previousRegionSnapshot = copyCanvasRegion(canvas, frameSize_.width, region);
        }

        if (region.width > 0 && region.height > 0) {
            compositeOnto(canvas, frameSize_.width, region, framePixels.data(), meta.blend);
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(frameSize_.width);
        desc.Height = static_cast<UINT>(frameSize_.height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = canvas.data();
        initData.SysMemPitch = static_cast<UINT>(frameSize_.width) * 4;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        if (FAILED(device_->CreateTexture2D(&desc, &initData, &texture)) ||
            FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr, &view))) {
            throw std::runtime_error("failed to create texture for image frame: " + path);
        }
        frames_.push_back(view);
        frameDelaysMs.push_back(meta.delayMs);

        if (region.width > 0 && region.height > 0) {
            if (meta.disposal == Disposal::Background) {
                clearCanvasRegion(canvas, frameSize_.width, region);
            } else if (meta.disposal == Disposal::Previous) {
                writeCanvasRegion(canvas, frameSize_.width, region, previousRegionSnapshot.data());
            }
        }
    }

    scheduler_ = FrameScheduler(frameCount > 1 ? std::move(frameDelaysMs) : std::vector<int>{});
}

void ImageEngine::advance(double deltaSeconds) { scheduler_.advance(deltaSeconds); }

ID3D11ShaderResourceView* ImageEngine::currentFrame() const {
    return frames_[static_cast<size_t>(scheduler_.currentIndex())].Get();
}

}  // namespace umbra
