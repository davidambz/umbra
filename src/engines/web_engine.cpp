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

namespace umbra {

namespace {

std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), wide.data(),
                        length);
    return wide;
}

std::wstring toFileUrl(const std::string& path) { return L"file:///" + toWide(path); }

}  // namespace

WebEngine::WebEngine(HWND parentWindow, std::string indexHtmlPath)
    : parentWindow_(parentWindow), indexHtmlPath_(std::move(indexHtmlPath)) {
    // A wallpaper's own imported folder (per library/'s import flow) is
    // used as the WebView2 user data folder too, so each wallpaper's
    // browser state (cache, localStorage) stays isolated from the others
    // instead of sharing one global profile.
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
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
    environment_->CreateCoreWebView2Controller(
        parentWindow_,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
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

    webView_->Navigate(toFileUrl(indexHtmlPath_).c_str());

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
