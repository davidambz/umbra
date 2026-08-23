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

#include <filesystem>

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
// meaningfully faked in a unit test.
class LockScreenSync {
   public:
    // api must outlive this LockScreenSync. snapshotPath is where the
    // captured frame is written before being handed to the lock screen
    // broker (e.g. %LOCALAPPDATA%\Umbra\lockscreen.png) — overwritten on
    // every sync, not kept per-wallpaper.
    LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath);

    // Captures surface's current back buffer, writes it to snapshotPath as
    // a PNG, and hands that file to the lock screen broker. Any failure
    // along the way (capture, encode, or the broker call itself) is
    // swallowed — see ILockScreenApi's contract — since this is a
    // best-effort visual touch the app doesn't depend on.
    void syncFromSurface(RenderSurface& surface) const;

   private:
    const ILockScreenApi& api_;
    std::filesystem::path snapshotPath_;
};

}  // namespace umbra
