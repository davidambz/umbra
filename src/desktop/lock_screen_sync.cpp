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

#include "desktop/lock_screen_sync.h"

#include <objbase.h>

#include "engines/thumbnail_generator.h"

namespace umbra {

LockScreenSync::LockScreenSync(const ILockScreenApi& api, std::filesystem::path snapshotPath)
    : api_(api), snapshotPath_(std::move(snapshotPath)) {}

LockScreenSync::~LockScreenSync() {
    if (syncThread_.joinable()) {
        syncThread_.join();
    }
}

void LockScreenSync::syncFromContentFile(WallpaperType type, std::filesystem::path contentDir) {
    if (type == WallpaperType::Web) {
        // No single frame to grab (a Web wallpaper is a live page, not a
        // decodable file) — same as ThumbnailGenerator::generate()'s own
        // silent skip, checked here too rather than only inside the
        // thread below so this doesn't bother spinning one up at all.
        return;
    }

    bool expected = false;
    if (!syncInProgress_.compare_exchange_strong(expected, true)) {
        return;  // a previous sync's decode/encode/broker call hasn't finished yet
    }

    // syncInProgress_ having let us past the compare_exchange above means
    // any previous syncThread_ has already finished its work — this join
    // just reclaims its OS thread handle (a prerequisite for the
    // reassignment below), not an extra wait.
    if (syncThread_.joinable()) {
        syncThread_.join();
    }

    syncThread_ = std::thread([this, type, contentDir = std::move(contentDir)]() {
        // ThumbnailGenerator's WIC/Media Foundation decode needs an
        // apartment on *this* thread specifically (same requirement
        // application.cpp's generateThumbnail() background thread has,
        // for the same reason) — Multithreaded rather than
        // winrt::init_apartment() (which the broker call below also
        // needs) since there's no window/message queue here to keep
        // single-threaded-affine, and blocking on .get() needs no message
        // pumping outside an STA anyway.
        const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        if (comInitialized) {
            // Best-effort: leaves snapshotPath_ untouched (whatever it
            // last held) on any decode/encode failure — see
            // ThumbnailGenerator::generate()'s own contract. Calling
            // setLockScreenImage() unconditionally after it is still
            // correct either way: on success it's the freshly decoded
            // frame, on failure it's just re-applying whatever was
            // already there.
            ThumbnailGenerator::generate(type, contentDir, snapshotPath_);
            api_.setLockScreenImage(snapshotPath_.wstring());
            CoUninitialize();
        }
        syncInProgress_.store(false);
    });
}

}  // namespace umbra
