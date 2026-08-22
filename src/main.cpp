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

#include <shlobj.h>
#include <windows.h>

#include <filesystem>

#include "app/application.h"

namespace {

// %LOCALAPPDATA%\Umbra\settings.json — matches the folder library_manager's
// storage root uses (%LOCALAPPDATA%\Umbra\Wallpapers\<Title>\), so every
// piece of Umbra's own state lives under one per-user folder.
std::filesystem::path resolveSettingsPath() {
    PWSTR localAppData = nullptr;
    std::filesystem::path baseDir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
        baseDir = std::filesystem::path(localAppData);
        CoTaskMemFree(localAppData);
    } else {
        // SHGetKnownFolderPath failing at all is rare, but silently
        // falling through would leave settingsDir empty and persist
        // settings to whatever the process's current working directory
        // happens to be — an explicit, documented last resort instead.
        baseDir = std::filesystem::current_path();
    }

    const std::filesystem::path settingsDir = baseDir / L"Umbra";
    std::error_code ec;
    std::filesystem::create_directories(settingsDir, ec);
    return settingsDir / L"settings.json";
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*previousInstance*/, PWSTR /*commandLine*/,
                    int /*showCommand*/) {
    umbra::Application app(resolveSettingsPath());
    if (!app.initialize(instance)) {
        return 1;
    }
    return app.run();
}
