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

// Detects a wallpaper's type from its source path: a video/image file is
// classified by extension, while a directory or .zip is always Web (a web
// wallpaper is a project folder, not a single file).
WallpaperType detectWallpaperType(const std::filesystem::path& sourcePath);

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

   private:
    std::filesystem::path storageRoot_;
};

}  // namespace umbra
