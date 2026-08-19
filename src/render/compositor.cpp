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

#include "render/compositor.h"

#include <d3dcompiler.h>

#include <cstring>
#include <stdexcept>
#include <string>

namespace umbra {

namespace {

struct Vertex {
    float x, y;  // NDC position
    float u, v;  // texture coordinate
};

// A fullscreen quad (two triangles) in NDC space. Compositor::draw() maps
// this onto the fit rect computed by computeFitRect() via the D3D11
// viewport, rather than varying the vertex positions per frame.
constexpr Vertex kQuadVertices[] = {
    {-1.0f, 1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f},  {-1.0f, -1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},  {1.0f, -1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 1.0f},
};

constexpr char kVertexShaderSource[] = R"(
struct VsOut {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VsOut main(float2 position : POSITION, float2 texCoord : TEXCOORD0) {
    VsOut result;
    result.position = float4(position, 0.0, 1.0);
    result.texCoord = texCoord;
    return result;
}
)";

constexpr char kPixelShaderSource[] = R"(
Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET {
    return sourceTexture.Sample(sourceSampler, texCoord);
}
)";

Microsoft::WRL::ComPtr<ID3DBlob> compileShader(const char* source, const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main",
                                  target, 0, 0, &blob, &errors);
    if (FAILED(hr)) {
        const std::string message = errors ? static_cast<const char*>(errors->GetBufferPointer())
                                           : "unknown shader compile error";
        throw std::runtime_error("failed to compile " + std::string(target) +
                                 " shader: " + message);
    }
    return blob;
}

}  // namespace

Compositor::Compositor(RenderSurface& surface, FitMode fitMode)
    : surface_(surface), fitMode_(fitMode) {
    createPipeline();
}

void Compositor::createPipeline() {
    ID3D11Device* device = surface_.device();

    const auto vsBlob = compileShader(kVertexShaderSource, "vs_5_0");
    const auto psBlob = compileShader(kPixelShaderSource, "ps_5_0");

    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                          nullptr, &vertexShader_)) ||
        FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                         nullptr, &pixelShader_))) {
        throw std::runtime_error("failed to create compositor shaders");
    }

    const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(device->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(),
                                         vsBlob->GetBufferSize(), &inputLayout_))) {
        throw std::runtime_error("failed to create compositor input layout");
    }

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(kQuadVertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = kQuadVertices;
    if (FAILED(device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer_))) {
        throw std::runtime_error("failed to create compositor vertex buffer");
    }

    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    if (FAILED(device->CreateSamplerState(&samplerDesc, &samplerState_))) {
        throw std::runtime_error("failed to create compositor sampler state");
    }
}

void Compositor::draw(ID3D11ShaderResourceView* sourceView, Size sourceSize) {
    ID3D11RenderTargetView* rtv = surface_.backBufferView();
    if (rtv == nullptr) {
        // A prior RenderSurface::resize() failed, leaving no back-buffer
        // view to draw into — nothing to do until it succeeds.
        return;
    }

    ID3D11DeviceContext* context = surface_.context();
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    context->ClearRenderTargetView(rtv, clearColor);

    const Rect fitRect =
        computeFitRect(sourceSize, Size{surface_.width(), surface_.height()}, fitMode_);
    if (fitRect.width <= 0 || fitRect.height <= 0) {
        surface_.present();
        return;
    }

    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = static_cast<float>(fitRect.x);
    viewport.TopLeftY = static_cast<float>(fitRect.y);
    viewport.Width = static_cast<float>(fitRect.width);
    viewport.Height = static_cast<float>(fitRect.height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(inputLayout_.Get());
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ID3D11Buffer* vertexBuffer = vertexBuffer_.Get();
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, &sourceView);
    ID3D11SamplerState* sampler = samplerState_.Get();
    context->PSSetSamplers(0, 1, &sampler);

    context->Draw(6, 0);

    surface_.present();
}

}  // namespace umbra
