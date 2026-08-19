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

#include <unordered_map>

namespace umbra {

namespace {

std::unordered_map<std::string, const MonitorInfo*> indexById(
    const std::vector<MonitorInfo>& monitors) {
    std::unordered_map<std::string, const MonitorInfo*> byId;
    for (const auto& monitor : monitors) {
        byId[monitor.id] = &monitor;
    }
    return byId;
}

}  // namespace

MonitorManager::MonitorManager(const IMonitorEnumerator& enumerator) : enumerator_(enumerator) {}

MonitorChangeSet MonitorManager::refresh() {
    std::vector<MonitorInfo> next = enumerator_.enumerate();

    const auto previousById = indexById(current_);
    const auto nextById = indexById(next);

    MonitorChangeSet changes;
    for (const auto& monitor : next) {
        const auto it = previousById.find(monitor.id);
        if (it == previousById.end()) {
            changes.added.push_back(monitor);
        } else if (!(*it->second == monitor)) {
            changes.changed.push_back(monitor);
        }
    }
    for (const auto& monitor : current_) {
        if (!nextById.contains(monitor.id)) {
            changes.removed.push_back(monitor);
        }
    }

    current_ = std::move(next);
    return changes;
}

}  // namespace umbra
