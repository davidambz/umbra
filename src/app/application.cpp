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

#include <cmath>

#include "app/monitor_assignment.h"
#include "app/render_policy.h"
#include "engines/image_engine.h"
#include "engines/video_engine.h"
#include "engines/wallpaper_engine.h"
#include "engines/web_engine.h"
#include "engines/win32_text.h"
#include "render/compositor.h"
#include "render/render_surface.h"
#include "resource.h"
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

// Resolves a WallpaperProfile's actual content file within its imported
// folder (see library_manager.cpp's import()): "video.<ext>"/"image.<ext>"
// for Video/Image, "index.html" for Web. Returns an empty path if the
// expected file isn't there (e.g. the folder was deleted out from under
// Settings).
std::filesystem::path resolveContentPath(const WallpaperProfile& profile) {
    const std::filesystem::path dir(profile.path);
    if (profile.type == WallpaperType::Web) {
        const std::filesystem::path indexHtml = dir / "index.html";
        std::error_code ec;
        return std::filesystem::exists(indexHtml, ec) ? indexHtml : std::filesystem::path{};
    }

    const std::string stem = profile.type == WallpaperType::Video ? "video" : "image";
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
std::wstring currentExecutableCommand() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return L"";
    }
    return L"\"" + std::wstring(buffer, length) + L"\"";
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
std::string showImportPicker(WallpaperType type) {
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
        const COMDLG_FILTERSPEC filters[] = {{L"Image files", L"*.gif;*.apng"}};
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
    const std::wstring widePath(rawPath);
    CoTaskMemFree(rawPath);

    const int length =
        WideCharToMultiByte(CP_UTF8, 0, widePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string path(static_cast<size_t>(length > 0 ? length - 1 : 0), '\0');
    if (length > 0) {
        WideCharToMultiByte(CP_UTF8, 0, widePath.c_str(), -1, path.data(), length, nullptr,
                            nullptr);
    }
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
};

struct Application::TrayIconState {
    NOTIFYICONDATAW data{};
};

Application::Application(std::filesystem::path settingsPath, std::filesystem::path storageRoot)
    : settingsPath_(std::move(settingsPath)),
      settings_(Settings::loadFromFile(settingsPath_.string())),
      libraryManager_(std::move(storageRoot)),
      workerWHost_(workerWApi_),
      monitorManager_(monitorEnumerator_),
      fullscreenWatcher_(fullscreenApi_),
      powerWatcher_(powerApi_, PowerThrottleConfig{.pauseOnBattery = settings_.pauseOnBattery}),
      autostart_(registryApi_, currentExecutableCommand()) {}

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

    messageWindow_ = CreateWindowExW(0, kMessageWindowClassName, L"Umbra", 0, 0, 0, 0, 0,
                                     HWND_MESSAGE, nullptr, instance, this);
    if (messageWindow_ == nullptr) {
        return false;
    }

    if (!workerWHost_.ensureWorkerWSpawned()) {
        return false;
    }

    trayIcon_ = std::make_unique<TrayIconState>();
    trayIcon_->data.cbSize = sizeof(NOTIFYICONDATAW);
    trayIcon_->data.hWnd = messageWindow_;
    trayIcon_->data.uID = 1;
    trayIcon_->data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    trayIcon_->data.uCallbackMessage = kTrayCallbackMessage;
    // The same violet mark as umbra.exe's own resource icon (see
    // resources/umbra.rc) — falls back to the generic system icon only if
    // that resource is somehow missing (e.g. a dev build without it linked).
    trayIcon_->data.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (trayIcon_->data.hIcon == nullptr) {
        trayIcon_->data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wcsncpy_s(trayIcon_->data.szTip, L"Umbra", _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &trayIcon_->data);

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

    for (auto& host : monitorHosts_) {
        if (host->webEngine != nullptr) {
            continue;  // WebView2 presents itself; nothing to drive here.
        }
        if (host->paused || host->engine == nullptr) {
            continue;
        }

        host->sinceLastRenderSeconds += kTickIntervalSeconds;
        const double frameIntervalSeconds =
            host->fpsCap > 0 ? 1.0 / static_cast<double>(host->fpsCap) : 0.0;
        if (frameIntervalSeconds > 0.0 && host->sinceLastRenderSeconds < frameIntervalSeconds) {
            continue;
        }

        // Advance by the real elapsed time since the last advance(), not
        // just this one tick's fixed interval — this host may have skipped
        // several ticks above waiting for frameIntervalSeconds to elapse
        // (e.g. under a reduced fps cap), and shortchanging advance() would
        // make video/gif playback run in slow motion. The remainder carries
        // over instead of resetting to 0 so pacing doesn't drift.
        const double elapsedSeconds = host->sinceLastRenderSeconds;
        host->sinceLastRenderSeconds =
            frameIntervalSeconds > 0.0 ? std::fmod(elapsedSeconds, frameIntervalSeconds) : 0.0;

        host->engine->advance(elapsedSeconds);
        host->compositor->draw(host->engine->currentFrame(), host->engine->frameSize());
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
        if (!workerWHost_.attach(host->window)) {
            // Not parented behind the desktop icons — showing it anyway
            // would just be an ordinary top-level window covering the
            // screen, worse than not rendering at all.
            continue;
        }

        try {
            const std::filesystem::path contentPath = resolveContentPath(*assignment.profile);
            if (contentPath.empty()) {
                continue;
            }

            if (assignment.profile->type == WallpaperType::Web) {
                host->webEngine = std::make_unique<WebEngine>(host->window, contentPath.string());
                host->webEngine->setBounds(assignment.monitor.width, assignment.monitor.height);
            } else {
                host->renderSurface = std::make_unique<RenderSurface>(
                    host->window, assignment.monitor.width, assignment.monitor.height);
                host->compositor = std::make_unique<Compositor>(*host->renderSurface);
                if (assignment.profile->type == WallpaperType::Video) {
                    host->engine = std::make_unique<VideoEngine>(host->renderSurface->device(),
                                                                 contentPath.string(),
                                                                 assignment.profile->fpsCap);
                } else {
                    host->engine = std::make_unique<ImageEngine>(host->renderSurface->device(),
                                                                 contentPath.string());
                }
            }
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
}

void Application::persistSettingsAndRebuildMonitorHosts() {
    persistSettings();
    rebuildMonitorHosts();
}

std::string Application::pickImportSource(WallpaperType type) { return showImportPicker(type); }

void Application::setAllPaused(bool paused) { manuallyPausedAll_ = paused; }

void Application::quit() { DestroyWindow(messageWindow_); }

}  // namespace umbra
