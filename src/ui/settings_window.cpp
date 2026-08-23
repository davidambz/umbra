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

#include "ui/settings_window.h"

#include <dwmapi.h>

#include "engines/win32_text.h"

namespace umbra {

namespace {

constexpr wchar_t kWindowClassName[] = L"UmbraSettingsWindow";

// Only defined in Windows 11 SDK headers; the vcpkg-pinned SDK on some CI
// images predates it. The value itself has been stable since Windows 10
// 2004 (build 19041) — defining it manually is the standard workaround.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Matches the WebView2 content's own dark/light switch (tokens.css, driven
// by useSystemTheme.ts) so the title bar doesn't stay stuck white while
// everything below it follows the Windows theme. A no-op — not an error —
// on Windows versions that predate DWMWA_USE_IMMERSIVE_DARK_MODE.
void applyTitleBarTheme(HWND window, const std::string& theme) {
    const BOOL useDarkMode = theme == "dark" ? TRUE : FALSE;
    DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
}

// Defines window.umbra before settings-ui/'s bundle runs (this script is
// injected via AddScriptToExecuteOnDocumentCreatedAsync, which — unlike a
// <script> tag in the page itself — is guaranteed to run first on every
// navigation). Implements settings-ui/src/bridge/uiBridge.ts's UiBridge
// interface as a postMessage request/response client matching
// UiBridge::handleRequest's protocol on the native side.
constexpr char kBridgeShimScript[] = R"(
(function () {
    let nextId = 1;
    const pending = new Map();
    const themeListeners = new Set();

    window.chrome.webview.addEventListener('message', (event) => {
        const data = event.data;
        if (data && data.event === 'themeChanged') {
            themeListeners.forEach((listener) => listener(data.payload));
            return;
        }
        const entry = pending.get(data.id);
        if (!entry) return;
        pending.delete(data.id);
        if ('error' in data) {
            entry.reject(new Error(data.error));
        } else {
            entry.resolve(data.result);
        }
    });

    function call(method, params) {
        return new Promise((resolve, reject) => {
            const id = nextId++;
            pending.set(id, { resolve, reject });
            window.chrome.webview.postMessage({ id, method, params: params || {} });
        });
    }

    window.umbra = {
        getMonitors: () => call('getMonitors'),
        getLibrary: () => call('getLibrary'),
        getAssignment: (monitorId) => call('getAssignment', { monitorId }),
        getSettings: () => call('getSettings'),
        getTheme: () => call('getTheme'),
        onThemeChange: (callback) => {
            themeListeners.add(callback);
            return () => themeListeners.delete(callback);
        },
        assignSingle: (monitorId, wallpaperId, fpsCap) =>
            call('assignSingle', { monitorId, wallpaperId, fpsCap }),
        assignPlaylist: (monitorId, playlist, fpsCap) =>
            call('assignPlaylist', { monitorId, playlist, fpsCap }),
        clearAssignment: (monitorId) => call('clearAssignment', { monitorId }),
        importWallpaper: (title, type) => call('importWallpaper', { title, type }),
        renameWallpaper: (id, newTitle) => call('renameWallpaper', { id, newTitle }),
        removeWallpaper: (id) => call('removeWallpaper', { id }),
        updateSettings: (patch) => call('updateSettings', patch),
    };
})();
)";

}  // namespace

SettingsWindow::SettingsWindow(HINSTANCE instance, IUiBridgeHost& host,
                               std::filesystem::path assetsDir)
    : instance_(instance), assetsDir_(std::move(assetsDir)), host_(host), bridge_(host) {}

SettingsWindow::~SettingsWindow() {
    if (controller_) {
        controller_->Close();
    }
    if (window_ != nullptr) {
        DestroyWindow(window_);
    }
}

void SettingsWindow::ensureWindowCreated() {
    if (window_ != nullptr) {
        return;
    }

    static bool classRegistered = false;
    if (!classRegistered) {
        // Same icon as the tray icon (see win32_text.h's loadAppIcon) —
        // without this the window falls back to a generic default in the
        // taskbar and Alt-Tab.
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = &SettingsWindow::staticWndProc;
        windowClass.hInstance = instance_;
        windowClass.lpszClassName = kWindowClassName;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hIcon = loadAppIcon(instance_);
        RegisterClassW(&windowClass);
        classRegistered = true;
    }

    window_ = CreateWindowExW(0, kWindowClassName, L"Umbra Settings",
                              WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
                              960, 640, nullptr, nullptr, instance_, this);
    if (window_ != nullptr) {
        applyTitleBarTheme(window_, host_.currentTheme());
    }

    const std::weak_ptr<char> weakAlive = aliveToken_;
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, weakAlive](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (weakAlive.expired()) {
                    return S_OK;
                }
                if (SUCCEEDED(result)) {
                    onEnvironmentCreated(environment);
                }
                return S_OK;
            })
            .Get());
}

void SettingsWindow::onEnvironmentCreated(ICoreWebView2Environment* environment) {
    environment_ = environment;
    const std::weak_ptr<char> weakAlive = aliveToken_;
    environment_->CreateCoreWebView2Controller(
        window_,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this, weakAlive](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                if (weakAlive.expired()) {
                    return S_OK;
                }
                if (SUCCEEDED(result)) {
                    onControllerCreated(controller);
                }
                return S_OK;
            })
            .Get());
}

void SettingsWindow::onControllerCreated(ICoreWebView2Controller* controller) {
    controller_ = controller;
    controller_->get_CoreWebView2(&webView_);

    RECT bounds{};
    GetClientRect(window_, &bounds);
    controller_->put_Bounds(bounds);

    const std::weak_ptr<char> weakAlive = aliveToken_;
    webView_->AddScriptToExecuteOnDocumentCreated(
        utf8ToWide(kBridgeShimScript).c_str(),
        Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
            [](HRESULT, LPCWSTR) -> HRESULT { return S_OK; })
            .Get());

    EventRegistrationToken token{};
    webView_->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this, weakAlive](ICoreWebView2*,
                              ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                if (!weakAlive.expired()) {
                    onWebMessageReceived(args);
                }
                return S_OK;
            })
            .Get(),
        &token);

    // See navigateToLocalFolder() (win32_text.h) for why this isn't a
    // plain file:// Navigate() to assetsDir_ / "index.html" — issue #31.
    navigateToLocalFolder(webView_.Get(), assetsDir_, L"umbra-settings-ui.internal");
}

void SettingsWindow::onWebMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* args) {
    LPWSTR rawJson = nullptr;
    if (FAILED(args->get_WebMessageAsJson(&rawJson)) || rawJson == nullptr) {
        return;
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, rawJson, -1, nullptr, 0, nullptr, nullptr);
    std::string requestJson(static_cast<size_t>(length > 0 ? length - 1 : 0), '\0');
    if (length > 0) {
        WideCharToMultiByte(CP_UTF8, 0, rawJson, -1, requestJson.data(), length, nullptr, nullptr);
    }
    CoTaskMemFree(rawJson);

    const std::string responseJson = bridge_.handleRequest(requestJson);
    if (webView_) {
        webView_->PostWebMessageAsJson(utf8ToWide(responseJson).c_str());
    }
}

void SettingsWindow::notifyThemeChanged(const std::string& theme) {
    if (window_ != nullptr) {
        applyTitleBarTheme(window_, theme);
    }
    if (!webView_) {
        return;
    }
    webView_->PostWebMessageAsJson(utf8ToWide(UiBridge::buildThemeChangedEvent(theme)).c_str());
}

void SettingsWindow::show() {
    ensureWindowCreated();
    ShowWindow(window_, SW_SHOW);
    SetForegroundWindow(window_);
}

LRESULT CALLBACK SettingsWindow::staticWndProc(HWND window, UINT message, WPARAM wParam,
                                               LPARAM lParam) {
    SettingsWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<SettingsWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->handleMessage(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT SettingsWindow::handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE:
            // Hide rather than destroy — WebView2's startup cost is real
            // enough that reopening the settings window from the tray
            // should be instant, not a fresh cold-start every time.
            ShowWindow(window, SW_HIDE);
            return 0;

        case WM_SIZE: {
            if (controller_) {
                RECT bounds{};
                GetClientRect(window, &bounds);
                controller_->put_Bounds(bounds);
            }
            return 0;
        }

        default:
            return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace umbra
