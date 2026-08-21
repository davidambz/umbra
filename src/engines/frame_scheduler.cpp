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

#include "engines/frame_scheduler.h"

#include <algorithm>

namespace umbra {

namespace {

int clampDelay(int delayMs) { return std::max(delayMs, FrameScheduler::kMinFrameDelayMs); }

}  // namespace

FrameScheduler::FrameScheduler(std::vector<int> frameDelaysMs)
    : frameDelaysMs_(std::move(frameDelaysMs)) {
    for (int& delay : frameDelaysMs_) {
        delay = clampDelay(delay);
    }
}

int FrameScheduler::advance(double deltaSeconds) {
    if (frameDelaysMs_.empty()) {
        return currentIndex_;
    }

    elapsedMs_ += deltaSeconds * 1000.0;
    while (elapsedMs_ >= frameDelaysMs_[currentIndex_]) {
        elapsedMs_ -= frameDelaysMs_[currentIndex_];
        currentIndex_ = (currentIndex_ + 1) % static_cast<int>(frameDelaysMs_.size());
    }
    return currentIndex_;
}

}  // namespace umbra
