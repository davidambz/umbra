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

#include "power/power_state.h"

namespace umbra {

// What every monitor's render loop should do right now, combining
// FullscreenWatcher and PowerWatcher's independent signals into one
// decision per the PRD's "detect a foreground fullscreen app and
// automatically pause rendering" / "reduce fps or pause entirely on
// battery saver" requirements.
struct RenderPolicy {
    bool paused = false;
    int fpsCap = 60;
};

// Pure combination logic — no Win32 dependency, so unit-testable without a
// live desktop/power session. fullscreenActive and powerAction each come
// from their own watcher; pauseOnFullscreen is the user's
// Settings::pauseOnFullscreen toggle (a fullscreen app never pauses
// rendering if the user disabled that). profileFpsCap is the wallpaper's
// own configured cap (WallpaperProfile::fpsCap); the effective cap is
// whichever of that and powerAction's reduced cap is lower, never higher.
RenderPolicy computeRenderPolicy(bool fullscreenActive, bool pauseOnFullscreen,
                                 ThrottleAction powerAction, int profileFpsCap, int reducedFpsCap);

}  // namespace umbra
