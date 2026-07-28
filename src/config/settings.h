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

#include <string>
#include <vector>

#include "config/wallpaper_profile.h"

namespace umbra {

struct Settings {
    bool launchOnStartup = true;
    bool pauseOnFullscreen = true;
    bool pauseOnBattery = false;
    std::vector<WallpaperProfile> profiles;

    static Settings loadFromString(const std::string& json);
    std::string toJsonString() const;

    static Settings loadFromFile(const std::string& path);
    void saveToFile(const std::string& path) const;
};

}  // namespace umbra
