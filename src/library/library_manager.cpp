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

#include "library/library_manager.h"

#include <zip.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace umbra {

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return value;
}

// A title becomes a folder name directly under storageRoot_, so it must not
// be a path traversal segment or contain a path separator.
bool isValidTitle(const std::string& title) {
    if (title == "." || title == "..") {
        return false;
    }
    return title.find('/') == std::string::npos && title.find('\\') == std::string::npos;
}

bool hasRootIndexHtml(const fs::path& webRoot) {
    for (const auto& entry : fs::directory_iterator(webRoot)) {
        if (entry.is_regular_file() && toLower(entry.path().filename().string()) == "index.html") {
            return true;
        }
    }
    return false;
}

// Copies every entry of a zip archive into destRoot, rejecting any entry
// whose normalized path would escape destRoot (a "zip slip" attack).
bool extractZip(const fs::path& zipPath, const fs::path& destRoot, std::error_code& ec) {
    int errorCode = 0;
    zip_t* archive = zip_open(zipPath.string().c_str(), ZIP_RDONLY, &errorCode);
    if (archive == nullptr) {
        ec = std::make_error_code(std::errc::io_error);
        return false;
    }

    const zip_int64_t entryCount = zip_get_num_entries(archive, 0);
    bool ok = true;
    for (zip_int64_t i = 0; ok && i < entryCount; ++i) {
        const char* rawName = zip_get_name(archive, static_cast<zip_uint64_t>(i), 0);
        if (rawName == nullptr) {
            continue;
        }

        const fs::path entryPath = fs::path(rawName);
        const fs::path destPath = (destRoot / entryPath).lexically_normal();
        const fs::path relative = destPath.lexically_relative(destRoot);
        if (relative.empty() || relative.begin()->string() == "..") {
            ok = false;
            break;
        }

        const std::string name(rawName);
        if (!name.empty() && name.back() == '/') {
            fs::create_directories(destPath, ec);
            continue;
        }

        fs::create_directories(destPath.parent_path(), ec);
        zip_file_t* zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0);
        if (zf == nullptr) {
            ok = false;
            break;
        }

        std::ofstream out(destPath, std::ios::binary);
        char buffer[8192];
        zip_int64_t bytesRead = 0;
        while ((bytesRead = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            out.write(buffer, bytesRead);
        }
        zip_fclose(zf);
        if (!out) {
            ok = false;
            break;
        }
    }

    zip_close(archive);
    return ok;
}

}  // namespace

WallpaperType detectWallpaperType(const fs::path& sourcePath) {
    std::error_code ec;
    if (fs::is_directory(sourcePath, ec)) {
        return WallpaperType::Web;
    }

    const std::string ext = toLower(sourcePath.extension().string());
    if (ext == ".mp4" || ext == ".webm") {
        return WallpaperType::Video;
    }
    if (ext == ".gif" || ext == ".apng") {
        return WallpaperType::Image;
    }
    if (ext == ".zip") {
        return WallpaperType::Web;
    }
    throw std::invalid_argument("unsupported wallpaper source extension: " + ext);
}

LibraryManager::LibraryManager(fs::path storageRoot) : storageRoot_(std::move(storageRoot)) {}

fs::path LibraryManager::pathForTitle(const std::string& title) const {
    return storageRoot_ / title;
}

ImportResult LibraryManager::import(const std::string& title, const fs::path& sourcePath) {
    ImportResult result;

    if (title.empty()) {
        result.error = ImportError::TitleEmpty;
        return result;
    }
    if (!isValidTitle(title)) {
        result.error = ImportError::InvalidTitle;
        return result;
    }

    std::error_code ec;
    if (!fs::exists(sourcePath, ec)) {
        result.error = ImportError::SourceNotFound;
        return result;
    }

    WallpaperType type;
    try {
        type = detectWallpaperType(sourcePath);
    } catch (const std::invalid_argument&) {
        result.error = ImportError::UnsupportedFileType;
        return result;
    }

    const fs::path destDir = pathForTitle(title);
    if (fs::exists(destDir, ec)) {
        result.error = ImportError::DestinationAlreadyExists;
        return result;
    }

    fs::create_directories(destDir, ec);
    if (ec) {
        result.error = ImportError::CopyFailed;
        return result;
    }

    if (type == WallpaperType::Web) {
        if (fs::is_directory(sourcePath, ec)) {
            if (!hasRootIndexHtml(sourcePath)) {
                fs::remove_all(destDir, ec);
                result.error = ImportError::WebMissingIndexHtml;
                return result;
            }
            fs::copy(sourcePath, destDir, fs::copy_options::recursive, ec);
            if (ec) {
                fs::remove_all(destDir, ec);
                result.error = ImportError::CopyFailed;
                return result;
            }
        } else {
            if (!extractZip(sourcePath, destDir, ec)) {
                fs::remove_all(destDir, ec);
                result.error = ImportError::CopyFailed;
                return result;
            }
            if (!hasRootIndexHtml(destDir)) {
                fs::remove_all(destDir, ec);
                result.error = ImportError::WebMissingIndexHtml;
                return result;
            }
        }
    } else {
        const std::string normalizedName = (type == WallpaperType::Video ? "video" : "image") +
                                           toLower(sourcePath.extension().string());
        fs::copy_file(sourcePath, destDir / normalizedName, fs::copy_options::none, ec);
        if (ec) {
            fs::remove_all(destDir, ec);
            result.error = ImportError::CopyFailed;
            return result;
        }
    }

    result.success = true;
    result.profile.type = type;
    result.profile.path = destDir.string();
    return result;
}

bool LibraryManager::rename(const std::string& oldTitle, const std::string& newTitle) {
    if (oldTitle.empty() || newTitle.empty()) {
        return false;
    }
    if (!isValidTitle(oldTitle) || !isValidTitle(newTitle)) {
        return false;
    }

    std::error_code ec;
    const fs::path oldPath = pathForTitle(oldTitle);
    const fs::path newPath = pathForTitle(newTitle);
    if (!fs::exists(oldPath, ec) || fs::exists(newPath, ec)) {
        return false;
    }

    fs::rename(oldPath, newPath, ec);
    return !ec;
}

bool LibraryManager::remove(const std::string& title) {
    if (title.empty() || !isValidTitle(title)) {
        return false;
    }

    std::error_code ec;
    const auto removedCount = fs::remove_all(pathForTitle(title), ec);
    return !ec && removedCount > 0;
}

}  // namespace umbra
