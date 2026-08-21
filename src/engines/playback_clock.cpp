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

#include "engines/playback_clock.h"

#include <cmath>

namespace umbra {

PlaybackClock::PlaybackClock(double durationSeconds, int fpsCap)
    : durationSeconds_(durationSeconds),
      frameIntervalSeconds_(fpsCap > 0 ? 1.0 / static_cast<double>(fpsCap) : 0.0) {}

bool PlaybackClock::advance(double deltaSeconds) {
    if (durationSeconds_ > 0.0) {
        position_ = std::fmod(position_ + deltaSeconds, durationSeconds_);
        if (position_ < 0.0) {
            position_ += durationSeconds_;
        }
    }

    if (frameIntervalSeconds_ <= 0.0) {
        return true;
    }

    sinceLastFrame_ += deltaSeconds;
    if (sinceLastFrame_ < frameIntervalSeconds_) {
        return false;
    }
    sinceLastFrame_ = std::fmod(sinceLastFrame_, frameIntervalSeconds_);
    return true;
}

}  // namespace umbra
