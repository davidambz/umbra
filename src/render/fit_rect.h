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

namespace umbra {

// How to fit a source frame (video/image/web content) into a target render
// surface when their aspect ratios differ.
enum class FitMode {
    Stretch,  // exact fill, aspect ratio ignored
    Contain,  // whole source visible, letterboxed/pillarboxed if needed
    Cover,    // fills target entirely, cropping source if needed
};

struct Size {
    int width = 0;
    int height = 0;

    bool operator==(const Size&) const = default;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const Rect&) const = default;
};

// Computes where to draw a source of the given size within a target of the
// given size, per fitMode. A monitor's resolution rarely matches a
// wallpaper's content resolution 1:1, so Compositor uses this to position
// and scale each frame instead of assuming an exact fit. Returns an
// all-zero Rect if either size has a non-positive dimension.
Rect computeFitRect(Size source, Size target, FitMode fitMode);

}  // namespace umbra
