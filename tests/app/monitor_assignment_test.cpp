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
using umbra::canonicalMonitorOrder;
using umbra::MonitorInfo;
using umbra::WallpaperProfile;
using umbra::WallpaperType;

TEST(AssignProfilesToMonitors, MatchesByStableMonitorIdWithPrimaryFirstInOutputOrder) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "secondary", .x = -1920, .y = 0, .width = 1920, .height = 1080},
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
    };
    WallpaperProfile profileForPrimary;
    profileForPrimary.monitorId = "primary";
    profileForPrimary.path = "primary-wallpaper";
    WallpaperProfile profileForSecondary;
    profileForSecondary.monitorId = "secondary";
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

TEST(AssignProfilesToMonitors, AssignmentSurvivesMonitorsReorderingBetweenCalls) {
    // Regression test for #87: a profile keyed by stable monitor id must
    // keep pointing at the same physical monitor even if a hotplug shifts
    // canonical ordering (e.g. the previously-primary monitor is now
    // unplugged, so what was "secondary" becomes canonical index 0).
    WallpaperProfile profileForSecondary;
    profileForSecondary.monitorId = "secondary";
    profileForSecondary.path = "secondary-wallpaper";
    const std::vector<WallpaperProfile> profiles = {profileForSecondary};

    // Secondary was canonical index 1 while primary was still connected.
    const std::vector<MonitorInfo> beforeUnplug = {
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
        MonitorInfo{.id = "secondary", .x = 1920, .y = 0, .width = 1920, .height = 1080},
    };
    const auto assignmentsBefore = assignProfilesToMonitors(beforeUnplug, profiles);
    ASSERT_EQ(assignmentsBefore.size(), 2u);
    EXPECT_EQ(assignmentsBefore[1].monitor.id, "secondary");
    ASSERT_NE(assignmentsBefore[1].profile, nullptr);

    // Primary is unplugged: "secondary" is now the only monitor and sorts
    // to canonical index 0 — an index-keyed lookup would now see it as
    // unassigned instead of matching the profile that targets it.
    const std::vector<MonitorInfo> afterUnplug = {
        MonitorInfo{.id = "secondary", .x = 0, .y = 0, .width = 1920, .height = 1080},
    };
    const auto assignmentsAfter = assignProfilesToMonitors(afterUnplug, profiles);
    ASSERT_EQ(assignmentsAfter.size(), 1u);
    EXPECT_EQ(assignmentsAfter[0].monitor.id, "secondary");
    ASSERT_NE(assignmentsAfter[0].profile, nullptr);
    EXPECT_EQ(assignmentsAfter[0].profile->path, "secondary-wallpaper");
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
    orphaned.monitorId = "disconnected";  // no monitor with this id

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

TEST(CanonicalMonitorOrder, SortsPrimaryFirstThenAscendingXThenY) {
    const std::vector<MonitorInfo> monitors = {
        MonitorInfo{.id = "right", .x = 1920, .y = 0, .width = 1920, .height = 1080},
        MonitorInfo{
            .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
        MonitorInfo{.id = "left", .x = -1920, .y = 0, .width = 1920, .height = 1080},
    };

    const auto ordered = canonicalMonitorOrder(monitors);

    ASSERT_EQ(ordered.size(), 3u);
    EXPECT_EQ(ordered[0].id, "primary");
    EXPECT_EQ(ordered[1].id, "left");
    EXPECT_EQ(ordered[2].id, "right");
}
