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

#include <gmock/gmock.h>

#include "desktop/monitor_manager.h"
#include "desktop/workerw_host.h"
#include "power/fullscreen_watcher.h"
#include "power/power_watcher.h"

namespace umbra::testing_support {

// Stands in for Win32MonitorEnumerator: returns whatever list was last
// handed to setMonitors(), so tests can drive MonitorManager::refresh()
// through a scripted sequence of monitor configurations.
class FakeMonitorEnumerator : public IMonitorEnumerator {
   public:
    void setMonitors(std::vector<MonitorInfo> monitors) { monitors_ = std::move(monitors); }

    std::vector<MonitorInfo> enumerate() const override { return monitors_; }

   private:
    std::vector<MonitorInfo> monitors_;
};

// Stands in for Win32WorkerWApi so WorkerWHost's attach sequencing can be
// verified without a live desktop session.
class MockWorkerWApi : public IWorkerWApi {
   public:
    MOCK_METHOD(WindowHandle, findWindowByClass, (const char* className), (const, override));
    MOCK_METHOD(void, sendSpawnWorkerWMessage, (WindowHandle progman), (const, override));
    MOCK_METHOD(WindowHandle, findBackgroundWorkerW, (), (const, override));
    MOCK_METHOD(bool, setParent, (WindowHandle child, WindowHandle parent), (const, override));
};

// Stands in for Win32PowerApi so PowerWatcher's refresh() can be verified
// without a live power subsystem.
class MockPowerApi : public IPowerApi {
   public:
    MOCK_METHOD(PowerState, queryState, (), (const, override));
};

// Stands in for Win32FullscreenApi so FullscreenWatcher's refresh() can be
// verified without a live desktop session.
class MockFullscreenApi : public IFullscreenApi {
   public:
    MOCK_METHOD(ForegroundWindowInfo, queryForegroundWindow, (), (const, override));
};

}  // namespace umbra::testing_support
