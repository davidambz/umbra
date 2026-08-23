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
#include "desktop/lock_screen_sync.h"
#include "desktop/monitor_manager.h"
#include "desktop/win32_lock_screen_api.h"
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

    // Creates (or shows, if already created) the native Settings window.
    // Called both from the tray (double-click / "Open Settings") and by
    // main.cpp right after a non-autostart launch finishes initializing.
    void openSettingsWindow();

    // Finds an already-running instance's message window (by class name —
    // see kMessageWindowClassName in application.cpp) and asks it to open
    // its Settings window, mirroring a tray double-click. Called from
    // main.cpp when CreateMutexW finds another instance already holds the
    // single-instance mutex; a no-op if no such window is found.
    static void notifyRunningInstance();

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
    void setAllPaused(bool paused);
    void quit();
    void addTrayIcon();
    void syncLockScreenIfDue();

    std::filesystem::path settingsPath_;
    Settings settings_;
    LibraryManager libraryManager_;

    HINSTANCE instance_ = nullptr;
    HWND messageWindow_ = nullptr;
    // The registered "TaskbarCreated" message id, broadcast by Explorer to
    // every top-level window when it restarts — the standard way a Win32
    // app detects that restart and knows to re-add its tray icon and
    // re-attach its render windows (see handleMessage()). Not a #define'd
    // WM_* constant, so it can't be a switch case label; checked
    // separately before the switch instead.
    UINT taskbarCreatedMessage_ = 0;
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

    Win32LockScreenApi lockScreenApi_;
    LockScreenSync lockScreenSync_;
    // Ticks remaining before syncLockScreenIfDue() is considered, set by
    // rebuildMonitorHostsFromCurrentMonitorList() whenever the primary
    // monitor's render surface is (re)created. -1 means no sync is
    // pending. Counts down every tick regardless of pause state — it's
    // lockScreenPrimaryFramePresented_ below, not this, that decides
    // whether a sync actually happens once it reaches 0.
    int lockScreenSyncCountdown_ = -1;
    // Set (until the next rebuild resets it) the first time onTick() draws
    // the primary monitor's host during the current countdown window. If
    // still false when the countdown reaches 0 — the primary is paused
    // (fullscreen app, battery throttle, manual pause-all) for the whole
    // window, or fps-capped low enough to have not presented yet —
    // syncLockScreenIfDue() is skipped rather than capturing whatever
    // garbage sits in a never-presented back buffer.
    bool lockScreenPrimaryFramePresented_ = false;

    std::vector<std::unique_ptr<MonitorHost>> monitorHosts_;
    bool manuallyPausedAll_ = false;
};

}  // namespace umbra
