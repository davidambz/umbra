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

#include <string>

#include "render/fit_rect.h"

namespace umbra {

// What FullscreenWatcher needs to know about the current foreground
// window to decide whether it's a real fullscreen app (as opposed to,
// say, the desktop itself, the taskbar, or a minimized/cloaked window
// that happens to still report a full-monitor rect).
struct ForegroundWindowInfo {
    Rect windowRect;
    Rect monitorRect;
    std::string windowClassName;
    bool isMinimized = false;
    bool isCloaked = false;  // DWM-cloaked (e.g. a suspended UWP app) window.
};

// Window classes that can legitimately report a rect exactly matching a
// monitor without being a fullscreen app the user is actually looking at
// — the desktop itself, the WorkerW umbra renders into, and the shell's
// own top-level windows.
bool isShellWindowClass(const std::string& windowClassName);

// Pure heuristic: is the foreground window a real fullscreen app that
// should pause wallpaper rendering, per the PRD's "detect a foreground
// fullscreen app and automatically pause rendering" requirement. No Win32
// dependency, so unit-testable without a live desktop session — the
// actual OS queries live behind IFullscreenApi/Win32FullscreenApi.
bool isFullscreenForeground(const ForegroundWindowInfo& info);

// Abstracts the actual OS queries (GetForegroundWindow, GetWindowRect,
// MonitorFromWindow/GetMonitorInfo, GetClassName, IsIconic,
// DwmGetWindowAttribute(DWMWA_CLOAKED)) so FullscreenWatcher's
// poll-and-decide logic is unit-testable — see Win32FullscreenApi for the
// real implementation.
class IFullscreenApi {
   public:
    virtual ~IFullscreenApi() = default;
    virtual ForegroundWindowInfo queryForegroundWindow() const = 0;
};

// Polls IFullscreenApi (call refresh() periodically, per
// ARCHITECTURE.md's Power/Focus Watcher) and reports whether a real
// fullscreen app just became (or stopped being) the foreground window.
class FullscreenWatcher {
   public:
    // api must outlive this FullscreenWatcher.
    explicit FullscreenWatcher(const IFullscreenApi& api);

    // Re-queries the foreground window and re-decides fullscreen state.
    // Returns true if isFullscreenActive() changed since the last call.
    bool refresh();

    bool isFullscreenActive() const { return isFullscreenActive_; }

   private:
    const IFullscreenApi& api_;
    bool isFullscreenActive_ = false;
};

}  // namespace umbra
