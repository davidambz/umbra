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

#include "render/fit_rect.h"

namespace umbra {

// Common surface implemented by VideoEngine, ImageEngine, and WebEngine so
// the caller (per ARCHITECTURE.md, the app/ orchestrator's per-monitor
// render loop, and Compositor::draw()) can drive whichever engine a
// monitor's WallpaperProfile::type selects without knowing which one it
// is. Windows-only, like RenderSurface/Compositor: a real decoder/browser
// can't be meaningfully faked in a unit test, so each implementation is
// verified manually (see TESTING.md) while the pure timing/scheduling
// logic they lean on (PlaybackClock, FrameScheduler) is unit-tested on its
// own.
class IWallpaperEngine {
   public:
    virtual ~IWallpaperEngine() = default;

    // Advances playback/animation state by deltaSeconds. May decode a new
    // frame or, for WebEngine, pump the browser's render loop.
    virtual void advance(double deltaSeconds) = 0;

    // The texture to draw this frame, for Compositor::draw(). May be
    // nullptr before the first frame is ready (e.g. mid-load).
    virtual ID3D11ShaderResourceView* currentFrame() const = 0;

    // Pixel size of currentFrame(), for computeFitRect(). All-zero if
    // currentFrame() is nullptr.
    virtual Size frameSize() const = 0;
};

}  // namespace umbra
