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

#include "config/settings.h"

#include <gtest/gtest.h>

using umbra::Settings;
using umbra::WallpaperType;

TEST(Settings, DefaultsAreSaneWhenLoadingEmptyString) {
    Settings settings = Settings::loadFromString("");
    EXPECT_TRUE(settings.launchOnStartup);
    EXPECT_TRUE(settings.pauseOnFullscreen);
    EXPECT_FALSE(settings.pauseOnBattery);
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, RoundTripsThroughJson) {
    Settings settings;
    settings.launchOnStartup = false;
    settings.pauseOnFullscreen = false;
    settings.pauseOnBattery = true;

    umbra::WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.type = WallpaperType::Video;
    profile.monitorIndex = 1;
    profile.fpsCap = 30;
    settings.profiles.push_back(profile);

    const std::string json = settings.toJsonString();
    const Settings reloaded = Settings::loadFromString(json);

    EXPECT_EQ(reloaded.launchOnStartup, settings.launchOnStartup);
    EXPECT_EQ(reloaded.pauseOnFullscreen, settings.pauseOnFullscreen);
    EXPECT_EQ(reloaded.pauseOnBattery, settings.pauseOnBattery);
    ASSERT_EQ(reloaded.profiles.size(), 1u);
    EXPECT_EQ(reloaded.profiles[0].path, profile.path);
    EXPECT_EQ(reloaded.profiles[0].type, profile.type);
    EXPECT_EQ(reloaded.profiles[0].monitorIndex, profile.monitorIndex);
    EXPECT_EQ(reloaded.profiles[0].fpsCap, profile.fpsCap);
}

TEST(Settings, LoadFromMissingFileReturnsDefaults) {
    Settings settings = Settings::loadFromFile("/nonexistent/path/settings.json");
    EXPECT_TRUE(settings.launchOnStartup);
    EXPECT_TRUE(settings.profiles.empty());
}
