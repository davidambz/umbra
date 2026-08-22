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

#include "engines/web_engine.h"

#include <filesystem>

#include "engines/win32_text.h"

namespace umbra {

namespace {

// A wallpaper's own imported folder (per library/'s import flow) is the
// natural home for its WebView2 user data too, so each wallpaper's browser
// state (cache, localStorage) stays isolated from the others instead of
// every WebEngine sharing one global profile.
std::wstring userDataFolderFor(const std::string& indexHtmlPath) {
    const std::filesystem::path wallpaperFolder =
        std::filesystem::path(indexHtmlPath).parent_path() / L"webview2_data";
    return wallpaperFolder.wstring();
}

}  // namespace

WebEngine::WebEngine(HWND parentWindow, std::string indexHtmlPath)
    : parentWindow_(parentWindow), indexHtmlPath_(std::move(indexHtmlPath)) {
    const std::wstring userDataFolder = userDataFolderFor(indexHtmlPath_);
    const std::weak_ptr<char> weakAlive = aliveToken_;
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, weakAlive](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (weakAlive.expired()) {
                    // This WebEngine was destroyed before the async
                    // environment creation completed — don't touch it.
                    return S_OK;
                }
                if (SUCCEEDED(result)) {
                    onEnvironmentCreated(environment);
                }
                return S_OK;
            })
            .Get());
}

WebEngine::~WebEngine() {
    if (controller_) {
        controller_->Close();
    }
}

void WebEngine::onEnvironmentCreated(ICoreWebView2Environment* environment) {
    environment_ = environment;
    const std::weak_ptr<char> weakAlive = aliveToken_;
    environment_->CreateCoreWebView2Controller(
        parentWindow_,
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

void WebEngine::onControllerCreated(ICoreWebView2Controller* controller) {
    controller_ = controller;
    controller_->get_CoreWebView2(&webView_);

    RECT bounds{0, 0, width_, height_};
    controller_->put_Bounds(bounds);
    controller_->put_IsVisible(TRUE);

    // A plain file:// Navigate() only reliably works for a wallpaper
    // that's a single self-contained index.html: this Chromium version
    // enforces CORS on file:// origins, so any separate CSS/JS file the
    // page references via a relative URL fails to load
    // (net::ERR_FAILED), silently — see settings_window.cpp's
    // onControllerCreated() for the same bug found there. Mapping a
    // virtual https://-origin hostname to the wallpaper's own folder
    // avoids file:// entirely, so imported wallpapers with a normal
    // multi-file structure work the same as a single-file one.
    Microsoft::WRL::ComPtr<ICoreWebView2_3> webView3;
    if (SUCCEEDED(webView_.As(&webView3))) {
        const std::filesystem::path folder = std::filesystem::path(indexHtmlPath_).parent_path();
        webView3->SetVirtualHostNameToFolderMapping(L"umbra-wallpaper.internal", folder.c_str(),
                                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        webView_->Navigate(L"https://umbra-wallpaper.internal/index.html");
    } else {
        webView_->Navigate(toFileUrl(indexHtmlPath_).c_str());
    }

    if (paused_) {
        setPaused(true);
    }
}

void WebEngine::setBounds(int width, int height) {
    width_ = width;
    height_ = height;
    if (controller_) {
        RECT bounds{0, 0, width_, height_};
        controller_->put_Bounds(bounds);
    }
}

void WebEngine::setPaused(bool paused) {
    paused_ = paused;
    if (!webView_) {
        return;
    }
    Microsoft::WRL::ComPtr<ICoreWebView2_3> webView3;
    if (SUCCEEDED(webView_.As(&webView3))) {
        if (paused) {
            webView3->TrySuspend(nullptr);
        } else {
            webView3->Resume();
        }
    }
    if (controller_) {
        controller_->put_IsVisible(!paused);
    }
}

}  // namespace umbra
