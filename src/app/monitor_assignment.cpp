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

#include "app/monitor_assignment.h"

#include <algorithm>

namespace umbra {

namespace {

std::vector<MonitorInfo> canonicalOrder(const std::vector<MonitorInfo>& monitors) {
    std::vector<MonitorInfo> ordered = monitors;
    std::sort(ordered.begin(), ordered.end(), [](const MonitorInfo& a, const MonitorInfo& b) {
        if (a.isPrimary != b.isPrimary) {
            return a.isPrimary;  // primary sorts first
        }
        if (a.x != b.x) {
            return a.x < b.x;
        }
        return a.y < b.y;
    });
    return ordered;
}

}  // namespace

std::vector<MonitorAssignment> assignProfilesToMonitors(
    const std::vector<MonitorInfo>& monitors, const std::vector<WallpaperProfile>& profiles) {
    const std::vector<MonitorInfo> ordered = canonicalOrder(monitors);

    std::vector<MonitorAssignment> assignments;
    assignments.reserve(ordered.size());
    for (int index = 0; index < static_cast<int>(ordered.size()); ++index) {
        MonitorAssignment assignment;
        assignment.monitor = ordered[static_cast<size_t>(index)];
        // If more than one profile targets the same monitorIndex (e.g. a
        // hand-edited settings.json, or monitors renumbering after one is
        // unplugged before settings-ui/ (#8) catches up), the first one
        // found wins and the rest are silently ignored for this monitor —
        // not surfaced as an error since there's no user-facing channel
        // for it here.
        for (const WallpaperProfile& profile : profiles) {
            if (profile.monitorIndex == index) {
                assignment.profile = &profile;
                break;
            }
        }
        assignments.push_back(assignment);
    }
    return assignments;
}

int indexOfMonitor(const std::vector<MonitorInfo>& monitors, const std::string& monitorId) {
    const std::vector<MonitorInfo> ordered = canonicalOrder(monitors);
    for (int index = 0; index < static_cast<int>(ordered.size()); ++index) {
        if (ordered[static_cast<size_t>(index)].id == monitorId) {
            return index;
        }
    }
    return -1;
}

}  // namespace umbra
