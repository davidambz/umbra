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

#include "app/render_tick.h"

#include <gtest/gtest.h>

using umbra::evaluateRenderTickGate;

TEST(EvaluateRenderTickGate, UncappedRendersEveryTick) {
    double sinceLastRenderSeconds = 0.0;
    const auto gate = evaluateRenderTickGate(sinceLastRenderSeconds, 1.0 / 60.0, 0);
    EXPECT_TRUE(gate.shouldRender);
    EXPECT_DOUBLE_EQ(gate.elapsedSeconds, 1.0 / 60.0);
    EXPECT_DOUBLE_EQ(sinceLastRenderSeconds, 0.0);
}

TEST(EvaluateRenderTickGate, NegativeFpsCapIsTreatedAsUncapped) {
    double sinceLastRenderSeconds = 0.0;
    const auto gate = evaluateRenderTickGate(sinceLastRenderSeconds, 1.0 / 60.0, -1);
    EXPECT_TRUE(gate.shouldRender);
}

TEST(EvaluateRenderTickGate, WithholdsRenderUntilFrameIntervalElapses) {
    // fpsCap=15 -> frame interval ~= 0.0667s; a single 1/60s tick isn't
    // enough on its own.
    double sinceLastRenderSeconds = 0.0;
    const auto gate = evaluateRenderTickGate(sinceLastRenderSeconds, 1.0 / 60.0, 15);
    EXPECT_FALSE(gate.shouldRender);
    EXPECT_DOUBLE_EQ(sinceLastRenderSeconds, 1.0 / 60.0);
}

TEST(EvaluateRenderTickGate, RendersOnceAccumulatedTimeReachesFrameInterval) {
    double sinceLastRenderSeconds = 0.0;
    const double tickIntervalSeconds = 1.0 / 60.0;
    umbra::RenderTickGate gate;
    for (int i = 0; i < 4; ++i) {
        gate = evaluateRenderTickGate(sinceLastRenderSeconds, tickIntervalSeconds, 15);
    }
    // 4 ticks at 1/60s ~= 0.0667s, right at the 15fps frame interval.
    EXPECT_TRUE(gate.shouldRender);
    EXPECT_NEAR(gate.elapsedSeconds, 4.0 / 60.0, 1e-9);
}

TEST(EvaluateRenderTickGate, ElapsedSecondsAccumulatesRealTimeNotJustOneTick) {
    // Simulates several skipped ticks under a reduced fps cap — advance()
    // must be told about all the real elapsed time, or playback runs in
    // slow motion (this was a real regression before this gate existed).
    // fpsCap=4 -> frame interval = 0.25s; ticks of 0.1s don't reach that
    // until the third one.
    double sinceLastRenderSeconds = 0.0;
    const double tickIntervalSeconds = 0.1;
    umbra::RenderTickGate gate;
    for (int i = 0; i < 3; ++i) {
        gate = evaluateRenderTickGate(sinceLastRenderSeconds, tickIntervalSeconds, 4);
    }
    // 3 ticks of 0.1s = 0.3s total elapsed since the last render, even
    // though only the third tick actually renders.
    EXPECT_TRUE(gate.shouldRender);
    EXPECT_NEAR(gate.elapsedSeconds, 0.3, 1e-9);
}

TEST(EvaluateRenderTickGate, RemainderCarriesOverInsteadOfResettingToZero) {
    // fpsCap=4 -> frame interval = 0.25s. Ticks of 0.2s: first tick doesn't
    // reach 0.25s; second tick's accumulated 0.4s renders and should leave
    // a 0.15s remainder (fmod), not reset to 0 (which would drift pacing).
    double sinceLastRenderSeconds = 0.0;
    const double tickIntervalSeconds = 0.2;

    const auto first = evaluateRenderTickGate(sinceLastRenderSeconds, tickIntervalSeconds, 4);
    EXPECT_FALSE(first.shouldRender);

    const auto second = evaluateRenderTickGate(sinceLastRenderSeconds, tickIntervalSeconds, 4);
    EXPECT_TRUE(second.shouldRender);
    EXPECT_NEAR(second.elapsedSeconds, 0.4, 1e-9);
    EXPECT_NEAR(sinceLastRenderSeconds, 0.15, 1e-9);
}
