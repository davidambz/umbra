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

namespace umbra {

// A snapshot of the OS power state relevant to the PRD's performance
// management requirements. Populated from GetSystemPowerStatus() on
// Windows (see Win32PowerApi); kept OS-agnostic here so decideThrottleAction
// is unit-testable without a live power subsystem.
struct PowerState {
    bool onBatterySaver = false;  // Windows "Battery saver" mode is active.
    bool onBattery = false;       // Running off battery, not AC power.
    int batteryPercent = -1;      // 0-100, or -1 if there's no battery to report.
};

// What a wallpaper's rendering should do in response to the current power
// state, from least to most aggressive.
enum class ThrottleAction {
    Normal,   // render at full configured fps
    Reduced,  // render at reducedFpsCap
    Paused,   // stop rendering entirely
};

// User-configurable policy for how aggressively to react to power state,
// per the PRD's "configurable by the user" requirement for both the
// battery-saver and low-battery cases.
struct PowerThrottleConfig {
    // Paused whenever running on battery at all, per Settings::pauseOnBattery
    // — independent of, and checked before, pauseOnBatterySaver below (which
    // only fires once Windows' own Battery Saver kicks in, typically at a
    // low charge threshold the user doesn't control).
    bool pauseOnBattery = false;
    bool pauseOnBatterySaver = false;  // Paused instead of Reduced when battery saver is on.
    int reducedFpsCap = 15;

    // Paused once battery drops to/below this percent while on battery.
    // -1 disables this check entirely.
    int pauseBelowBatteryPercent = -1;
};

// Pure decision: given the current power state and the user's configured
// policy, what should rendering do right now. No Win32 dependency, so
// unit-testable without a live desktop session — the actual OS polling
// lives behind IPowerApi/Win32PowerApi instead.
ThrottleAction decideThrottleAction(const PowerState& state, const PowerThrottleConfig& config);

}  // namespace umbra
