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

std::vector<MonitorAssignment> assignProfilesToMonitors(
    const std::vector<MonitorInfo>& monitors, const std::vector<WallpaperProfile>& profiles) {
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

    std::vector<MonitorAssignment> assignments;
    assignments.reserve(ordered.size());
    for (int index = 0; index < static_cast<int>(ordered.size()); ++index) {
        MonitorAssignment assignment;
        assignment.monitor = ordered[static_cast<size_t>(index)];
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

}  // namespace umbra
