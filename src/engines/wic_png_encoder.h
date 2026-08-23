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

#include <windows.h>

#include <filesystem>
#include <vector>

namespace umbra {

// Encodes a tightly-packed 32bpp BGRA pixel buffer (width*height*4 bytes,
// no row padding) to a PNG file via WIC. Shared by every adapter that
// writes a PNG snapshot from raw pixels — LockScreenSync's lock screen
// capture and ThumbnailGenerator's video/image previews — so the same
// pitfall isn't fixed twice: WIC's PNG encoder doesn't natively support
// every pixel format IWICBitmapFrameEncode::SetPixelFormat is asked for,
// and silently substitutes an encoder-supported one instead of failing —
// without also swapping the caller's bytes to match. This verifies the
// requested BGRA format actually stuck before writing pixels, rather than
// trusting that substitution never happens. Returns false on any failure.
bool encodeBgraPixelsToPngFile(const std::vector<BYTE>& bgraPixels, UINT width, UINT height,
                               const std::filesystem::path& path);

}  // namespace umbra
