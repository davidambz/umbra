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

#include "app/version_compare.h"

#include <gtest/gtest.h>

using umbra::isNewerVersion;
using umbra::parseVersion;
using umbra::Version;

TEST(ParseVersion, ParsesAPlainXYZString) {
    EXPECT_EQ(parseVersion("1.2.3"), Version(1, 2, 3));
    EXPECT_EQ(parseVersion("0.0.0"), Version(0, 0, 0));
    EXPECT_EQ(parseVersion("10.20.300"), Version(10, 20, 300));
}

TEST(ParseVersion, StripsALeadingVFromAGitHubTagName) {
    EXPECT_EQ(parseVersion("v1.2.3"), Version(1, 2, 3));
    EXPECT_EQ(parseVersion("v0.1.0"), Version(0, 1, 0));
}

TEST(ParseVersion, RejectsAnythingNotShapedXYZ) {
    EXPECT_EQ(parseVersion(""), std::nullopt);
    EXPECT_EQ(parseVersion("v"), std::nullopt);
    EXPECT_EQ(parseVersion("1.2"), std::nullopt);
    EXPECT_EQ(parseVersion("1.2.3.4"), std::nullopt);
    EXPECT_EQ(parseVersion("1.2.3-beta.1"), std::nullopt);
    EXPECT_EQ(parseVersion("v1.2.3-rc1"), std::nullopt);
    EXPECT_EQ(parseVersion("a.b.c"), std::nullopt);
    EXPECT_EQ(parseVersion("1..3"), std::nullopt);
    EXPECT_EQ(parseVersion(".2.3"), std::nullopt);
    EXPECT_EQ(parseVersion("1.2."), std::nullopt);
}

TEST(IsNewerVersion, TrueWhenAnyComponentIsGreater) {
    EXPECT_TRUE(isNewerVersion(Version(0, 1, 0), Version(0, 2, 0)));
    EXPECT_TRUE(isNewerVersion(Version(0, 1, 0), Version(1, 0, 0)));
    EXPECT_TRUE(isNewerVersion(Version(0, 1, 0), Version(0, 1, 1)));
    EXPECT_TRUE(isNewerVersion(Version(1, 9, 9), Version(2, 0, 0)));
}

TEST(IsNewerVersion, FalseWhenEqualOrOlder) {
    EXPECT_FALSE(isNewerVersion(Version(0, 1, 0), Version(0, 1, 0)));
    EXPECT_FALSE(isNewerVersion(Version(0, 2, 0), Version(0, 1, 0)));
    EXPECT_FALSE(isNewerVersion(Version(1, 0, 0), Version(0, 9, 9)));
}
