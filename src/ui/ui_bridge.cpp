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
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "app/monitor_assignment.h"

namespace umbra {

namespace {

using json = nlohmann::json;

std::string titleOf(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

// Embeds thumbnailPath's bytes directly as a data: URL rather than serving
// it through the WebView2 virtual host mapping (settings_window.cpp) —
// thumbnails are small (a downscaled preview, not full-res), so there's no
// real cost to this, and it sidesteps adding a second virtual host mapping
// or reusing the settings-ui assets one for unrelated content. Returns an
// empty string if the file can't be read.
std::string thumbnailDataUrl(const std::filesystem::path& thumbnailPath) {
    std::ifstream file(thumbnailPath, std::ios::binary);
    if (!file) {
        return {};
    }
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        return {};
    }

    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve(((bytes.size() + 2) / 3) * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        const unsigned int byte0 = bytes[i];
        const unsigned int byte1 = i + 1 < bytes.size() ? bytes[i + 1] : 0;
        const unsigned int byte2 = i + 2 < bytes.size() ? bytes[i + 2] : 0;
        const unsigned int triple = (byte0 << 16) | (byte1 << 8) | byte2;

        encoded.push_back(kAlphabet[(triple >> 18) & 0x3F]);
        encoded.push_back(kAlphabet[(triple >> 12) & 0x3F]);
        encoded.push_back(i + 1 < bytes.size() ? kAlphabet[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(i + 2 < bytes.size() ? kAlphabet[triple & 0x3F] : '=');
    }

    return "data:image/png;base64," + encoded;
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
    json result{{"id", entry.title}, {"title", entry.title}, {"type", toString(entry.type)}};
    if (!entry.thumbnailPath.empty()) {
        const std::string dataUrl = thumbnailDataUrl(entry.thumbnailPath);
        if (!dataUrl.empty()) {
            result["thumbnailUrl"] = dataUrl;
        }
    }
    return result;
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
                {"pauseOnBattery", settings.pauseOnBattery},
                {"syncLockScreen", settings.syncLockScreen},
                {"syncMonitors", settings.syncMonitors}};
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

// Replaces every monitor's profile with its own copy of profileTemplate
// (monitorIndex overwritten per target) — used when settings.syncMonitors
// is on, so assignSingle/assignPlaylist/clearAssignment (an empty
// playlistPaths/path template, effectively) end up applying to every
// connected monitor at once instead of just the one the caller named, per
// issue #71.
//
// profileTemplate is taken by value rather than by reference deliberately:
// updateSettings' syncMonitors-toggled-on handling calls this with a
// reference into settings.profiles itself (the primary's current
// profile), and clearProfilesForMonitor() below mutates that very vector
// — a by-reference parameter would dangle the moment the loop clears
// whichever monitor that source profile originally belonged to.
void applyProfileToEveryMonitor(Settings& settings, WallpaperProfile profileTemplate,
                                const std::vector<MonitorInfo>& monitors) {
    // Sorted once up front rather than calling indexOfMonitor() per
    // monitor — that would re-sort the whole list from scratch on every
    // call, for what canonicalMonitorOrder() already gives as a single
    // O(N log N) pass.
    const std::vector<MonitorInfo> ordered = canonicalMonitorOrder(monitors);
    for (int monitorIndex = 0; monitorIndex < static_cast<int>(ordered.size()); ++monitorIndex) {
        clearProfilesForMonitor(settings, monitorIndex);
        WallpaperProfile profile = profileTemplate;
        profile.monitorIndex = monitorIndex;
        settings.profiles.push_back(profile);
    }
}

// clearAssignment's syncMonitors counterpart to applyProfileToEveryMonitor
// above — clears every monitor rather than assigning them all the same
// profile. Doesn't need canonicalMonitorOrder() itself: every monitorIndex
// from 0 to monitors.size()-1 exists exactly once regardless of which
// monitor ends up at which index, and clearing doesn't care which is
// which.
void clearEveryMonitor(Settings& settings, const std::vector<MonitorInfo>& monitors) {
    for (int monitorIndex = 0; monitorIndex < static_cast<int>(monitors.size()); ++monitorIndex) {
        clearProfilesForMonitor(settings, monitorIndex);
    }
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

// AddWallpaperDialog.tsx's onImport contract (settings-ui/) is: resolve
// null only for a cancelled picker, throw for an actual import failure —
// so it can tell "the user backed out" apart from "something went
// wrong" and show a message that isn't just misleadingly generic. Each
// case here names the real reason instead of leaving the caller to
// guess from a bare failure.
std::string importErrorMessage(ImportError error, const std::string& title) {
    switch (error) {
        case ImportError::TitleEmpty:
            return "Title can't be empty.";
        case ImportError::InvalidTitle:
            return "\"" + title + "\" isn't a valid title.";
        case ImportError::SourceNotFound:
            return "The selected file couldn't be found.";
        case ImportError::UnsupportedFileType:
            return "That file type isn't supported.";
        case ImportError::WebMissingIndexHtml:
            return "That folder or .zip doesn't have an index.html at its root.";
        case ImportError::DestinationAlreadyExists:
            return "A wallpaper named \"" + title + "\" already exists.";
        case ImportError::CopyFailed:
            return "Failed to copy the file into Umbra's library.";
        case ImportError::None:
            break;
    }
    return "Import failed.";
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

            const auto monitors = host_.monitors();
            const int monitorIndex = requireMonitorIndex(monitors, monitorId);
            // Named rather than chained directly into requireLibraryEntry():
            // that would bind entry to a reference into a temporary
            // std::vector destroyed at the end of this statement, leaving
            // entry dangling for every use below it.
            const auto libraryEntries = host_.library().list();
            const LibraryEntry& entry = requireLibraryEntry(libraryEntries, wallpaperId);

            WallpaperProfile profile;
            profile.path = entry.path.string();
            profile.type = entry.type;
            profile.fpsCap = fpsCap;

            if (host_.settings().syncMonitors) {
                applyProfileToEveryMonitor(host_.settings(), profile, monitors);
            } else {
                profile.monitorIndex = monitorIndex;
                clearProfilesForMonitor(host_.settings(), monitorIndex);
                host_.settings().profiles.push_back(profile);
            }
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "assignPlaylist") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const json playlistJson = params.at("playlist");
            const int fpsCap = params.at("fpsCap").get<int>();

            const auto monitors = host_.monitors();
            const int monitorIndex = requireMonitorIndex(monitors, monitorId);
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
            profile.fpsCap = fpsCap;
            profile.playlistPaths = std::move(playlistPaths);
            profile.playlistIntervalSeconds = intervalSeconds;
            profile.playlistMode = mode;

            if (host_.settings().syncMonitors) {
                applyProfileToEveryMonitor(host_.settings(), profile, monitors);
            } else {
                profile.monitorIndex = monitorIndex;
                clearProfilesForMonitor(host_.settings(), monitorIndex);
                host_.settings().profiles.push_back(profile);
            }
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "clearAssignment") {
            const std::string monitorId = params.at("monitorId").get<std::string>();
            const auto monitors = host_.monitors();
            const int monitorIndex = requireMonitorIndex(monitors, monitorId);
            if (host_.settings().syncMonitors) {
                clearEveryMonitor(host_.settings(), monitors);
            } else {
                clearProfilesForMonitor(host_.settings(), monitorIndex);
            }
            host_.persistSettingsAndRebuildMonitorHosts();
            result = nullptr;
        } else if (method == "importWallpaper") {
            const std::string title = params.at("title").get<std::string>();
            const WallpaperType type =
                wallpaperTypeFromString(params.at("type").get<std::string>());

            const std::filesystem::path sourcePath = host_.pickImportSource(type);
            if (sourcePath.empty()) {
                result = nullptr;  // user cancelled the picker
            } else {
                const ImportResult imported = host_.library().import(title, sourcePath);
                if (imported.success) {
                    const std::filesystem::path contentDir = host_.library().pathForTitle(title);
                    // Generated synchronously so the very first response
                    // (not just a later getLibrary() call) already carries
                    // a thumbnailUrl, matching how the mock bridge's
                    // importWallpaper behaves in settings-ui/'s dev mode.
                    host_.generateThumbnail(title, imported.profile.type, contentDir);
                    const std::filesystem::path thumbnailPath =
                        host_.library().thumbnailPathForTitle(title);
                    std::error_code thumbnailEc;
                    result = libraryEntryToJson(LibraryEntry{
                        title, imported.profile.type, contentDir,
                        std::filesystem::exists(thumbnailPath, thumbnailEc) ? thumbnailPath
                                                                            : std::filesystem::path{}});
                } else {
                    throw std::invalid_argument(importErrorMessage(imported.error, title));
                }
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
            bool needsRebuild = false;
            if (params.contains("launchOnStartup")) {
                settings.launchOnStartup = params.at("launchOnStartup").get<bool>();
            }
            if (params.contains("pauseOnFullscreen")) {
                settings.pauseOnFullscreen = params.at("pauseOnFullscreen").get<bool>();
            }
            if (params.contains("pauseOnBattery")) {
                settings.pauseOnBattery = params.at("pauseOnBattery").get<bool>();
            }
            if (params.contains("syncLockScreen")) {
                settings.syncLockScreen = params.at("syncLockScreen").get<bool>();
            }
            if (params.contains("syncMonitors")) {
                const bool newValue = params.at("syncMonitors").get<bool>();
                if (newValue && !settings.syncMonitors) {
                    // Turning it on: copy the primary monitor's (monitorIndex
                    // 0 — assignProfilesToMonitors' canonical order always
                    // sorts it first) current assignment to every other
                    // monitor, so this has an immediate, predictable effect
                    // rather than silently doing nothing until some
                    // monitor's assignment next happens to change.
                    const auto monitors = host_.monitors();
                    const auto primaryProfile = std::find_if(
                        settings.profiles.begin(), settings.profiles.end(),
                        [](const WallpaperProfile& p) { return p.monitorIndex == 0; });
                    if (primaryProfile != settings.profiles.end()) {
                        applyProfileToEveryMonitor(settings, *primaryProfile, monitors);
                    } else {
                        clearEveryMonitor(settings, monitors);
                    }
                    needsRebuild = true;
                }
                settings.syncMonitors = newValue;
            }
            if (needsRebuild) {
                host_.persistSettingsAndRebuildMonitorHosts();
            } else {
                host_.persistSettings();
            }
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
