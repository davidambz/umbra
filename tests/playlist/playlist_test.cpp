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

#include <gtest/gtest.h>

#include <algorithm>
#include <set>

using umbra::Playlist;
using umbra::PlaylistMode;
using umbra::PlaylistRotator;

TEST(PlaylistMode, RoundTripsThroughString) {
    EXPECT_EQ(umbra::toString(PlaylistMode::Sequential), "sequential");
    EXPECT_EQ(umbra::toString(PlaylistMode::Shuffle), "shuffle");
    EXPECT_EQ(umbra::playlistModeFromString("sequential"), PlaylistMode::Sequential);
    EXPECT_EQ(umbra::playlistModeFromString("shuffle"), PlaylistMode::Shuffle);
}

TEST(PlaylistMode, ThrowsOnUnknownString) {
    EXPECT_THROW(umbra::playlistModeFromString("random"), std::invalid_argument);
}

TEST(Playlist, InvalidWhenEmpty) {
    Playlist playlist;
    EXPECT_FALSE(playlist.isValid());
}

TEST(Playlist, InvalidWithNonPositiveInterval) {
    Playlist playlist;
    playlist.wallpaperIds = {"a"};
    playlist.intervalSeconds = 0;
    EXPECT_FALSE(playlist.isValid());
}

TEST(Playlist, ValidWithIdsAndPositiveInterval) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b"};
    playlist.intervalSeconds = 60;
    EXPECT_TRUE(playlist.isValid());
}

TEST(PlaylistRotator, EmptyPlaylistReturnsEmptyId) {
    Playlist playlist;
    PlaylistRotator rotator(playlist);

    EXPECT_TRUE(rotator.current().empty());
    EXPECT_TRUE(rotator.advance().empty());
}

TEST(PlaylistRotator, SingleItemStaysOnItself) {
    Playlist playlist;
    playlist.wallpaperIds = {"only"};
    PlaylistRotator rotator(playlist);

    EXPECT_EQ(rotator.current(), "only");
    EXPECT_EQ(rotator.advance(), "only");
    EXPECT_EQ(rotator.advance(), "only");
}

TEST(PlaylistRotator, SequentialAdvancesInOrderAndWraps) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c"};
    playlist.mode = PlaylistMode::Sequential;
    PlaylistRotator rotator(playlist);

    EXPECT_EQ(rotator.current(), "a");
    EXPECT_EQ(rotator.advance(), "b");
    EXPECT_EQ(rotator.advance(), "c");
    EXPECT_EQ(rotator.advance(), "a");
    EXPECT_EQ(rotator.advance(), "b");
}

TEST(PlaylistRotator, SequentialResetStartsOverFromFirstItem) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c"};
    playlist.mode = PlaylistMode::Sequential;
    PlaylistRotator rotator(playlist);

    rotator.advance();
    rotator.advance();
    ASSERT_EQ(rotator.current(), "c");

    rotator.reset();

    EXPECT_EQ(rotator.current(), "a");
}

TEST(PlaylistRotator, ShuffleVisitsEveryItemExactlyOncePerCycle) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c", "d"};
    playlist.mode = PlaylistMode::Shuffle;
    PlaylistRotator rotator(playlist, /*seed=*/42);

    std::set<std::string> seen;
    seen.insert(rotator.current());
    for (int i = 0; i < 3; ++i) {
        seen.insert(rotator.advance());
    }

    EXPECT_EQ(seen, std::set<std::string>({"a", "b", "c", "d"}));
}

TEST(PlaylistRotator, ShuffleIsDeterministicForAGivenSeed) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c", "d"};
    playlist.mode = PlaylistMode::Shuffle;

    PlaylistRotator first(playlist, /*seed=*/7);
    PlaylistRotator second(playlist, /*seed=*/7);

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(first.current(), second.current());
        first.advance();
        second.advance();
    }
}

TEST(PlaylistRotator, DifferentSeedsCanProduceDifferentOrders) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c", "d", "e", "f", "g", "h"};
    playlist.mode = PlaylistMode::Shuffle;

    PlaylistRotator first(playlist, /*seed=*/1);
    PlaylistRotator second(playlist, /*seed=*/2);

    std::vector<std::string> firstOrder;
    std::vector<std::string> secondOrder;
    firstOrder.push_back(first.current());
    secondOrder.push_back(second.current());
    for (int i = 0; i < 7; ++i) {
        firstOrder.push_back(first.advance());
        secondOrder.push_back(second.advance());
    }

    EXPECT_NE(firstOrder, secondOrder);
}
