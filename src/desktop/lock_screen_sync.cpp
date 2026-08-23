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

#include <wrl/client.h>

#include <cstring>
#include <thread>
#include <vector>

#include "engines/wic_png_encoder.h"

namespace umbra {

namespace {

// Reads surface's current back buffer into a tightly-packed BGRA buffer,
// swapping the R/B channels as it copies — render_surface.cpp's swap chain
// is DXGI_FORMAT_R8G8B8A8_UNORM (RGBA), but WIC's PNG encoder only
// natively supports BGRA-family formats (see encodeBgraPixelsToPng's
// comment), so the conversion happens once here rather than trusting WIC
// to do it silently and correctly. mapped.RowPitch can also exceed
// width*4 due to GPU row alignment padding, so this can't just memcpy the
// whole mapped region in one shot either way. Returns an empty vector on
// any failure.
std::vector<BYTE> captureBackBufferAsBgra(RenderSurface& surface, UINT* outWidth, UINT* outHeight) {
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
        const BYTE* srcRow = src + static_cast<size_t>(row) * mapped.RowPitch;
        BYTE* dstRow = pixels.data() + static_cast<size_t>(row) * desc.Width * 4;
        for (UINT col = 0; col < desc.Width; ++col) {
            // RGBA -> BGRA: swap the R and B bytes, carry G/A as-is.
            dstRow[col * 4 + 0] = srcRow[col * 4 + 2];
            dstRow[col * 4 + 1] = srcRow[col * 4 + 1];
            dstRow[col * 4 + 2] = srcRow[col * 4 + 0];
            dstRow[col * 4 + 3] = srcRow[col * 4 + 3];
        }
    }
    context->Unmap(staging.Get(), 0);

    *outWidth = desc.Width;
    *outHeight = desc.Height;
    return pixels;
}

}  // namespace

LockScreenSync::LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath)
    : api_(api), snapshotPath_(std::move(snapshotPath)) {}

LockScreenSync::~LockScreenSync() {
    if (syncThread_.joinable()) {
        syncThread_.join();
    }
}

void LockScreenSync::syncFromSurface(RenderSurface& surface) {
    // Checked before touching the GPU at all: capturing a frame just to
    // discard it because a previous sync is still encoding/awaiting the
    // broker would pay the exact GPU stall this class exists to bound to
    // rare, one-per-rebuild occurrences, for a result nobody uses.
    bool expected = false;
    if (!syncInProgress_.compare_exchange_strong(expected, true)) {
        return;  // a previous sync's encode/broker call hasn't finished yet
    }

    UINT width = 0;
    UINT height = 0;
    std::vector<BYTE> pixels = captureBackBufferAsBgra(surface, &width, &height);
    if (pixels.empty()) {
        syncInProgress_.store(false);
        return;
    }

    // syncInProgress_ having let us past the compare_exchange above means
    // any previous syncThread_ has already finished its work — this join
    // just reclaims its OS thread handle (a prerequisite for the
    // reassignment below), not an extra wait.
    if (syncThread_.joinable()) {
        syncThread_.join();
    }

    // The D3D11 readback above has to run on the caller's thread (the
    // device context isn't safe to touch concurrently), but nothing past
    // this point does — WIC encoding is disk I/O, and both WinRT calls in
    // Win32LockScreenApi are already async under the hood. Moving them off
    // the render tick that called this keeps a lock screen sync from
    // stalling wallpaper playback for however long that takes. Joined by
    // the destructor rather than detached — this lambda reads api_ and
    // snapshotPath_ through `this`, which must still be alive when it runs.
    syncThread_ = std::thread([this, pixels = std::move(pixels), width, height]() mutable {
        // This thread has never touched COM — main.cpp's CoInitializeEx
        // only covers the thread that called it. WIC's CoCreateInstance
        // and Win32LockScreenApi's WinRT calls both need an apartment on
        // *this* thread specifically. Multithreaded (not the main thread's
        // apartment-threaded) since there's no window/message queue here
        // to keep single-threaded-affine, and blocking on .get() needs no
        // message pumping outside an STA anyway.
        const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        if (comInitialized) {
            if (encodeBgraPixelsToPngFile(pixels, width, height, snapshotPath_)) {
                api_.setLockScreenImage(snapshotPath_.wstring());
            }
            CoUninitialize();
        }
        syncInProgress_.store(false);
    });
}

}  // namespace umbra
