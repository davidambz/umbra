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

#include "engines/frame_scheduler.h"

#include <gtest/gtest.h>

using umbra::FrameScheduler;

TEST(FrameScheduler, StartsOnFrameZero) {
    FrameScheduler scheduler({100, 100, 100});
    EXPECT_EQ(scheduler.currentIndex(), 0);
}

TEST(FrameScheduler, EmptyDelayListStaysOnFrameZeroRegardlessOfElapsedTime) {
    FrameScheduler scheduler({});
    scheduler.advance(100.0);
    EXPECT_EQ(scheduler.currentIndex(), 0);
}

TEST(FrameScheduler, AdvancesToNextFrameOncePastItsDelay) {
    FrameScheduler scheduler({100, 200, 300});  // ms
    EXPECT_EQ(scheduler.advance(0.05), 0);      // 50ms < 100ms
    EXPECT_EQ(scheduler.advance(0.06), 1);      // 110ms total, past frame 0's 100ms
}

TEST(FrameScheduler, LoopsBackToFrameZeroAfterTheLastFrame) {
    FrameScheduler scheduler({100, 100});  // total loop: 200ms
    scheduler.advance(0.25);  // 250ms: frame 0 (100ms) + frame 1 (100ms) + 50ms into loop 2
    EXPECT_EQ(scheduler.currentIndex(), 0);
}

TEST(FrameScheduler, SkipsMultipleFramesInOneLargeAdvance) {
    FrameScheduler scheduler({10, 10, 10, 10});
    EXPECT_EQ(scheduler.advance(0.035), 3);  // 35ms clears frames 0,1,2 (10ms each)
}

TEST(FrameScheduler, ClampsNonPositiveDelaysToTheMinimum) {
    FrameScheduler scheduler({0, 0});
    // Both frames clamp to kMinFrameDelayMs; advancing by exactly one
    // minimum delay should move to frame 1, not spin forever.
    EXPECT_EQ(scheduler.advance(FrameScheduler::kMinFrameDelayMs / 1000.0), 1);
}
