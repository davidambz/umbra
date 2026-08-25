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

#include "app/application.h"

#include <shellapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

#include "app/monitor_assignment.h"
#include "app/render_policy.h"
#include "app/render_tick.h"
#include "engines/image_engine.h"
#include "engines/thumbnail_generator.h"
#include "engines/video_engine.h"
#include "engines/wallpaper_engine.h"
#include "engines/web_engine.h"
#include "engines/win32_text.h"
#include "render/compositor.h"
#include "render/render_surface.h"
#include "ui/settings_window.h"

namespace umbra {

namespace {

constexpr wchar_t kMessageWindowClassName[] = L"UmbraMessageWindow";
constexpr wchar_t kRenderWindowClassName[] = L"UmbraRenderWindow";
constexpr UINT_PTR kTickTimerId = 1;
constexpr UINT kTickIntervalMs = 16;  // ~60Hz; each host paces its own fps below this.
constexpr double kTickIntervalSeconds = kTickIntervalMs / 1000.0;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kMenuOpenSettings = 1;
constexpr UINT kMenuTogglePause = 2;
constexpr UINT kMenuQuit = 3;

// Resolves a wallpaper folder's actual content file (see
// library_manager.cpp's import()): "video.<ext>"/"image.<ext>" for
// Video/Image, "index.html" for Web. Returns an empty path if the expected
// file isn't there (e.g. the folder was deleted out from under Settings).
// Takes a bare dir+type rather than a WallpaperProfile so it works equally
// for a profile's own path and for a playlist entry mid-rotation, whose
// type is re-detected fresh rather than read off the profile (see
// wallpaper_profile.h's playlistPaths comment).
std::filesystem::path resolveContentPath(const std::filesystem::path& dir, WallpaperType type) {
    if (type == WallpaperType::Web) {
        const std::filesystem::path indexHtml = dir / "index.html";
        std::error_code ec;
        return std::filesystem::exists(indexHtml, ec) ? indexHtml : std::filesystem::path{};
    }

    const std::string stem = type == WallpaperType::Video ? "video" : "image";
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.path().stem() == stem) {
            return entry.path();
        }
    }
    return {};
}

// Returns an empty string (rather than a bogus quoted-empty command) if
// the running executable's own path can't be resolved — Autostart::enable()
// refuses to write an empty command to the registry.
//
// The registered command carries --autostart (parsed in main.cpp) so a
// sign-in-triggered launch stays silent instead of popping the Settings
// window like every other launch path does.
std::wstring currentExecutableCommand() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return L"";
    }
    return L"\"" + std::wstring(buffer, length) + L"\" --autostart";
}

std::filesystem::path currentExecutableDirectory() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return {};
    }
    return std::filesystem::path(buffer, buffer + length).parent_path();
}

// "light" or "dark", per ARCHITECTURE.md's "read via UISettings/registry
// (AppsUseLightTheme)". Defaults to dark if the value can't be read (a
// missing key on a very old build is more likely to mean "dark" was never
// overridden than anything else).
std::string readWindowsTheme() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return "dark";
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    const LONG status = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, &type,
                                         reinterpret_cast<BYTE*>(&value), &size);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_DWORD) {
        return "dark";
    }
    return value != 0 ? "light" : "dark";
}

// Shows a native file picker (Video/Image: a single file with an
// appropriate extension filter) or folder picker (Web: a project
// folder — see AddWallpaperDialog.tsx's note that a .zip isn't offered
// through this flow). Returns the chosen path, or an empty string if the
// user cancelled or the dialog couldn't be created.
std::filesystem::path showImportPicker(WallpaperType type) {
    Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) {
        return {};
    }

    if (type == WallpaperType::Web) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS);
    } else if (type == WallpaperType::Video) {
        const COMDLG_FILTERSPEC filters[] = {{L"Video files", L"*.mp4;*.webm"}};
        dialog->SetFileTypes(1, filters);
    } else {
        const COMDLG_FILTERSPEC filters[] = {
            {L"Image files", L"*.gif;*.apng;*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff"}};
        dialog->SetFileTypes(1, filters);
    }

    if (FAILED(dialog->Show(nullptr))) {
        return {};  // user cancelled
    }

    Microsoft::WRL::ComPtr<IShellItem> item;
    if (FAILED(dialog->GetResult(&item))) {
        return {};
    }

    PWSTR rawPath = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &rawPath))) {
        return {};
    }
    // Built straight from the wide string IFileOpenDialog handed back,
    // not round-tripped through a UTF-8 std::string first — fs::path's
    // narrow-string constructor decodes with the current C locale, not
    // UTF-8, which silently mangles any non-ASCII byte (a Unicode
    // separator or accented character in the file's own name is common
    // enough) and made a real, selected file register as "not found".
    const std::filesystem::path path(rawPath);
    CoTaskMemFree(rawPath);
    return path;
}

}  // namespace

// One connected monitor's live rendering pipeline: a WorkerW-attached
// render window plus whichever engine its assigned WallpaperProfile needs.
// VideoEngine/ImageEngine draw through renderSurface/compositor; WebEngine
// draws itself directly into window (see web_engine.h for why) and leaves
// renderSurface/compositor/engine null.
class MonitorHost {
   public:
    ~MonitorHost() {
        // Engines (WebEngine especially — see web_engine.h's "parentWindow
        // must outlive this WebEngine") must be torn down while window is
        // still alive; only then is it safe to destroy it. Declaration
        // order would otherwise destroy window first (member destructors
        // run in reverse declaration order, and window is declared before
        // these), so they're reset explicitly here instead of relying on
        // that order.
        webEngine.reset();
        engine.reset();
        compositor.reset();
        renderSurface.reset();
        if (window != nullptr) {
            DestroyWindow(window);
        }
    }

    MonitorInfo monitor;
    const WallpaperProfile* profile = nullptr;
    HWND window = nullptr;

    std::unique_ptr<RenderSurface> renderSurface;
    std::unique_ptr<Compositor> compositor;
    std::unique_ptr<IWallpaperEngine> engine;
    std::unique_ptr<WebEngine> webEngine;

    bool paused = false;
    bool webPauseApplied = false;
    int fpsCap = 60;
    double sinceLastRenderSeconds = 0.0;

    // Non-null only when `profile` is a playlist (see
    // WallpaperProfile::isPlaylist()) — tracks which entry is currently
    // showing and computes the next one per its rotation mode. Advanced by
    // Application::advancePlaylistRotations() based on elapsed wall time,
    // independently of paused/fps-capped rendering above.
    std::unique_ptr<PlaylistRotator> playlistRotator;
    double sincePlaylistAdvanceSeconds = 0.0;
};

struct Application::TrayIconState {
    NOTIFYICONDATAW data{};
};

namespace {

// (Re)builds the render pipeline for one already-windowed MonitorHost from
// a resolved content file, per `type`. Shared by
// rebuildMonitorHostsFromCurrentMonitorList() (fresh host, no prior engine)
// and advancePlaylistRotations() (host already torn down its previous
// engine before calling this) so the two don't duplicate this logic and
// silently drift apart.
void createEngineForHost(MonitorHost& host, const std::filesystem::path& contentPath,
                         WallpaperType type) {
    if (type == WallpaperType::Web) {
        host.webEngine = std::make_unique<WebEngine>(host.window, contentPath.string());
        host.webEngine->setBounds(host.monitor.width, host.monitor.height);
    } else {
        host.renderSurface =
            std::make_unique<RenderSurface>(host.window, host.monitor.width, host.monitor.height);
        host.compositor = std::make_unique<Compositor>(*host.renderSurface);
        if (type == WallpaperType::Video) {
            host.engine = std::make_unique<VideoEngine>(host.renderSurface->device(),
                                                        contentPath.string(), host.fpsCap);
        } else {
            host.engine = std::make_unique<ImageEngine>(host.renderSurface->device(),
                                                        contentPath.string());
        }
    }
}

}  // namespace

Application::Application(std::filesystem::path settingsPath, std::filesystem::path storageRoot)
    : settingsPath_(std::move(settingsPath)),
      settings_(Settings::loadFromFile(settingsPath_.string())),
      libraryManager_(std::move(storageRoot)),
      workerWHost_(workerWApi_),
      monitorManager_(monitorEnumerator_),
      fullscreenWatcher_(fullscreenApi_),
      powerWatcher_(powerApi_, PowerThrottleConfig{.pauseOnBattery = settings_.pauseOnBattery}),
      autostart_(registryApi_, currentExecutableCommand()),
      lockScreenSync_(lockScreenApi_, settingsPath_.parent_path() / "lockscreen.png") {
    // Set here rather than via a same-line default member initializer on
    // lockScreenSyncWasEnabled_ (application.h) — the constructor body
    // runs after every member is already constructed, regardless of
    // declaration order, so this can't silently start reading a not-yet-
    // loaded settings_ if the members are ever reordered.
    lockScreenSyncWasEnabled_ = settings_.syncLockScreen;
}

void Application::notifyRunningInstance() {
    // main.cpp claims the single-instance mutex before the running
    // instance's message window necessarily exists yet (it's created
    // partway through initialize(), well after that instance claimed the
    // mutex) — a launch that loses the mutex race an instant later could
    // find no window here at all. Retry briefly instead of silently giving
    // up on the first miss; initialize() finishing in under two seconds is
    // the normal case, not a generous allowance.
    HWND existing = nullptr;
    for (int attempt = 0; attempt < 20 && existing == nullptr; ++attempt) {
        existing = FindWindowW(kMessageWindowClassName, L"Umbra");
        if (existing == nullptr) {
            Sleep(100);
        }
    }
    if (existing != nullptr) {
        // Mirrors the tray icon's double-click handling in handleMessage()
        // below — reusing that path rather than inventing a second message
        // means a relaunch behaves identically to double-clicking the tray.
        PostMessageW(existing, kTrayCallbackMessage, 0, static_cast<LPARAM>(WM_LBUTTONDBLCLK));
    }
}

Application::~Application() {
    settingsWindow_.reset();
    monitorHosts_.clear();
    if (trayIcon_) {
        Shell_NotifyIconW(NIM_DELETE, &trayIcon_->data);
    }
    if (messageWindow_ != nullptr) {
        DestroyWindow(messageWindow_);
    }
}

bool Application::initialize(HINSTANCE instance) {
    instance_ = instance;

    WNDCLASSW messageClass{};
    messageClass.lpfnWndProc = &Application::staticWndProc;
    messageClass.hInstance = instance;
    messageClass.lpszClassName = kMessageWindowClassName;
    RegisterClassW(&messageClass);

    WNDCLASSW renderClass{};
    renderClass.lpfnWndProc = DefWindowProcW;
    renderClass.hInstance = instance;
    renderClass.lpszClassName = kRenderWindowClassName;
    RegisterClassW(&renderClass);

    // A real (if invisible, taskbar/Alt-Tab-excluded) top-level window,
    // *not* a message-only one (HWND_MESSAGE) — broadcast messages like
    // the registered "TaskbarCreated" below are only ever delivered to
    // top-level windows, never to message-only ones, so a message-only
    // window here would silently never learn that Explorer restarted
    // (see issue #27).
    messageWindow_ = CreateWindowExW(WS_EX_TOOLWINDOW, kMessageWindowClassName, L"Umbra", 0, 0, 0,
                                     0, 0, nullptr, nullptr, instance, this);
    if (messageWindow_ == nullptr) {
        return false;
    }

    if (!workerWHost_.ensureWorkerWSpawned()) {
        return false;
    }

    taskbarCreatedMessage_ = RegisterWindowMessageW(L"TaskbarCreated");

    addTrayIcon();

    if (settings_.launchOnStartup) {
        autostart_.enable();
    } else {
        autostart_.disable();
    }

    rebuildMonitorHosts();

    SetTimer(messageWindow_, kTickTimerId, kTickIntervalMs, nullptr);
    return true;
}

int Application::run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Application::staticWndProc(HWND window, UINT message, WPARAM wParam,
                                            LPARAM lParam) {
    Application* self = nullptr;
    if (message == WM_NCCREATE) {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<Application*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->handleMessage(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Application::handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    // A dynamically-registered message id can't be a switch case label, so
    // it's checked here first. Explorer broadcasts this to every top-level
    // window whenever it restarts (crash, manual kill, or otherwise) — the
    // WorkerW our render windows were parented into, and the tray icon
    // Explorer itself was hosting, are both gone by then (see issue #27).
    // Re-adding the tray icon and rebuilding every render window (which
    // re-attaches through WorkerWHost::attach()'s own self-healing) is the
    // standard, documented way to recover from this without requiring the
    // user to restart Umbra itself.
    if (taskbarCreatedMessage_ != 0 && message == taskbarCreatedMessage_) {
        addTrayIcon();
        rebuildMonitorHosts();
        return 0;
    }

    switch (message) {
        case WM_TIMER:
            if (wParam == kTickTimerId) {
                onTick();
            }
            return 0;

        case WM_DISPLAYCHANGE:
            onDisplayChange();
            return 0;

        case WM_SETTINGCHANGE:
            // Fired (among other things) when the user flips Windows'
            // light/dark theme — push it to the settings window live
            // rather than making the user close and reopen it to see it
            // follow, per ARCHITECTURE.md's "re-applied live if the user
            // switches theme while Umbra is open".
            if (settingsWindow_) {
                settingsWindow_->notifyThemeChanged(currentTheme());
            }
            return 0;

        case kTrayCallbackMessage: {
            const UINT mouseMessage = static_cast<UINT>(lParam);
            if (mouseMessage == WM_LBUTTONDBLCLK) {
                openSettingsWindow();
            } else if (mouseMessage == WM_RBUTTONUP || mouseMessage == WM_CONTEXTMENU) {
                POINT cursor{};
                GetCursorPos(&cursor);

                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING, kMenuOpenSettings, L"Open Settings");
                AppendMenuW(menu, MF_STRING | (manuallyPausedAll_ ? MF_CHECKED : 0),
                            kMenuTogglePause, manuallyPausedAll_ ? L"Resume" : L"Pause all");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, kMenuQuit, L"Quit");

                SetForegroundWindow(messageWindow_);
                TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, messageWindow_,
                               nullptr);
                DestroyMenu(menu);
            }
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kMenuOpenSettings:
                    openSettingsWindow();
                    break;
                case kMenuTogglePause:
                    setAllPaused(!manuallyPausedAll_);
                    break;
                case kMenuQuit:
                    quit();
                    break;
                default:
                    break;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wParam, lParam);
    }
}

void Application::onTick() {
    applyRenderPolicies();
    advancePlaylistRotations(kTickIntervalSeconds);

    for (auto& host : monitorHosts_) {
        if (host->webEngine != nullptr) {
            continue;  // WebView2 presents itself; nothing to drive here.
        }
        if (host->paused || host->engine == nullptr) {
            continue;
        }

        const RenderTickGate gate = evaluateRenderTickGate(host->sinceLastRenderSeconds,
                                                           kTickIntervalSeconds, host->fpsCap);
        if (!gate.shouldRender) {
            continue;
        }

        host->engine->advance(gate.elapsedSeconds);
        host->compositor->draw(host->engine->currentFrame(), host->engine->frameSize());
    }
}

void Application::syncLockScreenIfPrimary(const MonitorHost& host, WallpaperType type,
                                          const std::filesystem::path& contentDir) {
    if (host.monitor.isPrimary && settings_.syncLockScreen) {
        lockScreenSync_.syncFromContentFile(type, contentDir);
    }
}

void Application::syncLockScreenFromPrimaryAssignment() {
    const auto primary = std::find_if(monitorHosts_.begin(), monitorHosts_.end(),
                                      [](const auto& host) { return host->monitor.isPrimary; });
    if (primary == monitorHosts_.end() || (*primary)->profile == nullptr) {
        return;
    }

    // The primary's *active* content: whichever playlistPaths entry its
    // rotator is currently on, or the profile's own path for a
    // single-wallpaper assignment — mirrors
    // rebuildMonitorHostsFromCurrentMonitorList()'s own activeDir/activeType
    // logic (see its comment on why the type is re-detected rather than
    // trusted from the profile for a playlist entry).
    const WallpaperProfile& profile = *(*primary)->profile;
    if ((*primary)->playlistRotator != nullptr) {
        const std::filesystem::path activeDir((*primary)->playlistRotator->current());
        syncLockScreenIfPrimary(**primary, detectImportedFolderType(activeDir), activeDir);
    } else {
        syncLockScreenIfPrimary(**primary, profile.type, profile.path);
    }
}

void Application::applyRenderPolicies() {
    fullscreenWatcher_.refresh();
    powerWatcher_.refresh();

    for (auto& host : monitorHosts_) {
        if (host->profile == nullptr) {
            continue;
        }
        const RenderPolicy policy = computeRenderPolicy(
            fullscreenWatcher_.isFullscreenActive(), settings_.pauseOnFullscreen,
            powerWatcher_.currentAction(), host->profile->fpsCap, powerWatcher_.reducedFpsCap());

        host->fpsCap = policy.fpsCap;
        host->paused = policy.paused || manuallyPausedAll_;

        if (host->webEngine != nullptr && host->paused != host->webPauseApplied) {
            host->webEngine->setPaused(host->paused);
            host->webPauseApplied = host->paused;
        }
    }
}

void Application::advancePlaylistRotations(double elapsedSeconds) {
    for (auto& host : monitorHosts_) {
        if (host->playlistRotator == nullptr || host->profile == nullptr) {
            continue;
        }
        if (host->paused) {
            // Mirrors onTick()'s own render skip for a paused host (fullscreen
            // app, battery throttle, manual pause-all) — advancing the timer
            // here would be harmless, but swapping the engine below tears
            // down and reconstructs a RenderSurface/Compositor/VideoEngine
            // (GPU device + decoder init), exactly the work pausing exists to
            // avoid. The elapsed time is simply not counted while paused.
            continue;
        }

        host->sincePlaylistAdvanceSeconds += elapsedSeconds;
        const int intervalSeconds = host->profile->playlistIntervalSeconds;
        if (intervalSeconds <= 0 || host->sincePlaylistAdvanceSeconds < intervalSeconds) {
            continue;
        }
        // Subtracted rather than reset to 0 so a tick that runs long (e.g.
        // the process was suspended) doesn't perpetually re-arm the
        // interval from that moment instead of catching back up.
        host->sincePlaylistAdvanceSeconds -= intervalSeconds;

        const std::filesystem::path nextDir(host->playlistRotator->advance());
        const WallpaperType nextType = detectImportedFolderType(nextDir);
        const std::filesystem::path nextContentPath = resolveContentPath(nextDir, nextType);
        if (nextContentPath.empty()) {
            // A playlist entry's folder vanished out from under Settings —
            // keep showing whatever this monitor already has rather than
            // going blank; the rotator has still moved on, so the next
            // interval tries the entry after this one.
            continue;
        }

        try {
            host->webEngine.reset();
            host->engine.reset();
            host->compositor.reset();
            host->renderSurface.reset();
            host->webPauseApplied = false;
            createEngineForHost(*host, nextContentPath, nextType);
            syncLockScreenIfPrimary(*host, nextType, nextDir);
        } catch (const std::exception&) {
            // Same rationale as rebuildMonitorHostsFromCurrentMonitorList()'s
            // own catch — leave this monitor without an engine rather than
            // taking down the whole app.
        }
    }
}

void Application::onDisplayChange() {
    const MonitorChangeSet changes = monitorManager_.refresh();
    if (changes.isEmpty() && !monitorHosts_.empty()) {
        // Nothing actually changed (e.g. a spurious WM_DISPLAYCHANGE) — skip
        // the full rebuild below so it doesn't restart playback on every
        // monitor for no reason. Still runs if monitorHosts_ is empty (the
        // very first real layout after startup), even though refresh()
        // reporting "every monitor added" on its first call would already
        // make changes non-empty then too.
        return;
    }
    // refresh() already ran above (it had to, to compute changes) — don't
    // call rebuildMonitorHosts() and pay for a second, redundant
    // EnumDisplayMonitors.
    rebuildMonitorHostsFromCurrentMonitorList();
}

void Application::rebuildMonitorHosts() {
    monitorManager_.refresh();
    rebuildMonitorHostsFromCurrentMonitorList();
}

void Application::rebuildMonitorHostsFromCurrentMonitorList() {
    // A full rebuild on every call is simpler than
    // incrementally diffing which monitors/profiles actually changed, at
    // the cost of restarting playback on every still-connected monitor too
    // — an acceptable trade for how rarely this runs (startup, and monitor
    // connect/disconnect).
    monitorHosts_.clear();

    const auto assignments =
        assignProfilesToMonitors(monitorManager_.monitors(), settings_.profiles);
    for (const auto& assignment : assignments) {
        if (assignment.profile == nullptr) {
            continue;
        }

        auto host = std::make_unique<MonitorHost>();
        host->monitor = assignment.monitor;
        host->profile = assignment.profile;
        host->fpsCap = assignment.profile->fpsCap;

        host->window = CreateWindowExW(WS_EX_NOACTIVATE, kRenderWindowClassName, L"", WS_POPUP,
                                       assignment.monitor.x, assignment.monitor.y,
                                       assignment.monitor.width, assignment.monitor.height, nullptr,
                                       nullptr, GetModuleHandleW(nullptr), nullptr);
        if (host->window == nullptr) {
            continue;
        }
        if (!workerWHost_.attach(host->window, assignment.monitor.x, assignment.monitor.y,
                                 assignment.monitor.width, assignment.monitor.height)) {
            // Not parented behind the desktop icons — showing it anyway
            // would just be an ordinary top-level window covering the
            // screen, worse than not rendering at all.
            continue;
        }

        std::filesystem::path activeDir(assignment.profile->path);
        WallpaperType activeType = assignment.profile->type;
        if (assignment.profile->isPlaylist()) {
            // Seeded from the playlist's own content rather than
            // PlaylistRotator's default (deliberately fixed at 1 so tests
            // stay deterministic — see playlist.h) or a random one: a
            // fixed seed would reshuffle every playlist to the exact same
            // order, and a random one would make two monitors mirroring
            // the identical playlist while Settings.syncMonitors is on
            // (issue #71) shuffle independently and show different
            // wallpapers at the same time (issue #86) — deriving it from
            // content means identical playlists always shuffle identically,
            // with no coordination between monitors needed.
            const Playlist playlist{assignment.profile->playlistPaths,
                                    assignment.profile->playlistIntervalSeconds,
                                    assignment.profile->playlistMode};
            host->playlistRotator = std::make_unique<PlaylistRotator>(
                playlist, deterministicSeedForPlaylist(playlist));
            activeDir = host->playlistRotator->current();
            activeType = detectImportedFolderType(activeDir);
        }

        try {
            const std::filesystem::path contentPath = resolveContentPath(activeDir, activeType);
            if (contentPath.empty()) {
                continue;
            }
            createEngineForHost(*host, contentPath, activeType);
        } catch (const std::exception&) {
            // A corrupt/unreadable wallpaper file, or the folder vanishing
            // out from under us mid-scan, shouldn't take the whole app
            // down — leave this monitor without a host rather than
            // propagating the exception out of the message loop.
            continue;
        }

        ShowWindow(host->window, SW_SHOWNOACTIVATE);
        monitorHosts_.push_back(std::move(host));
    }

    syncLockScreenFromPrimaryAssignment();
}

void Application::openSettingsWindow() {
    if (!settingsWindow_) {
        // settings-ui/'s built bundle ships alongside the executable (see
        // the installer, #10) rather than under %LOCALAPPDATA% — it's
        // static content, not per-user state.
        const std::filesystem::path assetsDir = currentExecutableDirectory() / "settings-ui";
        settingsWindow_ = std::make_unique<SettingsWindow>(instance_, *this, assetsDir);
    }
    settingsWindow_->show();
}

std::string Application::currentTheme() { return readWindowsTheme(); }

void Application::persistSettings() {
    settings_.saveToFile(settingsPath_.string());

    if (settings_.launchOnStartup) {
        autostart_.enable();
    } else {
        autostart_.disable();
    }
    powerWatcher_.setConfig(PowerThrottleConfig{.pauseOnBattery = settings_.pauseOnBattery});

    // Only on an actual false-to-true transition of this one setting
    // (tracked via lockScreenSyncWasEnabled_) — not on every
    // persistSettings() call, which runs for any settings save at all
    // (e.g. toggling pauseOnBattery). Without that check, saving an
    // unrelated setting while this was already on would re-trigger a
    // spurious resync. Turning it on is what should make it take effect
    // immediately instead of waiting for the next monitor-host rebuild
    // (reassigning a wallpaper, a monitor hotplug, or an app restart).
    if (settings_.syncLockScreen && !lockScreenSyncWasEnabled_) {
        syncLockScreenFromPrimaryAssignment();
    }
    lockScreenSyncWasEnabled_ = settings_.syncLockScreen;
}

void Application::persistSettingsAndRebuildMonitorHosts() {
    persistSettings();
    rebuildMonitorHosts();
}

std::filesystem::path Application::pickImportSource(WallpaperType type) {
    return showImportPicker(type);
}

void Application::generateThumbnail(const std::string& title, WallpaperType type,
                                    const std::filesystem::path& contentDir) {
    const std::filesystem::path destination = libraryManager_.thumbnailPathForTitle(title);

    // Decoding a frame (Media Foundation especially, for Video) can take
    // anywhere from a few milliseconds to over a second depending on
    // codec/file — running it inline here would stall this process's one
    // message-loop thread, which is also what drives the render tick and
    // handles the WebView2 message that called this. Detached (not
    // joined) since the lambda only captures plain values/paths, not
    // anything tied to Application's own lifetime, so there's no
    // use-after-free risk in letting it keep running past this function
    // returning. Waiting briefly on doneFuture covers the common case —
    // most videos/images decode well under this — so importWallpaper's
    // own response can carry the thumbnail immediately; a slower decode
    // just finishes later and is picked up whenever getLibrary() is next
    // called (list() checks the file's existence fresh every time).
    //
    // Known, accepted gap: a rename/remove of this exact title racing
    // this thread's still-pending write can leave it writing under a
    // stale (or, in a very unlucky reuse, a since-recreated) path. The
    // worst case is a missing or wrong thumbnail for that one title,
    // self-corrected the next time it's regenerated — not a crash or
    // data-loss risk, so no cancellation/generation-id tracking here.
    auto done = std::make_shared<std::promise<void>>();
    std::future<void> doneFuture = done->get_future();
    std::thread([type, contentDir, destination, done]() {
        // This thread has never touched COM — main.cpp's CoInitializeEx
        // only covers the thread that called it, and every CoCreateInstance
        // inside ThumbnailGenerator (WIC) needs an apartment on *this*
        // thread specifically (same requirement lock_screen_sync.cpp's
        // background thread has, for the same reason).
        const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        if (comInitialized) {
            ThumbnailGenerator::generate(type, contentDir, destination);
            CoUninitialize();
        }
        done->set_value();
    }).detach();
    doneFuture.wait_for(std::chrono::milliseconds(250));
}

void Application::setAllPaused(bool paused) { manuallyPausedAll_ = paused; }

void Application::quit() { DestroyWindow(messageWindow_); }

void Application::addTrayIcon() {
    trayIcon_ = std::make_unique<TrayIconState>();
    trayIcon_->data.cbSize = sizeof(NOTIFYICONDATAW);
    trayIcon_->data.hWnd = messageWindow_;
    trayIcon_->data.uID = 1;
    trayIcon_->data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    trayIcon_->data.uCallbackMessage = kTrayCallbackMessage;
    // The same icon shown on the Settings window (see win32_text.h's
    // loadAppIcon, shared so the two can't silently drift apart).
    trayIcon_->data.hIcon = loadAppIcon(instance_);
    wcsncpy_s(trayIcon_->data.szTip, L"Umbra", _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &trayIcon_->data);
}

}  // namespace umbra
