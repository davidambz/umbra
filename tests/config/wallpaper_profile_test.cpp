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

#include "config/wallpaper_profile.h"

#include <gtest/gtest.h>

using umbra::WallpaperProfile;
using umbra::WallpaperType;

TEST(WallpaperProfile, DefaultIsInvalidWithoutPath) {
    WallpaperProfile profile;
    EXPECT_FALSE(profile.isValid());
}

TEST(WallpaperProfile, ValidWhenPathAndFpsCapAreSet) {
    WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.fpsCap = 30;
    EXPECT_TRUE(profile.isValid());
}

TEST(WallpaperProfile, InvalidWithZeroFpsCap) {
    WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.fpsCap = 0;
    EXPECT_FALSE(profile.isValid());
}

TEST(WallpaperProfile, IsPlaylistIsFalseByDefault) {
    WallpaperProfile profile;
    EXPECT_FALSE(profile.isPlaylist());
}

TEST(WallpaperProfile, IsPlaylistIsTrueWhenPlaylistPathsIsNonEmpty) {
    WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.playlistPaths = {"C:/wallpapers/rain.mp4", "C:/wallpapers/snow.mp4"};
    EXPECT_TRUE(profile.isPlaylist());
}

TEST(WallpaperProfile, InvalidWithNonPositivePlaylistIntervalWhenActingAsAPlaylist) {
    WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.playlistPaths = {"C:/wallpapers/rain.mp4"};
    profile.playlistIntervalSeconds = 0;
    EXPECT_FALSE(profile.isValid());
}

TEST(WallpaperType, RoundTripsThroughString) {
    EXPECT_EQ(umbra::toString(WallpaperType::Video), "video");
    EXPECT_EQ(umbra::toString(WallpaperType::Image), "image");
    EXPECT_EQ(umbra::toString(WallpaperType::Web), "web");

    EXPECT_EQ(umbra::wallpaperTypeFromString("video"), WallpaperType::Video);
    EXPECT_EQ(umbra::wallpaperTypeFromString("image"), WallpaperType::Image);
    EXPECT_EQ(umbra::wallpaperTypeFromString("web"), WallpaperType::Web);
}

TEST(WallpaperType, ThrowsOnUnknownString) {
    EXPECT_THROW(umbra::wallpaperTypeFromString("bogus"), std::invalid_argument);
}
