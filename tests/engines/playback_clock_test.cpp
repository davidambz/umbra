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

#include "engines/playback_clock.h"

#include <gtest/gtest.h>

using umbra::PlaybackClock;

TEST(PlaybackClock, PositionAdvancesByDeltaSeconds) {
    PlaybackClock clock(10.0, /*fpsCap=*/0);
    clock.advance(2.5);
    EXPECT_DOUBLE_EQ(clock.positionSeconds(), 2.5);
}

TEST(PlaybackClock, PositionLoopsBackAfterDuration) {
    PlaybackClock clock(4.0, /*fpsCap=*/0);
    clock.advance(3.0);
    clock.advance(3.0);
    EXPECT_DOUBLE_EQ(clock.positionSeconds(), 2.0);
}

TEST(PlaybackClock, ZeroDurationDisablesLooping) {
    PlaybackClock clock(0.0, /*fpsCap=*/0);
    clock.advance(5.0);
    EXPECT_DOUBLE_EQ(clock.positionSeconds(), 0.0);
}

TEST(PlaybackClock, UncappedFpsIsAlwaysDueANewFrame) {
    PlaybackClock clock(10.0, /*fpsCap=*/0);
    EXPECT_TRUE(clock.advance(0.001));
    EXPECT_TRUE(clock.advance(0.001));
}

TEST(PlaybackClock, CappedFpsWithholdsFrameUntilIntervalElapses) {
    PlaybackClock clock(10.0, /*fpsCap=*/10);  // one frame due every 0.1s
    EXPECT_FALSE(clock.advance(0.05));
    EXPECT_TRUE(clock.advance(0.06));  // 0.11s total, clears the 0.1s interval
}

TEST(PlaybackClock, CappedFpsCanProduceMultipleDueFramesAcrossOneLargeAdvance) {
    PlaybackClock clock(10.0, /*fpsCap=*/10);
    EXPECT_TRUE(clock.advance(0.36));
    // 0.36s / 0.1s interval leaves ~0.06s of remainder before the next is due.
    EXPECT_FALSE(clock.advance(0.03));
    EXPECT_TRUE(clock.advance(0.03));
}
