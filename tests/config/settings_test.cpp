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

#include <filesystem>

using umbra::PlaylistMode;
using umbra::Settings;
using umbra::WallpaperType;

TEST(Settings, DefaultsAreSaneWhenLoadingEmptyString) {
    Settings settings = Settings::loadFromString("");
    EXPECT_TRUE(settings.launchOnStartup);
    EXPECT_TRUE(settings.pauseOnFullscreen);
    EXPECT_FALSE(settings.pauseOnBattery);
    EXPECT_FALSE(settings.pauseOnBatterySaver);
    EXPECT_EQ(settings.reducedFpsCap, 15);
    EXPECT_EQ(settings.pauseBelowBatteryPercent, -1);
    EXPECT_FALSE(settings.syncLockScreen);
    EXPECT_FALSE(settings.syncMonitors);
    EXPECT_EQ(settings.themeOverride, "system");
    EXPECT_EQ(settings.languageOverride, "system");
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, RoundTripsThroughJson) {
    Settings settings;
    settings.launchOnStartup = false;
    settings.pauseOnFullscreen = false;
    settings.pauseOnBattery = true;
    settings.pauseOnBatterySaver = true;
    settings.reducedFpsCap = 10;
    settings.pauseBelowBatteryPercent = 20;
    settings.syncLockScreen = true;
    settings.syncMonitors = true;
    settings.themeOverride = "dark";
    settings.languageOverride = "pt-BR";

    umbra::WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.type = WallpaperType::Video;
    profile.monitorId = "\\\\.\\DISPLAY2";
    profile.fpsCap = 30;
    settings.profiles.push_back(profile);

    const std::string json = settings.toJsonString();
    const Settings reloaded = Settings::loadFromString(json);

    EXPECT_EQ(reloaded.launchOnStartup, settings.launchOnStartup);
    EXPECT_EQ(reloaded.pauseOnFullscreen, settings.pauseOnFullscreen);
    EXPECT_EQ(reloaded.pauseOnBattery, settings.pauseOnBattery);
    EXPECT_EQ(reloaded.pauseOnBatterySaver, settings.pauseOnBatterySaver);
    EXPECT_EQ(reloaded.reducedFpsCap, settings.reducedFpsCap);
    EXPECT_EQ(reloaded.pauseBelowBatteryPercent, settings.pauseBelowBatteryPercent);
    EXPECT_EQ(reloaded.syncLockScreen, settings.syncLockScreen);
    EXPECT_EQ(reloaded.syncMonitors, settings.syncMonitors);
    EXPECT_EQ(reloaded.themeOverride, settings.themeOverride);
    EXPECT_EQ(reloaded.languageOverride, settings.languageOverride);
    ASSERT_EQ(reloaded.profiles.size(), 1u);
    EXPECT_EQ(reloaded.profiles[0].path, profile.path);
    EXPECT_EQ(reloaded.profiles[0].type, profile.type);
    EXPECT_EQ(reloaded.profiles[0].monitorId, profile.monitorId);
    EXPECT_EQ(reloaded.profiles[0].fpsCap, profile.fpsCap);
}

TEST(Settings, RoundTripsPlaylistProfileThroughJson) {
    Settings settings;

    umbra::WallpaperProfile profile;
    profile.path = "C:/wallpapers/rain.mp4";
    profile.monitorId = "\\\\.\\DISPLAY1";
    profile.playlistPaths = {"C:/wallpapers/rain.mp4", "C:/wallpapers/snow.mp4"};
    profile.playlistIntervalSeconds = 120;
    profile.playlistMode = PlaylistMode::Shuffle;
    settings.profiles.push_back(profile);

    const Settings reloaded = Settings::loadFromString(settings.toJsonString());

    ASSERT_EQ(reloaded.profiles.size(), 1u);
    EXPECT_TRUE(reloaded.profiles[0].isPlaylist());
    EXPECT_EQ(reloaded.profiles[0].playlistPaths, profile.playlistPaths);
    EXPECT_EQ(reloaded.profiles[0].playlistIntervalSeconds, profile.playlistIntervalSeconds);
    EXPECT_EQ(reloaded.profiles[0].playlistMode, profile.playlistMode);
}

TEST(Settings, LoadFromMissingFileReturnsDefaults) {
    Settings settings = Settings::loadFromFile("/nonexistent/path/settings.json");
    EXPECT_TRUE(settings.launchOnStartup);
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, LoadFromMalformedJsonReturnsDefaultsInsteadOfThrowing) {
    Settings settings = Settings::loadFromString("{not valid json");
    EXPECT_TRUE(settings.launchOnStartup);
    EXPECT_TRUE(settings.pauseOnFullscreen);
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, LoadFromUnknownWallpaperTypeReturnsDefaultsInsteadOfThrowing) {
    const std::string json =
        R"({"profiles":[{"path":"C:/wallpapers/rain.mp4","type":"not-a-real-type"}]})";
    Settings settings = Settings::loadFromString(json);
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, LoadFromUnknownPlaylistModeReturnsDefaultsInsteadOfThrowing) {
    const std::string json =
        R"({"profiles":[{"path":"C:/wallpapers/rain.mp4","playlistMode":"not-a-real-mode"}]})";
    Settings settings = Settings::loadFromString(json);
    EXPECT_TRUE(settings.profiles.empty());
}

TEST(Settings, OneMalformedProfileIsSkippedWithoutLosingTheOthersOrTopLevelFields) {
    const std::string json = R"({
        "pauseOnFullscreen": false,
        "profiles": [
            {"path":"C:/wallpapers/rain.mp4","type":"video","monitorId":"\\\\.\\DISPLAY1"},
            {"path":"C:/wallpapers/bad.mp4","type":"not-a-real-type"},
            {"path":"C:/wallpapers/snow.mp4","type":"video","monitorId":"\\\\.\\DISPLAY2"}
        ]
    })";
    Settings settings = Settings::loadFromString(json);

    EXPECT_FALSE(settings.pauseOnFullscreen);
    ASSERT_EQ(settings.profiles.size(), 2u);
    EXPECT_EQ(settings.profiles[0].path, "C:/wallpapers/rain.mp4");
    EXPECT_EQ(settings.profiles[1].path, "C:/wallpapers/snow.mp4");
}

TEST(Settings, SaveToFileWritesAtomicallyAndLeavesNoTempFileBehind) {
    const std::filesystem::path dir = std::filesystem::temp_directory_path();
    const std::filesystem::path path = dir / "umbra_settings_atomic_save_test.json";
    const std::filesystem::path tempPath = dir / "umbra_settings_atomic_save_test.json.tmp";
    std::filesystem::remove(path);
    std::filesystem::remove(tempPath);

    Settings settings;
    settings.languageOverride = "pt-BR";
    settings.saveToFile(path.string());

    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_FALSE(std::filesystem::exists(tempPath));

    const Settings reloaded = Settings::loadFromFile(path.string());
    EXPECT_EQ(reloaded.languageOverride, "pt-BR");

    std::filesystem::remove(path);
}

TEST(Settings, SaveToFileOverwritesAnExistingFile) {
    const std::filesystem::path dir = std::filesystem::temp_directory_path();
    const std::filesystem::path path = dir / "umbra_settings_atomic_overwrite_test.json";
    std::filesystem::remove(path);

    Settings first;
    first.languageOverride = "en";
    first.saveToFile(path.string());

    Settings second;
    second.languageOverride = "es";
    second.saveToFile(path.string());

    const Settings reloaded = Settings::loadFromFile(path.string());
    EXPECT_EQ(reloaded.languageOverride, "es");

    std::filesystem::remove(path);
}
