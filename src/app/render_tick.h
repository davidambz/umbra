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

namespace umbra {

// The outcome of gating one Application::onTick() pass for a single
// monitor's engine-driven wallpaper (video/image; WebEngine presents
// itself and never reaches this gate — see application.cpp's onTick()).
struct RenderTickGate {
    bool shouldRender = false;
    // Only meaningful when shouldRender is true: the real elapsed time to
    // advance playback by, which may span several skipped ticks under a
    // reduced fps cap — not just one fixed tick interval, or video/gif
    // playback runs in slow motion under throttling.
    double elapsedSeconds = 0.0;
};

// Accumulates tickIntervalSeconds into sinceLastRenderSeconds (in/out) and
// decides whether enough time has passed to render another frame under
// fpsCap (0 or negative means uncapped: render every tick). When
// rendering, the elapsed time consumed is subtracted via fmod rather than
// reset to 0, so pacing doesn't drift when a frame interval isn't an exact
// multiple of the tick interval.
//
// Pulled out of Application::onTick() (a Windows-only orchestrator that
// can't be unit-tested itself) so this scheduling/pacing math — previously
// inline and unverified except by manual testing — is covered by
// render_tick_test.cpp.
RenderTickGate evaluateRenderTickGate(double& sinceLastRenderSeconds, double tickIntervalSeconds,
                                      int fpsCap);

}  // namespace umbra
