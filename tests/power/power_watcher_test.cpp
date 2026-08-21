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

#include <gtest/gtest.h>

#include "mocks/win32_mocks.h"

using ::testing::Return;
using umbra::PowerState;
using umbra::PowerThrottleConfig;
using umbra::PowerWatcher;
using umbra::ThrottleAction;
using umbra::testing_support::MockPowerApi;

TEST(PowerWatcher, RefreshReportsChangeOnFirstTransitionAwayFromNormal) {
    MockPowerApi api;
    EXPECT_CALL(api, queryState())
        .WillOnce(Return(PowerState{.onBatterySaver = true, .onBattery = true}));

    PowerWatcher watcher(api, PowerThrottleConfig{});
    EXPECT_TRUE(watcher.refresh());
    EXPECT_EQ(watcher.currentAction(), ThrottleAction::Reduced);
}

TEST(PowerWatcher, RefreshReportsNoChangeWhileActionIsStable) {
    MockPowerApi api;
    EXPECT_CALL(api, queryState())
        .WillRepeatedly(Return(PowerState{.onBatterySaver = false, .onBattery = false}));

    PowerWatcher watcher(api, PowerThrottleConfig{});
    EXPECT_FALSE(watcher.refresh());
    EXPECT_FALSE(watcher.refresh());
    EXPECT_EQ(watcher.currentAction(), ThrottleAction::Normal);
}

TEST(PowerWatcher, RefreshReportsChangeOnEachActualTransition) {
    MockPowerApi api;
    {
        testing::InSequence sequence;
        EXPECT_CALL(api, queryState())
            .WillOnce(Return(PowerState{.onBatterySaver = true, .onBattery = true}));
        EXPECT_CALL(api, queryState())
            .WillOnce(Return(PowerState{.onBatterySaver = false, .onBattery = false}));
    }

    PowerWatcher watcher(api, PowerThrottleConfig{});
    ASSERT_TRUE(watcher.refresh());
    ASSERT_EQ(watcher.currentAction(), ThrottleAction::Reduced);

    EXPECT_TRUE(watcher.refresh());
    EXPECT_EQ(watcher.currentAction(), ThrottleAction::Normal);
}

TEST(PowerWatcher, SetConfigTakesEffectOnTheNextRefresh) {
    MockPowerApi api;
    EXPECT_CALL(api, queryState())
        .WillRepeatedly(Return(PowerState{.onBatterySaver = false, .onBattery = true}));

    PowerWatcher watcher(api, PowerThrottleConfig{});
    ASSERT_FALSE(watcher.refresh());
    ASSERT_EQ(watcher.currentAction(), ThrottleAction::Normal);

    PowerThrottleConfig updated;
    updated.pauseOnBattery = true;
    watcher.setConfig(updated);

    EXPECT_TRUE(watcher.refresh());
    EXPECT_EQ(watcher.currentAction(), ThrottleAction::Paused);
}
