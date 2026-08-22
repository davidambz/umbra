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

#include <WebView2.h>
#include <windows.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <filesystem>
#include <memory>
#include <string>

#include "ui/ui_bridge.h"

namespace umbra {

// The native Win32 host window + embedded WebView2 control for
// settings-ui/'s built front-end, per ARCHITECTURE.md's "Settings window
// is a native Win32 host window with a WebView2 control filling it."
// Wires WebView2's postMessage/WebMessageReceived plumbing to UiBridge —
// see settings-ui/src/bridge/uiBridge.ts for the JS side of the protocol
// this implements. Windows-only, verified manually against a live
// desktop session (see TESTING.md); UiBridge's own request-handling logic
// is what's unit-tested.
class SettingsWindow {
   public:
    // host must outlive this SettingsWindow. assetsDir is the folder
    // containing settings-ui's built index.html (see main.cpp).
    SettingsWindow(HINSTANCE instance, IUiBridgeHost& host, std::filesystem::path assetsDir);
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    // Creates the window/WebView2 on first call; brings it to the
    // foreground on every call.
    void show();

    // Pushes the themeChanged event to the page, if a controller exists
    // yet (a no-op otherwise — the page will just ask via getTheme() once
    // it does).
    void notifyThemeChanged(const std::string& theme);

   private:
    static LRESULT CALLBACK staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    void ensureWindowCreated();
    void onEnvironmentCreated(ICoreWebView2Environment* environment);
    void onControllerCreated(ICoreWebView2Controller* controller);
    void onWebMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* args);

    HINSTANCE instance_;
    std::filesystem::path assetsDir_;
    UiBridge bridge_;

    HWND window_ = nullptr;
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment_;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2> webView_;

    // See web_engine.h's identical use of this pattern: WebView2's
    // environment/controller creation completes asynchronously, well
    // after these callbacks are registered, so they need a way to detect
    // this SettingsWindow was destroyed in the meantime.
    std::shared_ptr<char> aliveToken_ = std::make_shared<char>();
};

}  // namespace umbra
