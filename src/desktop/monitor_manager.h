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

namespace umbra {

// A monitor's virtual-desktop geometry. x/y can be negative — the virtual
// desktop origin is the primary monitor's top-left corner, and secondary
// monitors placed above/left of it get negative coordinates.
struct MonitorInfo {
    std::string id;  // stable device identifier, e.g. "\\.\DISPLAY1"
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool isPrimary = false;

    bool operator==(const MonitorInfo&) const = default;
};

// Abstracts the actual OS monitor enumeration (EnumDisplayMonitors on
// Windows) so MonitorManager's hot-plug diffing logic is unit-testable
// without a live desktop session — see Win32MonitorEnumerator for the real
// implementation.
class IMonitorEnumerator {
   public:
    virtual ~IMonitorEnumerator() = default;
    virtual std::vector<MonitorInfo> enumerate() const = 0;
};

// What changed between two calls to MonitorManager::refresh().
struct MonitorChangeSet {
    std::vector<MonitorInfo> added;
    std::vector<MonitorInfo> removed;
    std::vector<MonitorInfo> changed;  // same id, different geometry/primary flag

    bool isEmpty() const { return added.empty() && removed.empty() && changed.empty(); }
};

// Tracks the current set of connected monitors and reports what changed
// (connect/disconnect/resolution change) each time refresh() is called, so
// callers can react to hot-plug without crashing (per the PRD's
// multi-monitor requirements). Pure diffing logic — no Win32 dependency —
// part of the hexagonal core.
class MonitorManager {
   public:
    explicit MonitorManager(const IMonitorEnumerator& enumerator);

    // Re-enumerates monitors and returns what changed since the last call
    // (or since construction, on the first call — every monitor present
    // then is reported as "added").
    MonitorChangeSet refresh();

    const std::vector<MonitorInfo>& monitors() const { return current_; }

   private:
    const IMonitorEnumerator& enumerator_;
    std::vector<MonitorInfo> current_;
};

}  // namespace umbra
