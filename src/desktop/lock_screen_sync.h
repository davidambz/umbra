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

#include "config/wallpaper_profile.h"
#include "desktop/lock_screen_api.h"

namespace umbra {

// Keeps the Windows lock screen (Win+L) showing a static snapshot of the
// current wallpaper instead of whatever background Windows already had
// set — it obviously can't animate there, but a still frame beats content
// that's completely unrelated to what's running behind the desktop icons.
// Decodes the frame straight from the wallpaper's own source file (the
// same convention ThumbnailGenerator already uses for library previews)
// rather than reading back whatever's currently in a RenderSurface's back
// buffer: that avoids needing the primary monitor's wallpaper to actually
// be rendering (paused for any reason — a fullscreen app, battery
// throttle — or mid-teardown/rebuild) or contending with another app for
// the GPU/display at all, so this fires immediately whenever the
// primary's assignment changes rather than waiting on render/pause state.
// Windows-only; the WIC/Media Foundation decode is verified manually
// against a live desktop session (see TESTING.md) — a real decoder can't
// be meaningfully faked in a unit test. ILockScreenApi exists as a seam
// mainly so Win32LockScreenApi's real WinRT broker call isn't hardwired
// in; there's no branching orchestration logic sitting on top of it (this
// class is a straight-line decode/encode/call sequence) worth a
// mock-based test independent of the decode path it's embedded in.
class LockScreenSync {
   public:
    // api must outlive this LockScreenSync. snapshotPath is where the
    // decoded frame is written before being handed to the lock screen
    // broker (e.g. %LOCALAPPDATA%\Umbra\lockscreen.png) — overwritten on
    // every sync, not kept per-wallpaper.
    LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath);

    // Blocks until any sync still in flight finishes — see syncThread_.
    ~LockScreenSync();

    // contentDir is the primary monitor's currently active wallpaper
    // folder (a profile's own path, or whichever playlistPaths entry is
    // currently rotated to) — the same convention LibraryManager/
    // ThumbnailGenerator use. type is silently a no-op for
    // WallpaperType::Web (no single frame to grab, same as
    // ThumbnailGenerator). Runs entirely on a background thread (decode,
    // encode, and the two blocking WinRT broker calls) since none of it
    // touches anything the caller's thread needs back — a sync already in
    // flight makes this a no-op rather than queuing up a second one, and
    // any failure along the way is swallowed (see ILockScreenApi's
    // contract), since this is a best-effort visual touch the app doesn't
    // depend on.
    void syncFromContentFile(WallpaperType type, std::filesystem::path contentDir);

   private:
    const ILockScreenApi& api_;
    std::filesystem::path snapshotPath_;
    std::atomic<bool> syncInProgress_{false};
    // Joined (not detached): the background thread reads api_/snapshotPath_
    // through `this`, so it must finish before this object — a member of
    // Application, destroyed on quit — goes away. The destructor blocks on
    // this join if a sync is still in flight when the app quits, bounded
    // by how long one decode+encode+broker round trip takes (typically
    // well under a second) rather than risking a use-after-free on
    // shutdown.
    std::thread syncThread_;
};

}  // namespace umbra
