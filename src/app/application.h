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

#include <windows.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "app/autostart.h"
#include "app/win32_registry_api.h"
#include "config/settings.h"
#include "desktop/monitor_manager.h"
#include "desktop/win32_monitor_enumerator.h"
#include "desktop/win32_workerw_api.h"
#include "desktop/workerw_host.h"
#include "library/library_manager.h"
#include "power/fullscreen_watcher.h"
#include "power/power_watcher.h"
#include "power/win32_fullscreen_api.h"
#include "power/win32_power_api.h"
#include "ui/ui_bridge.h"

namespace umbra {

class MonitorHost;
class SettingsWindow;

// The main orchestrator (per ARCHITECTURE.md's "Main App" box): the only
// layer that wires the core (Settings, WallpaperProfile, MonitorManager's
// diffing) together with every Windows-only adapter (WorkerWHost,
// RenderSurface/Compositor, the three engines, the power/fullscreen
// watchers, the tray icon, autostart). Windows-only, verified manually
// against a live desktop session (see TESTING.md) — the pieces of its
// decision logic worth unit-testing on their own (monitor_assignment,
// render_policy, Autostart) already are.
class Application : public IUiBridgeHost {
   public:
    // settingsPath is where Settings persists (e.g.
    // %LOCALAPPDATA%\Umbra\settings.json); storageRoot is where
    // LibraryManager keeps imported wallpapers (e.g.
    // %LOCALAPPDATA%\Umbra\Wallpapers).
    Application(std::filesystem::path settingsPath, std::filesystem::path storageRoot);
    ~Application() override;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Loads Settings, creates the hidden message window + tray icon,
    // spawns WorkerW, and creates one render host per connected monitor
    // that has a wallpaper assigned. Returns false if any unrecoverable
    // step fails (e.g. WorkerW couldn't be spawned).
    bool initialize(HINSTANCE instance);

    // Runs the Win32 message loop until "Quit" is chosen from the tray
    // menu. Returns the process exit code.
    int run();

    // IUiBridgeHost — lets ui_bridge (#9) read/mutate live app state on
    // behalf of settings-ui/ without that dispatch logic needing to know
    // it's talking to *this* orchestrator specifically.
    Settings& settings() override { return settings_; }
    LibraryManager& library() override { return libraryManager_; }
    std::vector<MonitorInfo> monitors() override { return monitorManager_.monitors(); }
    std::string currentTheme() override;
    void persistSettings() override;
    void persistSettingsAndRebuildMonitorHosts() override;
    std::string pickImportSource(WallpaperType type) override;

   private:
    static LRESULT CALLBACK staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    void onTick();
    void onDisplayChange();
    void rebuildMonitorHosts();
    void rebuildMonitorHostsFromCurrentMonitorList();
    void applyRenderPolicies();
    void openSettingsWindow();
    void setAllPaused(bool paused);
    void quit();

    std::filesystem::path settingsPath_;
    Settings settings_;
    LibraryManager libraryManager_;

    HINSTANCE instance_ = nullptr;
    HWND messageWindow_ = nullptr;
    struct TrayIconState;
    std::unique_ptr<TrayIconState> trayIcon_;
    std::unique_ptr<SettingsWindow> settingsWindow_;

    Win32WorkerWApi workerWApi_;
    WorkerWHost workerWHost_;

    Win32MonitorEnumerator monitorEnumerator_;
    MonitorManager monitorManager_;

    Win32FullscreenApi fullscreenApi_;
    FullscreenWatcher fullscreenWatcher_;
    Win32PowerApi powerApi_;
    PowerWatcher powerWatcher_;

    Win32RegistryApi registryApi_;
    Autostart autostart_;

    std::vector<std::unique_ptr<MonitorHost>> monitorHosts_;
    bool manuallyPausedAll_ = false;
};

}  // namespace umbra
