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

#include "render/fit_rect.h"
#include "render/render_surface.h"

namespace umbra {

// Draws the current frame (a texture produced by a video/image/web engine,
// per ARCHITECTURE.md's engine/host split) into a RenderSurface's back
// buffer, positioned and scaled per fitMode via computeFitRect() — the
// engine's content resolution rarely matches the monitor's exactly.
// Windows-only; verified manually against a live desktop session (the
// underlying fit-rect math is unit-tested in fit_rect_test.cpp).
class Compositor {
   public:
    // surface must outlive this Compositor — it's stored by reference and
    // used by every draw() call, not just at construction.
    explicit Compositor(RenderSurface& surface, FitMode fitMode = FitMode::Contain);

    // Clears the back buffer to black, draws sourceView (of pixel size
    // sourceSize) scaled/positioned per fitMode, and presents. Clearing
    // first means letterbox/pillarbox bars are black rather than stale
    // content from a previous frame of a different aspect ratio. Returns
    // RenderSurface::present()'s own result (false without even attempting
    // a present if there's no back-buffer view to draw into at all) — see
    // its comment for why a caller capturing this frame afterwards should
    // care.
    bool draw(ID3D11ShaderResourceView* sourceView, Size sourceSize);

   private:
    void createPipeline();

    RenderSurface& surface_;
    FitMode fitMode_;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;
};

}  // namespace umbra
