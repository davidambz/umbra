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

// Tracks looped video playback position and paces decoding against an fps
// cap, independent of any Media Foundation decode call — this is the part
// of VideoEngine's behavior that's pure logic and unit-testable without a
// live decoder (see VideoEngine, which owns one of these and only calls
// into Media Foundation when advance() says a new frame is due).
class PlaybackClock {
   public:
    // durationSeconds is the clip's total length; playback loops back to 0
    // once positionSeconds() would reach it. A non-positive durationSeconds
    // disables looping (position stays pinned at 0). fpsCap <= 0 means
    // uncapped: every advance() call is due a new frame.
    explicit PlaybackClock(double durationSeconds, int fpsCap = 60);

    // Advances playback time by deltaSeconds, looping position as needed.
    // Returns true if enough time has accumulated since the last frame
    // that was due (per fpsCap) that the caller should decode a new one.
    bool advance(double deltaSeconds);

    double positionSeconds() const { return position_; }

   private:
    double durationSeconds_;
    double frameIntervalSeconds_;
    double position_ = 0.0;
    double sinceLastFrame_ = 0.0;
};

}  // namespace umbra
