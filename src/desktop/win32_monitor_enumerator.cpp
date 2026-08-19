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

#include "desktop/win32_monitor_enumerator.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace umbra {

namespace {

std::string toUtf8(const wchar_t* wide) {
    const int required = ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(required) - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), required, nullptr, nullptr);
    return result;
}

BOOL CALLBACK collectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM userData) {
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(userData);

    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!::GetMonitorInfoW(monitor, &info)) {
        return TRUE;
    }

    MonitorInfo entry;
    entry.id = toUtf8(info.szDevice);
    entry.x = info.rcMonitor.left;
    entry.y = info.rcMonitor.top;
    entry.width = info.rcMonitor.right - info.rcMonitor.left;
    entry.height = info.rcMonitor.bottom - info.rcMonitor.top;
    entry.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    monitors->push_back(std::move(entry));

    return TRUE;
}

}  // namespace

std::vector<MonitorInfo> Win32MonitorEnumerator::enumerate() const {
    std::vector<MonitorInfo> monitors;
    ::EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

}  // namespace umbra
