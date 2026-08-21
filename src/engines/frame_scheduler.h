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

#include <vector>

namespace umbra {

// Picks which frame of an animated image (gif/apng) should be on screen
// given elapsed time and each frame's own delay, looping back to frame 0
// after the last one. This is the part of ImageEngine's behavior that's
// pure logic and unit-testable without a live WIC decoder — ImageEngine
// owns one of these and only re-decodes/re-uploads a texture when
// advance() reports the index changed.
class FrameScheduler {
   public:
    // frameDelaysMs is one entry per frame, in display order. Delays
    // <= 0 (some encoders emit a 0ms delay, which browsers/viewers treat
    // as a small default rather than an infinite-spin frame) are clamped
    // up to kMinFrameDelayMs. An empty list means "not animated" — index
    // stays 0 regardless of elapsed time.
    explicit FrameScheduler(std::vector<int> frameDelaysMs);

    // Advances by deltaSeconds and returns the frame index that should be
    // displayed now.
    int advance(double deltaSeconds);

    int currentIndex() const { return currentIndex_; }

    static constexpr int kMinFrameDelayMs = 10;

   private:
    std::vector<int> frameDelaysMs_;
    double elapsedMs_ = 0.0;
    int currentIndex_ = 0;
};

}  // namespace umbra
