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
#include <string>

#include "config/wallpaper_profile.h"

namespace umbra {

enum class ImportError {
    None,
    TitleEmpty,
    InvalidTitle,
    SourceNotFound,
    UnsupportedFileType,
    WebMissingIndexHtml,
    DestinationAlreadyExists,
    CopyFailed,
};

struct ImportResult {
    bool success = false;
    ImportError error = ImportError::None;
    WallpaperProfile profile;
};

// One already-imported entry, as discovered by LibraryManager::list()
// rather than remembered from an import() call — so it reflects the
// library's actual on-disk state even across app restarts, since nothing
// currently persists "the full set of imported titles" anywhere else
// (Settings::profiles only records what's *assigned* to a monitor).
struct LibraryEntry {
    std::string title;
    WallpaperType type;
    std::filesystem::path path;
    // thumbnailPathForTitle()'s convention path, but only if that file
    // actually exists — empty otherwise (no thumbnail generated yet, or
    // the type doesn't support one). Populated by list() via a plain
    // filesystem check; the file itself is written by the Windows-only
    // ThumbnailGenerator, not by anything in this class.
    std::filesystem::path thumbnailPath;
};

// Detects a wallpaper's type from its source path: a video/image file is
// classified by extension, while a directory or .zip is always Web (a web
// wallpaper is a project folder, not a single file).
WallpaperType detectWallpaperType(const std::filesystem::path& sourcePath);

// Detects an *already-imported* wallpaper folder's type by inspecting its
// contents: an index.html at its root means Web, otherwise the
// video.<ext>/image.<ext> stem convention import() writes (defaulting to
// Video if neither is found, which shouldn't happen for a folder import()
// actually produced). Unlike detectWallpaperType() above — which classifies
// a raw *source* path (always a directory for Web) before import happens —
// this is for re-detecting the type of a folder that's already one of
// LibraryManager's own entries, since a playlist's rotation entries don't
// persist their own WallpaperType (see WallpaperProfile::playlistPaths).
WallpaperType detectImportedFolderType(const std::filesystem::path& importedDir);

// Manages Umbra's internal wallpaper storage: importing user-selected
// content (a single video/image file, or a folder/zip for a Web project)
// into its own per-title directory, and renaming/removing entries.
//
// Pure filesystem logic, no Win32 dependency — part of the hexagonal core.
class LibraryManager {
   public:
    explicit LibraryManager(std::filesystem::path storageRoot);

    // Copies/extracts sourcePath into <storageRoot>/<title>/, normalizing the
    // internal file name by detected type (video.<ext>, image.<ext>; Web
    // content is copied as-is). Fails if the destination folder already
    // exists, or if a Web source has no root-level index.html.
    ImportResult import(const std::string& title, const std::filesystem::path& sourcePath);

    // Renames <storageRoot>/<oldTitle>/ to <storageRoot>/<newTitle>/.
    bool rename(const std::string& oldTitle, const std::string& newTitle);

    // Deletes <storageRoot>/<title>/ and its contents.
    bool remove(const std::string& title);

    std::filesystem::path pathForTitle(const std::string& title) const;

    // Convention-based path for title's generated preview image, whether
    // or not it's actually been generated yet — the Windows-only
    // ThumbnailGenerator writes here after a successful import; this class
    // only knows the naming convention, not how to produce the file
    // (that needs Media Foundation/WIC, a Win32 dependency this core class
    // doesn't have).
    std::filesystem::path thumbnailPathForTitle(const std::string& title) const;

    // Every currently-imported wallpaper, discovered by scanning
    // storageRoot_'s immediate subdirectories (one per title, per
    // import()'s layout) rather than tracked separately.
    std::vector<LibraryEntry> list() const;

   private:
    std::filesystem::path storageRoot_;
};

}  // namespace umbra
