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

#include "power/fullscreen_watcher.h"

namespace umbra {

// Real IFullscreenApi backed by GetForegroundWindow/GetWindowRect,
// MonitorFromWindow/GetMonitorInfo, GetClassName, IsIconic, and
// DwmGetWindowAttribute(DWMWA_CLOAKED). Windows-only, verified manually
// against a live desktop session (see TESTING.md) — the fullscreen
// heuristic itself (isFullscreenForeground) is unit-tested on its own.
class Win32FullscreenApi : public IFullscreenApi {
   public:
    ForegroundWindowInfo queryForegroundWindow() const override;
};

}  // namespace umbra
