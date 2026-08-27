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

#include <fstream>
#include <nlohmann/json.hpp>

namespace umbra {

using json = nlohmann::json;

Settings Settings::loadFromString(const std::string& text) {
    Settings settings;
    if (text.empty()) {
        return settings;
    }

    const json root = json::parse(text);
    settings.launchOnStartup = root.value("launchOnStartup", settings.launchOnStartup);
    settings.pauseOnFullscreen = root.value("pauseOnFullscreen", settings.pauseOnFullscreen);
    settings.pauseOnBattery = root.value("pauseOnBattery", settings.pauseOnBattery);
    settings.syncLockScreen = root.value("syncLockScreen", settings.syncLockScreen);
    settings.syncMonitors = root.value("syncMonitors", settings.syncMonitors);
    settings.themeOverride = root.value("themeOverride", settings.themeOverride);

    if (root.contains("profiles")) {
        for (const auto& item : root.at("profiles")) {
            WallpaperProfile profile;
            profile.path = item.value("path", "");
            profile.type = wallpaperTypeFromString(item.value("type", std::string("video")));
            profile.monitorId = item.value("monitorId", std::string());
            profile.fpsCap = item.value("fpsCap", 60);
            profile.playlistPaths = item.value("playlistPaths", std::vector<std::string>{});
            profile.playlistIntervalSeconds = item.value("playlistIntervalSeconds", 300);
            profile.playlistMode =
                playlistModeFromString(item.value("playlistMode", std::string("sequential")));
            settings.profiles.push_back(profile);
        }
    }

    return settings;
}

std::string Settings::toJsonString() const {
    json root;
    root["launchOnStartup"] = launchOnStartup;
    root["pauseOnFullscreen"] = pauseOnFullscreen;
    root["pauseOnBattery"] = pauseOnBattery;
    root["syncLockScreen"] = syncLockScreen;
    root["syncMonitors"] = syncMonitors;
    root["themeOverride"] = themeOverride;

    json profilesJson = json::array();
    for (const auto& profile : profiles) {
        json profileJson;
        profileJson["path"] = profile.path;
        profileJson["type"] = toString(profile.type);
        profileJson["monitorId"] = profile.monitorId;
        profileJson["fpsCap"] = profile.fpsCap;
        profileJson["playlistPaths"] = profile.playlistPaths;
        profileJson["playlistIntervalSeconds"] = profile.playlistIntervalSeconds;
        profileJson["playlistMode"] = toString(profile.playlistMode);
        profilesJson.push_back(profileJson);
    }
    root["profiles"] = profilesJson;

    return root.dump(2);
}

Settings Settings::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Settings{};
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return loadFromString(content);
}

void Settings::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    file << toJsonString();
}

}  // namespace umbra
