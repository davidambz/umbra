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

#include "app/render_policy.h"

#include <gtest/gtest.h>

using umbra::computeRenderPolicy;
using umbra::ThrottleAction;

TEST(ComputeRenderPolicy, NormalWhenNothingIsActive) {
    const auto policy = computeRenderPolicy(false, true, ThrottleAction::Normal, 60, 15);
    EXPECT_FALSE(policy.paused);
    EXPECT_EQ(policy.fpsCap, 60);
}

TEST(ComputeRenderPolicy, FullscreenPausesWhenPauseOnFullscreenIsEnabled) {
    const auto policy = computeRenderPolicy(true, true, ThrottleAction::Normal, 60, 15);
    EXPECT_TRUE(policy.paused);
}

TEST(ComputeRenderPolicy, FullscreenIsIgnoredWhenPauseOnFullscreenIsDisabled) {
    const auto policy = computeRenderPolicy(true, false, ThrottleAction::Normal, 60, 15);
    EXPECT_FALSE(policy.paused);
    EXPECT_EQ(policy.fpsCap, 60);
}

TEST(ComputeRenderPolicy, PowerPausedOverridesEverything) {
    const auto policy = computeRenderPolicy(false, true, ThrottleAction::Paused, 60, 15);
    EXPECT_TRUE(policy.paused);
}

TEST(ComputeRenderPolicy, PowerReducedCapsFpsToTheLowerOfProfileAndReducedCap) {
    const auto policy = computeRenderPolicy(false, true, ThrottleAction::Reduced, 60, 15);
    EXPECT_FALSE(policy.paused);
    EXPECT_EQ(policy.fpsCap, 15);
}

TEST(ComputeRenderPolicy, PowerReducedNeverRaisesAnAlreadyLowerProfileCap) {
    const auto policy = computeRenderPolicy(false, true, ThrottleAction::Reduced, 10, 15);
    EXPECT_EQ(policy.fpsCap, 10);
}

TEST(ComputeRenderPolicy, FullscreenTakesPriorityOverPowerReduced) {
    const auto policy = computeRenderPolicy(true, true, ThrottleAction::Reduced, 60, 15);
    EXPECT_TRUE(policy.paused);
}
