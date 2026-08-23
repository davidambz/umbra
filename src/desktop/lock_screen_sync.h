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

#include <atomic>
#include <filesystem>
#include <thread>

#include "desktop/lock_screen_api.h"
#include "render/render_surface.h"

namespace umbra {

// Keeps the Windows lock screen (Win+L) showing a static snapshot of the
// current wallpaper instead of whatever background Windows already had
// set — it obviously can't animate there, but a still frame beats content
// that's completely unrelated to what's running behind the desktop icons.
// Windows-only; the D3D11 readback and WIC encode are verified manually
// against a live desktop session (see TESTING.md), the same as
// RenderSurface/Compositor's own GPU work — a real D3D11 device can't be
// meaningfully faked in a unit test. ILockScreenApi exists as a seam
// mainly so Win32LockScreenApi's real WinRT broker call isn't hardwired
// in; there's no branching orchestration logic sitting on top of it (this
// class is a straight-line capture/encode/call sequence) worth a
// mock-based test independent of the GPU-bound path it's embedded in.
class LockScreenSync {
   public:
    // api must outlive this LockScreenSync. snapshotPath is where the
    // captured frame is written before being handed to the lock screen
    // broker (e.g. %LOCALAPPDATA%\Umbra\lockscreen.png) — overwritten on
    // every sync, not kept per-wallpaper.
    LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath);

    // Blocks until any sync still in flight finishes — see syncThread_.
    ~LockScreenSync();

    // Copies surface's current back buffer into a plain pixel buffer (the
    // only part that touches the D3D11 device/context, so it must run on
    // the caller's thread rather than a background one; this GPU
    // readback's inherent CPU/GPU sync stall is accepted here since a
    // sync only fires once per monitor-host rebuild — startup, monitor
    // hotplug, assignment change — not every tick) and hands that buffer
    // off to a background thread for the slow part: PNG encoding and the
    // two blocking WinRT broker calls. A sync already in flight makes this
    // a no-op rather than queuing up a second one. Any failure along the
    // way (capture, encode, or the broker call itself) is swallowed — see
    // ILockScreenApi's contract — since this is a best-effort visual touch
    // the app doesn't depend on.
    void syncFromSurface(RenderSurface& surface);

   private:
    const ILockScreenApi& api_;
    std::filesystem::path snapshotPath_;
    std::atomic<bool> syncInProgress_{false};
    // Joined (not detached): the background thread reads api_/snapshotPath_
    // through `this`, so it must finish before this object — a member of
    // Application, destroyed on quit — goes away. The destructor blocks on
    // this join if a sync is still in flight when the app quits, bounded
    // by how long one encode+broker round trip takes (typically well under
    // a second) rather than risking a use-after-free on shutdown.
    std::thread syncThread_;
};

}  // namespace umbra
