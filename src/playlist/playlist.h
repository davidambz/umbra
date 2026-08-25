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

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace umbra {

enum class PlaylistMode {
    Sequential,
    Shuffle,
};

std::string toString(PlaylistMode mode);
PlaylistMode playlistModeFromString(const std::string& value);

// A monitor's playlist: an ordered set of imported wallpaper ids that rotate
// automatically after intervalSeconds, per ARCHITECTURE.md's "inline per
// monitor" persistence decision. Kept as plain data so it round-trips
// through Settings the same way WallpaperProfile does.
struct Playlist {
    std::vector<std::string> wallpaperIds;
    int intervalSeconds = 300;
    PlaylistMode mode = PlaylistMode::Sequential;

    bool isValid() const;
};

// A deterministic seed derived from a playlist's own content (its
// wallpaperIds and mode) rather than a random one. Two PlaylistRotators
// built from playlists with identical content — most notably two
// monitors mirroring the same assignment while Settings.syncMonitors is
// on (see issue #71) — end up shuffling to the exact same order and so
// stay in lockstep, without either needing to know about the other or
// share any state. Different content (a different wallpaper set, or a
// different mode) still produces a different seed, so Shuffle mode
// isn't stuck reshuffling to one fixed order for every playlist the way
// PlaylistRotator's own hardcoded test-only default would be.
uint32_t deterministicSeedForPlaylist(const Playlist& playlist);

// Tracks which item of a Playlist is currently active and computes the next
// one per its rotation mode. Pure logic, no timer/thread of its own — the
// caller (eventually the app orchestrator) is responsible for calling
// advance() every intervalSeconds.
class PlaylistRotator {
   public:
    // seed is exposed (rather than defaulting to a hardware-derived seed)
    // so Shuffle-mode rotation stays deterministic and unit-testable.
    explicit PlaylistRotator(Playlist playlist, uint32_t seed = 1);

    // The currently active wallpaper id. Empty if the playlist has no items.
    const std::string& current() const;

    // Moves to the next item per the playlist's mode and returns it.
    // A no-op (returns the same single item) for a one-item playlist.
    const std::string& advance();

    // Restarts rotation from the beginning (re-shuffling in Shuffle mode).
    void reset();

   private:
    void buildOrder();
    void reshuffleIfNeeded();

    Playlist playlist_;
    std::mt19937 rng_;
    std::vector<std::size_t> order_;
    std::size_t position_ = 0;
};

}  // namespace umbra
