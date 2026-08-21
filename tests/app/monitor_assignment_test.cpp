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

#include <gtest/gtest.h>

using umbra::assignProfilesToMonitors;
using umbra::indexOfMonitor;
using umbra::MonitorInfo;
using umbra::WallpaperProfile;
using umbra::WallpaperType;

TEST(AssignProfilesToMonitors, MatchesByIndexWithPrimaryFirst) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "secondary", .x = -1920, .y = 0, .width = 1920, .height = 1080},
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
    };
    WallpaperProfile profileForPrimary;
    profileForPrimary.monitorIndex = 0;
    profileForPrimary.path = "primary-wallpaper";
    WallpaperProfile profileForSecondary;
    profileForSecondary.monitorIndex = 1;
    profileForSecondary.path = "secondary-wallpaper";

    // Named (not a temporary) so it outlives `assignments`, per
    // assignProfilesToMonitors' documented contract that its profile
    // pointers reference into this vector's own storage.
    const std::vector<WallpaperProfile> profiles = {profileForPrimary, profileForSecondary};
    const auto assignments = assignProfilesToMonitors(monitors, profiles);

    ASSERT_EQ(assignments.size(), 2u);
    EXPECT_EQ(assignments[0].monitor.id, "primary");
    ASSERT_NE(assignments[0].profile, nullptr);
    EXPECT_EQ(assignments[0].profile->path, "primary-wallpaper");

    EXPECT_EQ(assignments[1].monitor.id, "secondary");
    ASSERT_NE(assignments[1].profile, nullptr);
    EXPECT_EQ(assignments[1].profile->path, "secondary-wallpaper");
}

TEST(AssignProfilesToMonitors, MonitorWithoutAMatchingProfileGetsNullptr) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "only", .x = 0, .y = 0, .width = 1920, .height = 1080},
    };

    const auto assignments = assignProfilesToMonitors(monitors, {});

    ASSERT_EQ(assignments.size(), 1u);
    EXPECT_EQ(assignments[0].profile, nullptr);
}

TEST(AssignProfilesToMonitors, ProfileTargetingADisconnectedMonitorIsSimplyUnused) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "only", .x = 0, .y = 0, .width = 1920, .height = 1080},
    };
    WallpaperProfile orphaned;
    orphaned.monitorIndex = 5;  // no monitor at index 5

    const auto assignments = assignProfilesToMonitors(monitors, {orphaned});

    ASSERT_EQ(assignments.size(), 1u);
    EXPECT_EQ(assignments[0].profile, nullptr);
}

TEST(AssignProfilesToMonitors, OrdersSecondaryMonitorsByAscendingXThenY) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "right", .x = 1920, .y = 0, .width = 1920, .height = 1080},
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
        MonitorInfo{.id = "left", .x = -1920, .y = 0, .width = 1920, .height = 1080},
    };

    const auto assignments = assignProfilesToMonitors(monitors, {});

    ASSERT_EQ(assignments.size(), 3u);
    EXPECT_EQ(assignments[0].monitor.id, "primary");
    EXPECT_EQ(assignments[1].monitor.id, "left");
    EXPECT_EQ(assignments[2].monitor.id, "right");
}

TEST(IndexOfMonitor, MatchesTheSameCanonicalOrderAsAssignProfilesToMonitors) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "right", .x = 1920, .y = 0, .width = 1920, .height = 1080},
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
        MonitorInfo{.id = "left", .x = -1920, .y = 0, .width = 1920, .height = 1080},
    };

    EXPECT_EQ(indexOfMonitor(monitors, "primary"), 0);
    EXPECT_EQ(indexOfMonitor(monitors, "left"), 1);
    EXPECT_EQ(indexOfMonitor(monitors, "right"), 2);
}

TEST(IndexOfMonitor, ReturnsNegativeOneWhenMonitorIdIsUnknown) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "only", .x = 0, .y = 0, .width = 1920, .height = 1080},
    };

    EXPECT_EQ(indexOfMonitor(monitors, "nonexistent"), -1);
}
