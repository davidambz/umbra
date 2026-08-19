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

#include "desktop/monitor_manager.h"

#include <gtest/gtest.h>

#include "mocks/win32_mocks.h"

using umbra::MonitorChangeSet;
using umbra::MonitorInfo;
using umbra::MonitorManager;
using umbra::testing_support::FakeMonitorEnumerator;

namespace {

MonitorInfo makeMonitor(std::string id, int x, int width, bool isPrimary = false) {
    MonitorInfo monitor;
    monitor.id = std::move(id);
    monitor.x = x;
    monitor.y = 0;
    monitor.width = width;
    monitor.height = 1080;
    monitor.isPrimary = isPrimary;
    return monitor;
}

}  // namespace

TEST(MonitorManager, FirstRefreshReportsEveryMonitorAsAdded) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true)});
    MonitorManager manager(enumerator);

    const MonitorChangeSet changes = manager.refresh();

    ASSERT_EQ(changes.added.size(), 1u);
    EXPECT_EQ(changes.added[0].id, "\\\\.\\DISPLAY1");
    EXPECT_TRUE(changes.removed.empty());
    EXPECT_TRUE(changes.changed.empty());
    EXPECT_EQ(manager.monitors().size(), 1u);
}

TEST(MonitorManager, NoChangeBetweenIdenticalRefreshes) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true)});
    MonitorManager manager(enumerator);
    manager.refresh();

    const MonitorChangeSet changes = manager.refresh();

    EXPECT_TRUE(changes.isEmpty());
}

TEST(MonitorManager, DetectsNewlyConnectedMonitor) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true)});
    MonitorManager manager(enumerator);
    manager.refresh();

    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true),
                            makeMonitor("\\\\.\\DISPLAY2", 1920, 1080)});
    const MonitorChangeSet changes = manager.refresh();

    ASSERT_EQ(changes.added.size(), 1u);
    EXPECT_EQ(changes.added[0].id, "\\\\.\\DISPLAY2");
    EXPECT_TRUE(changes.removed.empty());
    EXPECT_EQ(manager.monitors().size(), 2u);
}

TEST(MonitorManager, DetectsDisconnectedMonitor) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true),
                            makeMonitor("\\\\.\\DISPLAY2", 1920, 1080)});
    MonitorManager manager(enumerator);
    manager.refresh();

    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true)});
    const MonitorChangeSet changes = manager.refresh();

    ASSERT_EQ(changes.removed.size(), 1u);
    EXPECT_EQ(changes.removed[0].id, "\\\\.\\DISPLAY2");
    EXPECT_EQ(manager.monitors().size(), 1u);
}

TEST(MonitorManager, DetectsResolutionChangeOnExistingMonitor) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true)});
    MonitorManager manager(enumerator);
    manager.refresh();

    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 2560, true)});
    const MonitorChangeSet changes = manager.refresh();

    ASSERT_EQ(changes.changed.size(), 1u);
    EXPECT_EQ(changes.changed[0].width, 2560);
    EXPECT_TRUE(changes.added.empty());
    EXPECT_TRUE(changes.removed.empty());
}

TEST(MonitorManager, HandlesGoingFromMultipleMonitorsToNone) {
    FakeMonitorEnumerator enumerator;
    enumerator.setMonitors({makeMonitor("\\\\.\\DISPLAY1", 0, 1920, true),
                            makeMonitor("\\\\.\\DISPLAY2", 1920, 1080)});
    MonitorManager manager(enumerator);
    manager.refresh();

    enumerator.setMonitors({});
    const MonitorChangeSet changes = manager.refresh();

    EXPECT_EQ(changes.removed.size(), 2u);
    EXPECT_TRUE(manager.monitors().empty());
}
