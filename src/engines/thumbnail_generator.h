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

#include <cstdint>
#include <filesystem>

#include "config/wallpaper_profile.h"

namespace umbra {

// Generates a preview image decoded from a wallpaper's own content file —
// see IUiBridgeHost::generateThumbnail (ui_bridge.h), which this backs on
// Windows for the small Library/MonitorCard previews, and
// LockScreenSync::syncFromContentFile (desktop/lock_screen_sync.*), which
// wants a full-quality frame instead. Video: grabs the first decoded frame
// via a short-lived Media Foundation source reader. Image: decodes frame 0
// via WIC (the same approach as ImageEngine, minus GIF/APNG's multi-frame
// canvas compositing — a single representative frame is enough for a
// preview). Web has no single frame to grab and is silently skipped.
// Windows-only; verified manually against a live desktop session (see
// TESTING.md), the same as VideoEngine/ImageEngine's own Media
// Foundation/WIC decode paths — there's no meaningful way to unit-test
// either without a real decoder.
class ThumbnailGenerator {
   public:
    // The longer side a *thumbnail* (as opposed to a lock-screen sync) is
    // downscaled to — plenty for the small preview tiles MonitorCard/
    // WallpaperCard render, without bloating the base64 data: URL
    // ui_bridge.cpp embeds it as. Exposed so callers that want a
    // differently-sized (or, per kNoDownscaleLimit below, un-downscaled)
    // decode can say so explicitly rather than this being a silent,
    // unnamed default.
    static constexpr std::uint32_t kThumbnailMaxDimension = 480;

    // Passed as maxDimension to keep the frame at its native resolution —
    // for a lock screen sync, which (unlike a small UI thumbnail) is shown
    // close to full-monitor size, downscaling to kThumbnailMaxDimension
    // would visibly blur it.
    static constexpr std::uint32_t kNoDownscaleLimit = 0;

    // contentDir is the wallpaper's imported directory (LibraryManager::
    // pathForTitle) — the actual video.<ext>/image.<ext> file inside it is
    // resolved here rather than passed in, mirroring application.cpp's
    // resolveContentPath(). Writes a PNG to destination on success, scaled
    // down to fit within maxDimension on its longer side (kNoDownscaleLimit
    // to keep native resolution; a no-op if the source is already smaller
    // than maxDimension either way). Any failure (missing/corrupt content
    // file, decode error, encode error) leaves destination untouched
    // rather than throwing — this is a best-effort preview, not something
    // the rest of the app depends on.
    static void generate(WallpaperType type, const std::filesystem::path& contentDir,
                         const std::filesystem::path& destination,
                         std::uint32_t maxDimension = kThumbnailMaxDimension);
};

}  // namespace umbra
