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

#include "power/win32_power_api.h"

#include <windows.h>

namespace umbra {

PowerState Win32PowerApi::queryState() const {
    SYSTEM_POWER_STATUS status{};
    if (!GetSystemPowerStatus(&status)) {
        return PowerState{};
    }

    PowerState state;
    // SystemStatusFlag is documented as 1 when Battery Saver is on, 0
    // otherwise (any other value is treated as "off" rather than crashing
    // on an unexpected flag).
    state.onBatterySaver = status.SystemStatusFlag == 1;
    // ACLineStatus: 0 = offline (on battery), 1 = online (AC), 255 = unknown.
    // Only a confirmed AC connection counts as "not on battery" — treating
    // an unknown line status as AC would silently disable
    // pauseBelowBatteryPercent's low-battery protection on exactly the
    // machines/drivers that report it, which defeats the point of that
    // safeguard.
    state.onBattery = status.ACLineStatus != 1;
    state.batteryPercent =
        status.BatteryLifePercent <= 100 ? static_cast<int>(status.BatteryLifePercent) : -1;
    return state;
}

}  // namespace umbra
