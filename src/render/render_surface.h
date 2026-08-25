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

#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace umbra {

// Owns the Direct3D 11 device, swap chain, and back-buffer render target
// view for one monitor's render window (see ARCHITECTURE.md's Desktop
// Host / Render Surface split — one RenderSurface per active monitor).
// Windows-only; verified manually against a live desktop session, since a
// real D3D11 device/swap chain can't be meaningfully faked in a unit test.
class RenderSurface {
   public:
    RenderSurface(HWND window, int width, int height);

    RenderSurface(const RenderSurface&) = delete;
    RenderSurface& operator=(const RenderSurface&) = delete;

    // Recreates the swap chain's buffers for a new size (e.g. on monitor
    // resolution change). A no-op if the size hasn't actually changed. If
    // this throws (e.g. DXGI_ERROR_DEVICE_REMOVED), the surface is left
    // without a back-buffer view — backBufferView() returns nullptr until
    // a subsequent resize() succeeds — rather than attempting a recovery
    // that a lost D3D11 device may not support anyway.
    void resize(int width, int height);

    // Presents the back buffer to the screen. Returns false if the present
    // wasn't a real one — most commonly DXGI_STATUS_OCCLUDED, which Present()
    // reports (as a "success" HRESULT, not an error — SUCCEEDED() alone
    // can't tell the two apart) whenever a true exclusive-fullscreen app
    // (as opposed to a borderless one) has taken over the display: the draw
    // calls before this still ran, but there's no guarantee the back buffer
    // reflects them on screen, so callers that need to know a frame was
    // genuinely shown (e.g. before capturing it for the lock screen) should
    // treat a false return as "didn't actually present" rather than as
    // just "a frame was drawn".
    bool present();

    ID3D11Device* device() const { return device_.Get(); }
    ID3D11DeviceContext* context() const { return context_.Get(); }
    ID3D11RenderTargetView* backBufferView() const { return backBufferView_.Get(); }
    int width() const { return width_; }
    int height() const { return height_; }

   private:
    void createBackBufferView();

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferView_;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace umbra
