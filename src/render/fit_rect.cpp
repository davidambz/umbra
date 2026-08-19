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

#include "render/fit_rect.h"

#include <algorithm>

namespace umbra {

namespace {

Rect scaledAndCentered(Size source, Size target, double scale) {
    const int width = static_cast<int>(source.width * scale);
    const int height = static_cast<int>(source.height * scale);
    return Rect{
        (target.width - width) / 2,
        (target.height - height) / 2,
        width,
        height,
    };
}

}  // namespace

Rect computeFitRect(Size source, Size target, FitMode fitMode) {
    if (source.width <= 0 || source.height <= 0 || target.width <= 0 || target.height <= 0) {
        return Rect{};
    }

    if (fitMode == FitMode::Stretch) {
        return Rect{0, 0, target.width, target.height};
    }

    const double widthScale = static_cast<double>(target.width) / source.width;
    const double heightScale = static_cast<double>(target.height) / source.height;
    const double scale = fitMode == FitMode::Contain ? std::min(widthScale, heightScale)
                                                     : std::max(widthScale, heightScale);
    return scaledAndCentered(source, target, scale);
}

}  // namespace umbra
