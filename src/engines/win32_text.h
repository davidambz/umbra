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
// URL. Shared by web_engine.cpp and settings_window.cpp rather than each
// reimplementing (and each having to be separately remembered to fix) it.
std::wstring toFileUrl(const std::filesystem::path& path);

}  // namespace umbra
