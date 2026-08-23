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

#include "desktop/lock_screen_sync.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <cstring>
#include <vector>

namespace umbra {

namespace {

// Reads surface's current back buffer into a tightly-packed RGBA buffer
// (mapped.RowPitch can exceed width*4 due to GPU row alignment padding,
// so this can't just memcpy the whole mapped region in one shot). Returns
// an empty vector on any failure.
std::vector<BYTE> captureBackBufferPixels(RenderSurface& surface, UINT* outWidth, UINT* outHeight) {
    ID3D11Device* device = surface.device();
    ID3D11DeviceContext* context = surface.context();
    ID3D11RenderTargetView* backBufferView = surface.backBufferView();
    if (device == nullptr || context == nullptr || backBufferView == nullptr) {
        return {};
    }

    Microsoft::WRL::ComPtr<ID3D11Resource> backBufferResource;
    backBufferView->GetResource(&backBufferResource);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(backBufferResource.As(&backBuffer))) {
        return {};
    }

    D3D11_TEXTURE2D_DESC desc{};
    backBuffer->GetDesc(&desc);

    // A CPU-readable copy — the back buffer itself is GPU-only (no
    // D3D11_CPU_ACCESS_READ), so it can't be Map()'d directly.
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&stagingDesc, nullptr, &staging))) {
        return {};
    }
    context->CopyResource(staging.Get(), backBuffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return {};
    }

    std::vector<BYTE> pixels(static_cast<size_t>(desc.Width) * desc.Height * 4);
    const auto* src = static_cast<const BYTE*>(mapped.pData);
    for (UINT row = 0; row < desc.Height; ++row) {
        std::memcpy(pixels.data() + static_cast<size_t>(row) * desc.Width * 4,
                    src + static_cast<size_t>(row) * mapped.RowPitch,
                    static_cast<size_t>(desc.Width) * 4);
    }
    context->Unmap(staging.Get(), 0);

    *outWidth = desc.Width;
    *outHeight = desc.Height;
    return pixels;
}

// render_surface.cpp's swap chain is DXGI_FORMAT_R8G8B8A8_UNORM — plain
// RGBA, not BGRA — so the WIC frame's pixel format is set to match
// directly rather than through a channel-swapping conversion step.
bool encodeRgbaPixelsToPng(const std::vector<BYTE>& pixels, UINT width, UINT height,
                           const std::filesystem::path& path) {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
        FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
    if (FAILED(encoder->CreateNewFrame(&frame, nullptr)) || FAILED(frame->Initialize(nullptr)) ||
        FAILED(frame->SetSize(width, height))) {
        return false;
    }

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGBA;
    if (FAILED(frame->SetPixelFormat(&format))) {
        return false;
    }

    const UINT stride = width * 4;
    if (FAILED(frame->WritePixels(height, stride, static_cast<UINT>(pixels.size()),
                                  const_cast<BYTE*>(pixels.data())))) {
        return false;
    }

    return SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
}

}  // namespace

LockScreenSync::LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath)
    : api_(api), snapshotPath_(std::move(snapshotPath)) {}

void LockScreenSync::syncFromSurface(RenderSurface& surface) const {
    UINT width = 0;
    UINT height = 0;
    const std::vector<BYTE> pixels = captureBackBufferPixels(surface, &width, &height);
    if (pixels.empty()) {
        return;
    }
    if (!encodeRgbaPixelsToPng(pixels, width, height, snapshotPath_)) {
        return;
    }
    api_.setLockScreenImage(snapshotPath_.wstring());
}

}  // namespace umbra
