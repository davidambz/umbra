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

#include "config/settings.h"
#include "config/wallpaper_profile.h"
#include "desktop/monitor_manager.h"
#include "library/library_manager.h"

namespace umbra {

// What UiBridge needs from the running app to answer a request — settings_window.*
// (#9's Windows-only half) implements this against the real Application;
// tests implement it against a fake. Keeping this interface free of Win32
// types is what makes UiBridge's request-handling logic (the actual JSON
// protocol/validation/mutation logic settings-ui/'s UiBridge contract
// needs) unit-testable without a live WebView2 host.
class IUiBridgeHost {
   public:
    virtual ~IUiBridgeHost() = default;

    virtual Settings& settings() = 0;
    virtual LibraryManager& library() = 0;
    virtual std::vector<MonitorInfo> monitors() = 0;

    // "light" or "dark", per ARCHITECTURE.md's "follows the Windows
    // system theme" requirement.
    virtual std::string currentTheme() = 0;

    // Persists settings() to disk and re-applies its non-monitor-specific
    // effects live (autostart, power-throttle config) — called after any
    // request that mutates settings(), including ones that don't touch
    // any monitor's assignment (updateSettings, a rename/remove that
    // didn't affect an active assignment).
    virtual void persistSettings() = 0;

    // persistSettings(), plus rebuilding every monitor's render host —
    // called only after a request that actually changed which
    // wallpaper(s) are assigned (assignSingle/assignPlaylist/
    // clearAssignment, or a rename/remove that did affect one), since
    // that rebuild restarts playback on every monitor and shouldn't fire
    // for a change that didn't need it.
    virtual void persistSettingsAndRebuildMonitorHosts() = 0;

    // Shows a native "choose a file/folder" picker appropriate for type
    // (a single file for Video/Image, a folder or .zip for Web). Returns
    // the chosen source path, or an empty string if the user cancelled.
    virtual std::string pickImportSource(WallpaperType type) = 0;

    // Best-effort: generates a preview thumbnail for the wallpaper just
    // imported at contentDir (LibraryManager::pathForTitle(title)) and
    // writes it to LibraryManager::thumbnailPathForTitle(title), so it
    // shows up next time getLibrary()/importWallpaper's own response is
    // built. Needs Media Foundation (Video) or WIC (Image) — Win32-only,
    // which is why this can't just live inside LibraryManager::import()
    // itself. A no-op for Web (no single frame to grab) or on any failure;
    // never surfaced as an error to the caller.
    virtual void generateThumbnail(const std::string& title, WallpaperType type,
                                   const std::filesystem::path& contentDir) = 0;
};

// Implements the JSON request/response protocol settings-ui/'s
// src/bridge/uiBridge.ts UiBridge interface expects from window.umbra:
// a request is `{"id": number, "method": string, "params": object}`;
// handleRequest() returns `{"id": ..., "result": ...}` on success or
// `{"id": ..., "error": "..."}` on failure. settings_window.cpp is the
// thin Windows-only layer that actually wires this to WebView2's
// WebMessageReceived/PostWebMessageAsJson.
class UiBridge {
   public:
    // host must outlive this UiBridge.
    explicit UiBridge(IUiBridgeHost& host);

    std::string handleRequest(const std::string& rawRequestJson);

    // The unsolicited event message to push when the native theme
    // changes live (settings-ui/'s onThemeChange listens for this).
    static std::string buildThemeChangedEvent(const std::string& theme);

   private:
    IUiBridgeHost& host_;
};

}  // namespace umbra
