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

#include "power/power_state.h"

namespace umbra {

ThrottleAction decideThrottleAction(const PowerState& state, const PowerThrottleConfig& config) {
    if (state.onBattery && config.pauseBelowBatteryPercent >= 0 && state.batteryPercent >= 0 &&
        state.batteryPercent <= config.pauseBelowBatteryPercent) {
        return ThrottleAction::Paused;
    }

    if (state.onBatterySaver) {
        return config.pauseOnBatterySaver ? ThrottleAction::Paused : ThrottleAction::Reduced;
    }

    return ThrottleAction::Normal;
}

}  // namespace umbra
