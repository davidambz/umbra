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

using umbra::deterministicSeedForPlaylist;
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

TEST(PlaylistRotator, ShuffleNeverRepeatsTheSameItemBackToBackAcrossACycleBoundary) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c"};
    playlist.mode = PlaylistMode::Shuffle;

    // Several seeds, several full cycles each — std::shuffle has no
    // built-in reason to avoid picking the previous cycle's last item as
    // the next cycle's first, so this is exactly the seam where a
    // same-wallpaper-twice-in-a-row could otherwise slip through.
    for (uint32_t seed = 0; seed < 20; ++seed) {
        PlaylistRotator rotator(playlist, seed);
        std::string previous = rotator.current();
        for (int i = 0; i < 30; ++i) {
            const std::string next = rotator.advance();
            EXPECT_NE(next, previous) << "seed=" << seed << " step=" << i;
            previous = next;
        }
    }
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

TEST(DeterministicSeedForPlaylist, TwoRotatorsFromIdenticalPlaylistsShuffleTheSameWay) {
    Playlist playlist;
    playlist.wallpaperIds = {"a", "b", "c", "d", "e", "f"};
    playlist.mode = PlaylistMode::Shuffle;

    // Mirrors two monitors independently building a PlaylistRotator from
    // the same mirrored assignment (Settings.syncMonitors, issue #71) —
    // neither knows about the other, so the only thing keeping them in
    // lockstep is both landing on the same seed for the same content.
    PlaylistRotator first(playlist, deterministicSeedForPlaylist(playlist));
    PlaylistRotator second(playlist, deterministicSeedForPlaylist(playlist));

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(first.current(), second.current());
        first.advance();
        second.advance();
    }
}

TEST(DeterministicSeedForPlaylist, DifferentWallpaperIdsProduceADifferentSeed) {
    Playlist a;
    a.wallpaperIds = {"a", "b", "c"};
    a.mode = PlaylistMode::Shuffle;
    Playlist b;
    b.wallpaperIds = {"x", "y", "z"};
    b.mode = PlaylistMode::Shuffle;

    EXPECT_NE(deterministicSeedForPlaylist(a), deterministicSeedForPlaylist(b));
}

TEST(DeterministicSeedForPlaylist, DifferentModeProducesADifferentSeed) {
    Playlist sequential;
    sequential.wallpaperIds = {"a", "b", "c"};
    sequential.mode = PlaylistMode::Sequential;
    Playlist shuffle;
    shuffle.wallpaperIds = {"a", "b", "c"};
    shuffle.mode = PlaylistMode::Shuffle;

    EXPECT_NE(deterministicSeedForPlaylist(sequential), deterministicSeedForPlaylist(shuffle));
}
