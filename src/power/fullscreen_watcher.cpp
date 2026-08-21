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

#include "power/fullscreen_watcher.h"

#include <algorithm>
#include <array>

namespace umbra {

bool isShellWindowClass(const std::string& windowClassName) {
    // "WorkerW" and "Progman" are the desktop/background windows Umbra
    // itself attaches into (see desktop/workerw_host.*) — without this
    // exclusion, Umbra's own render window covering a monitor would be
    // mistaken for a fullscreen app pausing itself. "Shell_TrayWnd" and
    // "Shell_SecondaryTrayWnd" are the taskbar on the primary/secondary
    // monitors.
    static constexpr std::array<const char*, 4> kShellClasses = {
        "Progman",
        "WorkerW",
        "Shell_TrayWnd",
        "Shell_SecondaryTrayWnd",
    };
    return std::any_of(
        kShellClasses.begin(), kShellClasses.end(),
        [&windowClassName](const char* shellClass) { return windowClassName == shellClass; });
}

bool isFullscreenForeground(const ForegroundWindowInfo& info) {
    if (info.isMinimized || info.isCloaked) {
        return false;
    }
    if (isShellWindowClass(info.windowClassName)) {
        return false;
    }
    if (info.monitorRect.width <= 0 || info.monitorRect.height <= 0) {
        return false;
    }
    // The standard "borderless fullscreen" heuristic used by presentation-
    // mode detectors: the foreground window's rect matches its monitor's
    // rect exactly, with no border/titlebar visible.
    return info.windowRect == info.monitorRect;
}

FullscreenWatcher::FullscreenWatcher(const IFullscreenApi& api) : api_(api) {}

bool FullscreenWatcher::refresh() {
    const bool isFullscreen = isFullscreenForeground(api_.queryForegroundWindow());
    const bool changed = isFullscreen != isFullscreenActive_;
    isFullscreenActive_ = isFullscreen;
    return changed;
}

}  // namespace umbra
