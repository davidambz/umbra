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

    // See navigateToLocalFolder() (win32_text.h) for why this isn't a
    // plain file:// Navigate() to indexHtmlPath_ — issue #31.
    navigateToLocalFolder(webView_.Get(), std::filesystem::path(indexHtmlPath_).parent_path(),
                          L"umbra-wallpaper.internal");

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
