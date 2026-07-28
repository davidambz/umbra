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

#include <stdexcept>

namespace umbra {

bool WallpaperProfile::isValid() const {
    return !path.empty() && monitorIndex >= 0 && fpsCap > 0;
}

std::string toString(WallpaperType type) {
    switch (type) {
        case WallpaperType::Video: return "video";
        case WallpaperType::Image: return "image";
        case WallpaperType::Web: return "web";
    }
    throw std::invalid_argument("unknown WallpaperType");
}

WallpaperType wallpaperTypeFromString(const std::string& value) {
    if (value == "video") return WallpaperType::Video;
    if (value == "image") return WallpaperType::Image;
    if (value == "web") return WallpaperType::Web;
    throw std::invalid_argument("unknown wallpaper type string: " + value);
}

}  // namespace umbra
