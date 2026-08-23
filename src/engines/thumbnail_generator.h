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

#include "config/wallpaper_profile.h"

namespace umbra {

// Generates a downscaled preview image for a just-imported wallpaper —
// see IUiBridgeHost::generateThumbnail (ui_bridge.h), which this backs on
// Windows. Video: grabs the first decoded frame via a short-lived Media
// Foundation source reader. Image: decodes frame 0 via WIC (the same
// approach as ImageEngine, minus GIF/APNG's multi-frame canvas
// compositing — a single representative frame is enough for a preview).
// Web has no single frame to grab and is silently skipped. Windows-only;
// verified manually against a live desktop session (see TESTING.md), the
// same as VideoEngine/ImageEngine's own Media Foundation/WIC decode paths
// — there's no meaningful way to unit-test either without a real decoder.
class ThumbnailGenerator {
   public:
    // contentDir is the wallpaper's imported directory (LibraryManager::
    // pathForTitle) — the actual video.<ext>/image.<ext> file inside it is
    // resolved here rather than passed in, mirroring application.cpp's
    // resolveContentPath(). Writes a PNG to destination on success. Any
    // failure (missing/corrupt content file, decode error, encode error)
    // leaves destination untouched rather than throwing — this is a
    // best-effort preview, not something the rest of the app depends on.
    static void generate(WallpaperType type, const std::filesystem::path& contentDir,
                         const std::filesystem::path& destination);
};

}  // namespace umbra
