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

#include <memory>
#include <string>

namespace umbra {

// Hosts a WebView2 control that renders an imported web wallpaper
// (index.html + assets, per the library/ import flow) directly as a child
// window of a monitor's render window.
//
// Deliberately does NOT implement IWallpaperEngine: WebView2's documented,
// well-supported hosting mode is an owned child HWND that it presents to
// directly (it already composites itself efficiently via DirectComposition
// internally) — capturing that into a shared D3D11 texture for
// Compositor::draw(), the way VideoEngine/ImageEngine do, requires
// WebView2's separate visual-hosting API (ICoreWebView2CompositionController
// bound to a DirectComposition surface) and adds real complexity for
// content that already renders correctly without it. Revisit only if a
// requirement emerges that needs every engine drawn through the same D3D11
// pipeline (e.g. a compositor-level effect applied uniformly).
//
// Windows-only; environment/controller creation is asynchronous per the
// WebView2 API and driven by the host window's message loop. Verified
// manually against a live desktop session (see TESTING.md).
class WebEngine {
   public:
    // parentWindow is the render window this control is hosted in; it must
    // outlive this WebEngine. Construction kicks off asynchronous
    // environment/controller creation and returns immediately — isReady()
    // reports when navigation to indexHtmlPath can actually be seen.
    WebEngine(HWND parentWindow, std::string indexHtmlPath);
    ~WebEngine();

    WebEngine(const WebEngine&) = delete;
    WebEngine& operator=(const WebEngine&) = delete;

    // Resizes the hosted control to match the render window (call on
    // WM_SIZE, mirroring RenderSurface::resize()).
    void setBounds(int width, int height);

    // Per the PRD's performance-management requirements: a paused web
    // wallpaper stops its render loop and script timers rather than just
    // going invisible, so it doesn't burn CPU/GPU while a fullscreen app
    // is in the foreground or the system is on battery saver.
    void setPaused(bool paused);

    bool isReady() const { return controller_ != nullptr; }

   private:
    void onEnvironmentCreated(ICoreWebView2Environment* environment);
    void onControllerCreated(ICoreWebView2Controller* controller);

    HWND parentWindow_;
    std::string indexHtmlPath_;
    int width_ = 0;
    int height_ = 0;
    bool paused_ = false;

    Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment_;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2> webView_;

    // Environment/controller creation completes asynchronously, well after
    // the constructor returns — held by the completion callbacks (as a
    // weak_ptr) so they can detect this WebEngine was destroyed in the
    // meantime instead of calling back into freed memory.
    std::shared_ptr<char> aliveToken_ = std::make_shared<char>();
};

}  // namespace umbra
