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
// monitorIndex targets it, so the app orchestrator can create/destroy
// per-monitor render windows as monitors and profiles come and go.
//
// monitorIndex refers to a position in `monitors` sorted in a canonical
// order — primary monitor first, then by ascending virtual-desktop x/y —
// the same order settings-ui/ (#8) presents monitors in, since that's the
// only place a user picks a monitorIndex for a profile. This is a known
// simplification: unplugging a monitor before the primary shifts every
// later index, silently reassigning wallpapers. It works for the common
// case (monitors don't reorder while running) and is revisited once
// settings-ui/ can show monitor identity directly rather than an index.
//
// profiles must outlive the returned assignments — profile pointers
// reference into the input vector rather than copying WallpaperProfile.
std::vector<MonitorAssignment> assignProfilesToMonitors(
    const std::vector<MonitorInfo>& monitors, const std::vector<WallpaperProfile>& profiles);

// Sorts monitors into the same canonical order assignProfilesToMonitors()
// uses internally (primary first, then ascending x/y) — exposed so a
// caller that needs a monitor id's monitorIndex (e.g. ui_bridge.cpp
// handling an assign request) doesn't have to reimplement that ordering.
// Returns -1 if monitorId isn't in monitors.
int indexOfMonitor(const std::vector<MonitorInfo>& monitors, const std::string& monitorId);

// The canonical order itself (primary first, then ascending x/y) — a
// monitor's position in this list is its monitorIndex. Exposed for a
// caller that needs every monitor's index at once (e.g. ui_bridge.cpp
// mirroring a wallpaper assignment to every connected monitor): calling
// indexOfMonitor() once per monitor would re-sort the whole list from
// scratch on every call, which this avoids by sorting once up front.
std::vector<MonitorInfo> canonicalMonitorOrder(const std::vector<MonitorInfo>& monitors);

}  // namespace umbra
