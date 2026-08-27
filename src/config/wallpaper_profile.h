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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "playlist/playlist.h"

namespace umbra {

enum class WallpaperType {
    Video,
    Image,
    Web,
};

struct WallpaperProfile {
    // The active content folder for this monitor: a fixed single
    // wallpaper, or (when isPlaylist() is true) whichever playlistPaths
    // entry is currently showing. Always kept in sync with type below, so
    // a caller that doesn't care about playlists can keep treating this
    // as "the wallpaper for this monitor" — see ui_bridge.cpp, which is
    // what actually advances it on rotation.
    std::string path;
    WallpaperType type = WallpaperType::Video;
    // MonitorInfo::id of the monitor this profile targets — a stable
    // device identifier, not a recomputed canonical-order position, so an
    // assignment survives monitors being unplugged/replugged/reordered.
    // Empty means unassigned.
    std::string monitorId;
    int fpsCap = 60;

    // Playlist rotation across multiple imported wallpapers (see
    // playlist/playlist.h) — empty when this profile is a single fixed
    // wallpaper. Each entry is an absolute path under LibraryManager's
    // storage root, same convention as `path` above; a playlist entry's
    // WallpaperType isn't stored here since it's cheap to re-detect from
    // the folder's contents (LibraryManager::detectWallpaperType) at
    // rotation time, and storing it would just be a second place it could
    // go stale.
    std::vector<std::string> playlistPaths;
    int playlistIntervalSeconds = 300;
    PlaylistMode playlistMode = PlaylistMode::Sequential;

    bool isPlaylist() const { return !playlistPaths.empty(); }
    bool isValid() const;
};

std::string toString(WallpaperType type);
WallpaperType wallpaperTypeFromString(const std::string& value);

}  // namespace umbra
