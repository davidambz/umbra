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

#include "power/power_watcher.h"

namespace umbra {

PowerWatcher::PowerWatcher(const IPowerApi& api, PowerThrottleConfig config)
    : api_(api), config_(config) {}

bool PowerWatcher::refresh() {
    const ThrottleAction action = decideThrottleAction(api_.queryState(), config_);
    const bool changed = action != currentAction_;
    currentAction_ = action;
    return changed;
}

}  // namespace umbra
