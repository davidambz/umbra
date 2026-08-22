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

#include <filesystem>
#include <string>

namespace umbra {

// Converts a UTF-8 std::string (the encoding WallpaperProfile::path is
// stored in) to the UTF-16 std::wstring the Win32/Media Foundation/WIC
// file APIs need. Shared by every engine that opens a file by path, rather
// than each reimplementing it.
std::wstring utf8ToWide(const std::string& utf8);

// Builds a "file:///" URL WebView2's Navigate() can load, percent-encoding
// every byte outside the URL-safe set — a raw, unencoded path breaks on
// e.g. a space (common in an installed "C:\Program Files\..." path) or
// any non-ASCII character, since those aren't valid literally inside a
// URL. Used as navigateToLocalFolder()'s fallback below.
std::wstring toFileUrl(const std::filesystem::path& path);

// Navigates webView to "index.html" inside folder by mapping
// virtualHostName to folder as a virtual https:// origin
// (SetVirtualHostNameToFolderMapping) rather than a plain file://
// Navigate(). A page loaded via file:// can load itself but this
// Chromium version enforces CORS on file:// origins, so any separate
// CSS/JS file it references via a relative URL fails to load
// (net::ERR_FAILED) with no visible error outside DevTools — see issue
// #31, found when this broke settings-ui/'s bundle and, by the same
// pattern, would have broken any multi-file imported Web wallpaper too.
// Uses COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY: only same-origin
// requests from the page navigated to under virtualHostName need to load
// folder's contents, and DENY still permits that — ALLOW would additionally
// let *other* origins (e.g. content a Web wallpaper embeds) read every
// file under folder, which nothing here needs. Falls back to
// toFileUrl() if ICoreWebView2_3 isn't available or the mapping call
// itself fails, so a page is still shown either way.
void navigateToLocalFolder(ICoreWebView2* webView, const std::filesystem::path& folder,
                           const wchar_t* virtualHostName);

}  // namespace umbra
