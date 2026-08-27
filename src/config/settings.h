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
    // Off by default — silently overwriting the user's actual lock screen
    // picture is not something to opt them into without asking.
    bool syncLockScreen = false;
    // Off by default. While on, ui_bridge.cpp mirrors every monitor's
    // wallpaper assignment to every other connected monitor — turning it
    // on itself copies the primary monitor's current assignment to the
    // rest, per issue #71.
    bool syncMonitors = false;
    // "system" (default, follows the live Windows theme), "light", or
    // "dark" — a user override of ARCHITECTURE.md's "follows the Windows
    // system theme" default. Resolved against the live OS theme by
    // UiBridge::resolveTheme(), not stored pre-resolved, so flipping this
    // back to "system" immediately picks up whatever the OS theme
    // currently is.
    std::string themeOverride = "system";
    // "system" (default, follows the OS UI language) or an explicit locale
    // tag ("en", "pt-BR", "es", "zh-CN", "fr", "ru") — same override
    // pattern as themeOverride above. Resolved against the live OS
    // language by the native side (tray menu strings) and by settings-ui's
    // own i18n module, not stored pre-resolved.
    std::string languageOverride = "system";
    std::vector<WallpaperProfile> profiles;

    static Settings loadFromString(const std::string& json);
    std::string toJsonString() const;

    static Settings loadFromFile(const std::string& path);
    void saveToFile(const std::string& path) const;
};

}  // namespace umbra
