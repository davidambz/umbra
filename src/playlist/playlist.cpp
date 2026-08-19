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

#include "playlist/playlist.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace umbra {

namespace {
const std::string kEmptyId;
}  // namespace

std::string toString(PlaylistMode mode) {
    switch (mode) {
        case PlaylistMode::Sequential:
            return "sequential";
        case PlaylistMode::Shuffle:
            return "shuffle";
    }
    throw std::invalid_argument("unknown PlaylistMode");
}

PlaylistMode playlistModeFromString(const std::string& value) {
    if (value == "sequential") return PlaylistMode::Sequential;
    if (value == "shuffle") return PlaylistMode::Shuffle;
    throw std::invalid_argument("unknown playlist mode string: " + value);
}

bool Playlist::isValid() const { return !wallpaperIds.empty() && intervalSeconds > 0; }

PlaylistRotator::PlaylistRotator(Playlist playlist, uint32_t seed)
    : playlist_(std::move(playlist)), rng_(seed) {
    buildOrder();
}

void PlaylistRotator::reshuffleIfNeeded() {
    if (playlist_.mode == PlaylistMode::Shuffle) {
        std::shuffle(order_.begin(), order_.end(), rng_);
    }
}

void PlaylistRotator::buildOrder() {
    order_.resize(playlist_.wallpaperIds.size());
    std::iota(order_.begin(), order_.end(), 0);
    reshuffleIfNeeded();
    position_ = 0;
}

const std::string& PlaylistRotator::current() const {
    if (order_.empty()) {
        return kEmptyId;
    }
    return playlist_.wallpaperIds[order_[position_]];
}

const std::string& PlaylistRotator::advance() {
    if (order_.empty()) {
        return kEmptyId;
    }

    ++position_;
    if (position_ >= order_.size()) {
        reshuffleIfNeeded();
        position_ = 0;
    }
    return current();
}

void PlaylistRotator::reset() { buildOrder(); }

}  // namespace umbra
