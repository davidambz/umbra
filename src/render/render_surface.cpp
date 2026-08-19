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

#include "render/render_surface.h"

#include <dxgi.h>

#include <stdexcept>

namespace umbra {

namespace {

Microsoft::WRL::ComPtr<IDXGISwapChain> createSwapChain(HWND window, int width, int height,
                                                       ID3D11Device* device) {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Width = static_cast<UINT>(width);
    desc.BufferDesc.Height = static_cast<UINT>(height);
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
        throw std::runtime_error("failed to query IDXGIDevice from D3D11 device");
    }
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter))) {
        throw std::runtime_error("failed to get IDXGIAdapter from IDXGIDevice");
    }
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    if (FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) {
        throw std::runtime_error("failed to get IDXGIFactory from IDXGIAdapter");
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(factory->CreateSwapChain(device, &desc, &swapChain))) {
        throw std::runtime_error("failed to create DXGI swap chain");
    }
    return swapChain;
}

}  // namespace

RenderSurface::RenderSurface(HWND window, int width, int height) : width_(width), height_(height) {
    UINT deviceFlags = 0;
#ifndef NDEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL selectedLevel{};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags,
                                 featureLevels, 1, D3D11_SDK_VERSION, &device_, &selectedLevel,
                                 &context_))) {
        throw std::runtime_error("failed to create Direct3D 11 device");
    }

    swapChain_ = createSwapChain(window, width, height, device_.Get());
    createBackBufferView();
}

void RenderSurface::createBackBufferView() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
        throw std::runtime_error("failed to get swap chain back buffer");
    }
    if (FAILED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &backBufferView_))) {
        throw std::runtime_error("failed to create back buffer render target view");
    }
}

void RenderSurface::resize(int width, int height) {
    if (width == width_ && height == height_) {
        return;
    }

    backBufferView_.Reset();
    if (FAILED(swapChain_->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height),
                                         DXGI_FORMAT_UNKNOWN, 0))) {
        throw std::runtime_error("failed to resize swap chain buffers");
    }
    width_ = width;
    height_ = height;
    createBackBufferView();
}

void RenderSurface::present() { swapChain_->Present(1, 0); }

}  // namespace umbra
