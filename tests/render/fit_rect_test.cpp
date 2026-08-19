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

#include "render/fit_rect.h"

#include <gtest/gtest.h>

using umbra::computeFitRect;
using umbra::FitMode;
using umbra::Rect;
using umbra::Size;

TEST(ComputeFitRect, StretchAlwaysFillsTargetExactly) {
    const Rect rect = computeFitRect(Size{200, 100}, Size{300, 300}, FitMode::Stretch);
    EXPECT_EQ(rect, (Rect{0, 0, 300, 300}));
}

TEST(ComputeFitRect, ContainLetterboxesAWiderSource) {
    // 2:1 source into a 1:1 target: width-limited, bars top/bottom.
    const Rect rect = computeFitRect(Size{200, 100}, Size{100, 100}, FitMode::Contain);
    EXPECT_EQ(rect, (Rect{0, 25, 100, 50}));
}

TEST(ComputeFitRect, ContainPillarboxesATallerSource) {
    // 1:2 source into a 1:1 target: height-limited, bars left/right.
    const Rect rect = computeFitRect(Size{100, 200}, Size{100, 100}, FitMode::Contain);
    EXPECT_EQ(rect, (Rect{25, 0, 50, 100}));
}

TEST(ComputeFitRect, CoverCropsAWiderSource) {
    const Rect rect = computeFitRect(Size{200, 100}, Size{100, 100}, FitMode::Cover);
    EXPECT_EQ(rect, (Rect{-50, 0, 200, 100}));
}

TEST(ComputeFitRect, CoverCropsATallerSource) {
    const Rect rect = computeFitRect(Size{100, 200}, Size{100, 100}, FitMode::Cover);
    EXPECT_EQ(rect, (Rect{0, -50, 100, 200}));
}

TEST(ComputeFitRect, MatchingAspectRatioFillsExactlyRegardlessOfMode) {
    const Size source{100, 100};
    const Size target{200, 200};
    const Rect expected{0, 0, 200, 200};

    EXPECT_EQ(computeFitRect(source, target, FitMode::Stretch), expected);
    EXPECT_EQ(computeFitRect(source, target, FitMode::Contain), expected);
    EXPECT_EQ(computeFitRect(source, target, FitMode::Cover), expected);
}

TEST(ComputeFitRect, ReturnsZeroRectForNonPositiveSourceDimension) {
    EXPECT_EQ(computeFitRect(Size{0, 100}, Size{100, 100}, FitMode::Contain), Rect{});
    EXPECT_EQ(computeFitRect(Size{100, -1}, Size{100, 100}, FitMode::Cover), Rect{});
}

TEST(ComputeFitRect, ReturnsZeroRectForNonPositiveTargetDimension) {
    EXPECT_EQ(computeFitRect(Size{100, 100}, Size{0, 100}, FitMode::Contain), Rect{});
    EXPECT_EQ(computeFitRect(Size{100, 100}, Size{100, -1}, FitMode::Stretch), Rect{});
}
