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

#include "ui/ui_bridge.h"

#include <algorithm>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "app/monitor_assignment.h"

namespace umbra {

namespace {

using json = nlohmann::json;

std::string titleOf(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string renamedPath(const std::string& path, const std::string& oldTitle,
                        const std::string& newTitle) {
    const std::filesystem::path p(path);
    if (p.filename().string() != oldTitle) {
        return path;
    }
    return (p.parent_path() / newTitle).string();
}

const LibraryEntry* findEntry(const std::vector<LibraryEntry>& entries, const std::string& id) {
    for (const auto& entry : entries) {
        if (entry.title == id) {
            return &entry;
        }
    }
    return nullptr;
}

json monitorToJson(const MonitorInfo& monitor) {
    return json{{"id", monitor.id},         {"x", monitor.x},
                {"y", monitor.y},           {"width", monitor.width},
                {"height", monitor.height}, {"isPrimary", monitor.isPrimary}};
}

json monitorsToJson(const std::vector<MonitorInfo>& monitors) {
    json array = json::array();
    for (const auto& monitor : monitors) {
        array.push_back(monitorToJson(monitor));
    }
    return array;
}

json libraryEntryToJson(const LibraryEntry& entry) {
    return json{{"id", entry.title}, {"title", entry.title}, {"type", toString(entry.type)}};
}

json libraryToJson(const std::vector<LibraryEntry>& entries) {
    json array = json::array();
    for (const auto& entry : entries) {
        array.push_back(libraryEntryToJson(entry));
    }
    return array;
}

json assignmentToJson(const WallpaperProfile* profile) {
    if (profile == nullptr) {
        return json{{"kind", "none"}};
    }
    if (profile->isPlaylist()) {
        json wallpaperIds = json::array();
        for (const auto& path : profile->playlistPaths) {
            wallpaperIds.push_back(titleOf(path));
        }
        return json{{"kind", "playlist"},
                    {"playlist", json{{"wallpaperIds", wallpaperIds},
                                      {"intervalSeconds", profile->playlistIntervalSeconds},
                                      {"mode", toString(profile->playlistMode)}}},
                    {"fpsCap", profile->fpsCap}};
    }
    return json{
        {"kind", "single"}, {"wallpaperId", titleOf(profile->path)}, {"fpsCap", profile->fpsCap}};
}

json settingsToJson(const Settings& settings) {
    return json{{"launchOnStartup", settings.launchOnStartup},
                {"pauseOnFullscreen", settings.pauseOnFullscreen},
                {"pauseOnBattery", settings.pauseOnBattery}};
}

// Erases every profile currently targeting monitorIndex — assignSingle/
// assignPlaylist/clearAssignment all start from a clean slate for that
// monitor rather than trying to patch an existing entry in place.
void clearProfilesForMonitor(Settings& settings, int monitorIndex) {
    auto& profiles = settings.profiles;
    profiles.erase(std::remove_if(profiles.begin(), profiles.end(),
                                  [monitorIndex](const WallpaperProfile& p) {
                                      return p.monitorIndex == monitorIndex;
                                  }),
                   profiles.end());
}

int requireMonitorIndex(const std::vector<MonitorInfo>& monitors, const std::string& monitorId) {
    const int index = indexOfMonitor(monitors, monitorId);
    if (index < 0) {
        throw std::invalid_argument("unknown monitorId: " + monitorId);
    }
    return index;
}

const LibraryEntry& requireLibraryEntry(const std::vector<LibraryEntry>& entries,
                                        const std::string& id) {
    const LibraryEntry* entry = findEntry(entries, id);
    if (entry == nullptr) {
        throw std::invalid_argument("unknown wallpaper id: " + id);
    }
    return *entry;
}

}  // namespace

UiBridge::UiBridge(IUiBridgeHost& host) : host_(host) {}

std::string UiBridge::buildThemeChangedEvent(const std::string& theme) {
    return json{{"event", "themeChanged"}, {"payload", theme}}.dump();
}

std::string UiBridge::handleRequest(const std::string& rawRequestJson) {
    json id = nullptr;
    try {
        const json request = json::parse(rawRequestJson);
        id = request.at("id");
        const std::string method = request.at("method").get<std::string>();
        const json params = request.contains("params") ? request.at("params") : json::object();

        json result;
        if (method == "getMonitors") {
            result = monitorsToJson(host_.monitors());
        } else if (method == "getLibrary") {
            result = libraryToJson(host_.library().list());
        } else if (method == "getAssignment") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const auto monitors = host_.monitors();
            const auto assignments = assignProfilesToMonitors(monitors, host_.settings().profiles);
            const WallpaperProfile* profile = nullptr;
            for (const auto& assignment : assignments) {
                if (assignment.monitor.id == monitorId) {
                    profile = assignment.profile;
                    break;
                }
            }
            result = assignmentToJson(profile);
        } else if (method == "getSettings") {
            result = settingsToJson(host_.settings());
        } else if (method == "getTheme") {
            result = host_.currentTheme();
        } else if (method == "assignSingle") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const std::string wallpaperId = params.at("wallpaperId").get<std::string>();
            const int fpsCap = params.at("fpsCap").get<int>();

            const int monitorIndex = requireMonitorIndex(host_.monitors(), monitorId);
            // Named rather than chained directly into requireLibraryEntry():
            // that would bind entry to a reference into a temporary
            // std::vector destroyed at the end of this statement, leaving
            // entry dangling for every use below it.
            const auto libraryEntries = host_.library().list();
            const LibraryEntry& entry = requireLibraryEntry(libraryEntries, wallpaperId);

            WallpaperProfile profile;
            profile.path = entry.path.string();
            profile.type = entry.type;
            profile.monitorIndex = monitorIndex;
            profile.fpsCap = fpsCap;

            clearProfilesForMonitor(host_.settings(), monitorIndex);
            host_.settings().profiles.push_back(profile);
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "assignPlaylist") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const json playlistJson = params.at("playlist");
            const int fpsCap = params.at("fpsCap").get<int>();

            const int monitorIndex = requireMonitorIndex(host_.monitors(), monitorId);
            const auto libraryEntries = host_.library().list();

            std::vector<std::string> playlistPaths;
            WallpaperType firstType = WallpaperType::Video;
            for (const auto& wallpaperIdJson : playlistJson.at("wallpaperIds")) {
                const LibraryEntry& entry =
                    requireLibraryEntry(libraryEntries, wallpaperIdJson.get<std::string>());
                if (playlistPaths.empty()) {
                    firstType = entry.type;
                }
                playlistPaths.push_back(entry.path.string());
            }
            if (playlistPaths.empty()) {
                throw std::invalid_argument("a playlist needs at least one wallpaper");
            }
            const int intervalSeconds = playlistJson.at("intervalSeconds").get<int>();
            const PlaylistMode mode =
                playlistModeFromString(playlistJson.at("mode").get<std::string>());

            WallpaperProfile profile;
            profile.path = playlistPaths.front();
            profile.type = firstType;
            profile.monitorIndex = monitorIndex;
            profile.fpsCap = fpsCap;
            profile.playlistPaths = std::move(playlistPaths);
            profile.playlistIntervalSeconds = intervalSeconds;
            profile.playlistMode = mode;

            clearProfilesForMonitor(host_.settings(), monitorIndex);
            host_.settings().profiles.push_back(profile);
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "clearAssignment") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const int monitorIndex = requireMonitorIndex(host_.monitors(), monitorId);
            clearProfilesForMonitor(host_.settings(), monitorIndex);
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "importWallpaper") {
            const std::string title = params.at("title").get<std::string>();
            const WallpaperType type =
                wallpaperTypeFromString(params.at("type").get<std::string>());

            const std::string sourcePath = host_.pickImportSource(type);
            if (sourcePath.empty()) {
                result = nullptr;  // user cancelled the picker
            } else {
                const ImportResult imported = host_.library().import(title, sourcePath);
                result = imported.success
                             ? libraryEntryToJson(LibraryEntry{title, imported.profile.type,
                                                               host_.library().pathForTitle(title)})
                             : json(nullptr);
            }
        } else if (method == "renameWallpaper") {
            const std::string oldTitle = params.at("id").get<std::string>();
            const std::string newTitle = params.at("newTitle").get<std::string>();
            if (host_.library().rename(oldTitle, newTitle)) {
                bool affectedAnyProfile = false;
                for (auto& profile : host_.settings().profiles) {
                    if (titleOf(profile.path) == oldTitle) {
                        affectedAnyProfile = true;
                    }
                    profile.path = renamedPath(profile.path, oldTitle, newTitle);
                    for (auto& path : profile.playlistPaths) {
                        if (titleOf(path) == oldTitle) {
                            affectedAnyProfile = true;
                        }
                        path = renamedPath(path, oldTitle, newTitle);
                    }
                }
                // A rename that didn't touch any monitor's actual
                // assignment doesn't need every render host rebuilt —
                // that restarts playback everywhere, per
                // rebuildMonitorHosts()'s own doc comment.
                if (affectedAnyProfile) {
                    host_.persistSettingsAndRebuildMonitorHosts();
                } else {
                    host_.persistSettings();
                }
            }
            result = nullptr;
        } else if (method == "removeWallpaper") {
            const std::string removedId = params.at("id").get<std::string>();
            if (host_.library().remove(removedId)) {
                bool affectedAnyProfile = false;
                auto& profiles = host_.settings().profiles;
                for (auto& profile : profiles) {
                    auto& paths = profile.playlistPaths;
                    const size_t sizeBefore = paths.size();
                    paths.erase(std::remove_if(
                                    paths.begin(), paths.end(),
                                    [&](const std::string& p) { return titleOf(p) == removedId; }),
                                paths.end());
                    if (paths.size() != sizeBefore) {
                        affectedAnyProfile = true;
                    }
                    if (!paths.empty() && titleOf(profile.path) == removedId) {
                        affectedAnyProfile = true;
                        profile.path = paths.front();
                    }
                }
                const size_t profileCountBefore = profiles.size();
                profiles.erase(std::remove_if(profiles.begin(), profiles.end(),
                                              [&](const WallpaperProfile& p) {
                                                  if (p.isPlaylist()) {
                                                      return p.playlistPaths.empty();
                                                  }
                                                  return titleOf(p.path) == removedId;
                                              }),
                               profiles.end());
                if (profiles.size() != profileCountBefore) {
                    affectedAnyProfile = true;
                }

                if (affectedAnyProfile) {
                    host_.persistSettingsAndRebuildMonitorHosts();
                } else {
                    host_.persistSettings();
                }
            }
            result = nullptr;
        } else if (method == "updateSettings") {
            Settings& settings = host_.settings();
            if (params.contains("launchOnStartup")) {
                settings.launchOnStartup = params.at("launchOnStartup").get<bool>();
            }
            if (params.contains("pauseOnFullscreen")) {
                settings.pauseOnFullscreen = params.at("pauseOnFullscreen").get<bool>();
            }
            if (params.contains("pauseOnBattery")) {
                settings.pauseOnBattery = params.at("pauseOnBattery").get<bool>();
            }
            host_.persistSettings();
            result = nullptr;
        } else {
            throw std::invalid_argument("unknown method: " + method);
        }

        return json{{"id", id}, {"result", result}}.dump();
    } catch (const std::exception& e) {
        return json{{"id", id}, {"error", std::string(e.what())}}.dump();
    }
}

}  // namespace umbra
