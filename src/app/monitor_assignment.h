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
#include <vector>

#include "config/wallpaper_profile.h"
#include "desktop/monitor_manager.h"

namespace umbra {

// Which WallpaperProfile (if any) is currently assigned to one connected
// monitor.
struct MonitorAssignment {
    MonitorInfo monitor;
    const WallpaperProfile* profile = nullptr;  // nullptr if this monitor has no wallpaper.
};

// Matches each currently connected monitor to the WallpaperProfile whose
// monitorId targets it (WallpaperProfile::monitorId, a stable device
// identifier — see MonitorInfo::id), so the app orchestrator can
// create/destroy per-monitor render windows as monitors and profiles come
// and go. Because the match is by stable id rather than a recomputed
// position, unplugging/replugging/reordering monitors doesn't shift which
// profile applies to which monitor.
//
// profiles must outlive the returned assignments — profile pointers
// reference into the input vector rather than copying WallpaperProfile.
std::vector<MonitorAssignment> assignProfilesToMonitors(
    const std::vector<MonitorInfo>& monitors, const std::vector<WallpaperProfile>& profiles);

// Sorts monitors into a canonical display order — primary first, then by
// ascending virtual-desktop x/y — purely for presentation (settings-ui/'s
// "Display N" labeling and getMonitors' response order in ui_bridge.cpp).
// This ordering is not used for assignment matching, so it recomputing
// freely on every hot-plug has no effect on existing assignments.
std::vector<MonitorInfo> canonicalMonitorOrder(const std::vector<MonitorInfo>& monitors);

}  // namespace umbra
