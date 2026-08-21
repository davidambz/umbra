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

#include <gtest/gtest.h>

using umbra::decideThrottleAction;
using umbra::PowerState;
using umbra::PowerThrottleConfig;
using umbra::ThrottleAction;

TEST(DecideThrottleAction, NormalWhenOnAcPowerAndNoBatterySaver) {
    const PowerState state{.onBatterySaver = false, .onBattery = false, .batteryPercent = 80};
    EXPECT_EQ(decideThrottleAction(state, PowerThrottleConfig{}), ThrottleAction::Normal);
}

TEST(DecideThrottleAction, ReducedWhenBatterySaverIsOnByDefault) {
    const PowerState state{.onBatterySaver = true, .onBattery = true, .batteryPercent = 50};
    EXPECT_EQ(decideThrottleAction(state, PowerThrottleConfig{}), ThrottleAction::Reduced);
}

TEST(DecideThrottleAction, PausedWhenBatterySaverIsOnAndConfiguredToPause) {
    const PowerState state{.onBatterySaver = true, .onBattery = true, .batteryPercent = 50};
    PowerThrottleConfig config;
    config.pauseOnBatterySaver = true;
    EXPECT_EQ(decideThrottleAction(state, config), ThrottleAction::Paused);
}

TEST(DecideThrottleAction, PausedWhenBatteryDropsToOrBelowConfiguredThreshold) {
    PowerThrottleConfig config;
    config.pauseBelowBatteryPercent = 10;

    const PowerState atThreshold{.onBatterySaver = false, .onBattery = true, .batteryPercent = 10};
    EXPECT_EQ(decideThrottleAction(atThreshold, config), ThrottleAction::Paused);

    const PowerState aboveThreshold{
        .onBatterySaver = false, .onBattery = true, .batteryPercent = 11};
    EXPECT_EQ(decideThrottleAction(aboveThreshold, config), ThrottleAction::Normal);
}

TEST(DecideThrottleAction, LowBatteryThresholdIsIgnoredWhileOnAcPower) {
    PowerThrottleConfig config;
    config.pauseBelowBatteryPercent = 10;

    // batteryPercent below the threshold, but onBattery is false (plugged
    // in) — shouldn't pause just because the battery happens to be low.
    const PowerState state{.onBatterySaver = false, .onBattery = false, .batteryPercent = 5};
    EXPECT_EQ(decideThrottleAction(state, config), ThrottleAction::Normal);
}

TEST(DecideThrottleAction, LowBatteryThresholdDisabledByDefault) {
    const PowerState state{.onBatterySaver = false, .onBattery = true, .batteryPercent = 1};
    EXPECT_EQ(decideThrottleAction(state, PowerThrottleConfig{}), ThrottleAction::Normal);
}

TEST(DecideThrottleAction, LowBatteryTakesPriorityOverBatterySaverReduced) {
    PowerThrottleConfig config;
    config.pauseBelowBatteryPercent = 10;

    const PowerState state{.onBatterySaver = true, .onBattery = true, .batteryPercent = 5};
    EXPECT_EQ(decideThrottleAction(state, config), ThrottleAction::Paused);
}

TEST(DecideThrottleAction, UnknownBatteryPercentDoesNotTriggerLowBatteryPause) {
    PowerThrottleConfig config;
    config.pauseBelowBatteryPercent = 10;

    const PowerState state{.onBatterySaver = false, .onBattery = true, .batteryPercent = -1};
    EXPECT_EQ(decideThrottleAction(state, config), ThrottleAction::Normal);
}
