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

#include "power/win32_fullscreen_api.h"

#include <dwmapi.h>
#include <windows.h>

namespace umbra {

namespace {

Rect toRect(const RECT& rect) {
    return Rect{rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
}

std::string classNameOf(HWND window) {
    char buffer[256];
    const int length = GetClassNameA(window, buffer, sizeof(buffer));
    return length > 0 ? std::string(buffer, static_cast<size_t>(length)) : std::string();
}

}  // namespace

ForegroundWindowInfo Win32FullscreenApi::queryForegroundWindow() const {
    ForegroundWindowInfo info;

    const HWND foreground = GetForegroundWindow();
    if (foreground == nullptr) {
        return info;
    }

    info.windowClassName = classNameOf(foreground);
    info.isMinimized = IsIconic(foreground) != FALSE;

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(foreground, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        info.isCloaked = cloaked != FALSE;
    }

    RECT windowRect{};
    if (GetWindowRect(foreground, &windowRect)) {
        info.windowRect = toRect(windowRect);
    }

    const HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (monitor != nullptr && GetMonitorInfo(monitor, &monitorInfo)) {
        info.monitorRect = toRect(monitorInfo.rcMonitor);
    }

    return info;
}

}  // namespace umbra
