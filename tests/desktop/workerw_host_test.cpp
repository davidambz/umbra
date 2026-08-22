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

#include "desktop/workerw_host.h"

#include <gtest/gtest.h>

#include "mocks/win32_mocks.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using umbra::WindowHandle;
using umbra::WorkerWHost;
using umbra::testing_support::MockWorkerWApi;

namespace {
// Arbitrary non-null sentinel values standing in for real HWNDs.
WindowHandle asHandle(int value) {
    return reinterpret_cast<WindowHandle>(static_cast<intptr_t>(value));
}
}  // namespace

TEST(WorkerWHost, EnsureWorkerWSpawnedRunsTheFullSequenceOnce) {
    MockWorkerWApi api;
    const WindowHandle progman = asHandle(1);
    const WindowHandle workerW = asHandle(2);

    EXPECT_CALL(api, findWindowByClass(testing::StrEq("Progman"))).WillOnce(Return(progman));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(progman)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(workerW));

    WorkerWHost host(api);

    EXPECT_TRUE(host.ensureWorkerWSpawned());
    EXPECT_EQ(host.workerW(), workerW);
}

TEST(WorkerWHost, EnsureWorkerWSpawnedIsIdempotent) {
    MockWorkerWApi api;
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(asHandle(2)));

    WorkerWHost host(api);
    ASSERT_TRUE(host.ensureWorkerWSpawned());

    // A second call must not repeat the spawn sequence — no further calls
    // are expected on the mock, so the strict call counts above enforce it.
    EXPECT_TRUE(host.ensureWorkerWSpawned());
}

TEST(WorkerWHost, EnsureWorkerWSpawnedFailsWhenProgmanIsMissing) {
    MockWorkerWApi api;
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(umbra::kNullWindow));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(0);
    EXPECT_CALL(api, findBackgroundWorkerW()).Times(0);

    WorkerWHost host(api);

    EXPECT_FALSE(host.ensureWorkerWSpawned());
}

TEST(WorkerWHost, EnsureWorkerWSpawnedFailsWhenWorkerWCannotBeFound) {
    MockWorkerWApi api;
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    // Every lookup attempt in the retry loop comes back empty, so all of
    // them run before giving up, with a sleep between each but not after
    // the last.
    EXPECT_CALL(api, findBackgroundWorkerW()).Times(5).WillRepeatedly(Return(umbra::kNullWindow));
    EXPECT_CALL(api, sleepMilliseconds(_)).Times(4);

    WorkerWHost host(api);

    EXPECT_FALSE(host.ensureWorkerWSpawned());
}

TEST(WorkerWHost, EnsureWorkerWSpawnedRetriesUntilTheHierarchySettles) {
    // Simulates the shell's window hierarchy still settling right after
    // the spawn message — the first two lookups miss, the third finds it.
    MockWorkerWApi api;
    const WindowHandle workerW = asHandle(2);
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW())
        .WillOnce(Return(umbra::kNullWindow))
        .WillOnce(Return(umbra::kNullWindow))
        .WillOnce(Return(workerW));
    EXPECT_CALL(api, sleepMilliseconds(_)).Times(2);

    WorkerWHost host(api);

    EXPECT_TRUE(host.ensureWorkerWSpawned());
    EXPECT_EQ(host.workerW(), workerW);
}

TEST(WorkerWHost, AttachFailsBeforeWorkerWIsSpawned) {
    MockWorkerWApi api;
    EXPECT_CALL(api, setParent(_, _)).Times(0);

    WorkerWHost host(api);

    EXPECT_FALSE(host.attach(asHandle(42), 0, 0, 100, 100));
}

TEST(WorkerWHost, AttachReparentsAndPositionsRenderWindowIntoWorkerW) {
    MockWorkerWApi api;
    const WindowHandle workerW = asHandle(2);
    const WindowHandle renderWindow = asHandle(42);

    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(workerW));
    EXPECT_CALL(api, setParent(renderWindow, workerW)).WillOnce(Return(true));

    WorkerWHost host(api);
    ASSERT_TRUE(host.ensureWorkerWSpawned());

    // Virtual desktop origin at (-1920, 0) — a monitor sits to the left of
    // the primary one — so requesting screen position (0, 0) must
    // translate to (1920, 0) relative to that origin.
    EXPECT_CALL(api, getVirtualScreenOrigin(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(-1920), SetArgPointee<1>(0)));
    EXPECT_CALL(api, setWindowPosition(renderWindow, 1920, 0, 1920, 1080)).Times(1);

    EXPECT_TRUE(host.attach(renderWindow, 0, 0, 1920, 1080));
}

TEST(WorkerWHost, AttachRetriesSpawnSequenceWhenTheCachedWorkerWDied) {
    // Simulates an explorer.exe restart destroying the cached WorkerW
    // between the initial spawn and a later attach() (e.g. a monitor
    // hot-plug rebuild): setParent() against the stale handle fails, so
    // attach() should invalidate the cache, re-run the spawn sequence,
    // and retry setParent() against the freshly found WorkerW.
    MockWorkerWApi api;
    const WindowHandle staleWorkerW = asHandle(2);
    const WindowHandle freshWorkerW = asHandle(3);
    const WindowHandle renderWindow = asHandle(42);

    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(staleWorkerW));

    WorkerWHost host(api);
    ASSERT_TRUE(host.ensureWorkerWSpawned());

    EXPECT_CALL(api, setParent(renderWindow, staleWorkerW)).WillOnce(Return(false));
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(freshWorkerW));
    EXPECT_CALL(api, setParent(renderWindow, freshWorkerW)).WillOnce(Return(true));
    EXPECT_CALL(api, getVirtualScreenOrigin(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(0), SetArgPointee<1>(0)));
    EXPECT_CALL(api, setWindowPosition(renderWindow, 0, 0, 100, 100)).Times(1);

    EXPECT_TRUE(host.attach(renderWindow, 0, 0, 100, 100));
    EXPECT_EQ(host.workerW(), freshWorkerW);
}

TEST(WorkerWHost, AttachFailsIfRespawnAfterDeathAlsoFails) {
    MockWorkerWApi api;
    const WindowHandle staleWorkerW = asHandle(2);
    const WindowHandle renderWindow = asHandle(42);

    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(staleWorkerW));

    WorkerWHost host(api);
    ASSERT_TRUE(host.ensureWorkerWSpawned());

    EXPECT_CALL(api, setParent(renderWindow, staleWorkerW)).WillOnce(Return(false));
    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(umbra::kNullWindow));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(0);
    EXPECT_CALL(api, findBackgroundWorkerW()).Times(0);

    EXPECT_FALSE(host.attach(renderWindow, 0, 0, 100, 100));
}

TEST(WorkerWHost, AttachCanBeCalledForMultipleMonitorsAfterOneSpawn) {
    MockWorkerWApi api;
    const WindowHandle workerW = asHandle(2);

    EXPECT_CALL(api, findWindowByClass(_)).WillOnce(Return(asHandle(1)));
    EXPECT_CALL(api, sendSpawnWorkerWMessage(_)).Times(1);
    EXPECT_CALL(api, findBackgroundWorkerW()).WillOnce(Return(workerW));
    EXPECT_CALL(api, setParent(asHandle(42), workerW)).WillOnce(Return(true));
    EXPECT_CALL(api, setParent(asHandle(43), workerW)).WillOnce(Return(true));
    EXPECT_CALL(api, getVirtualScreenOrigin(_, _))
        .Times(2)
        .WillRepeatedly(DoAll(SetArgPointee<0>(0), SetArgPointee<1>(0)));
    EXPECT_CALL(api, setWindowPosition(asHandle(42), 0, 0, 1920, 1080)).Times(1);
    EXPECT_CALL(api, setWindowPosition(asHandle(43), 1920, 0, 1920, 1080)).Times(1);

    WorkerWHost host(api);
    ASSERT_TRUE(host.ensureWorkerWSpawned());

    EXPECT_TRUE(host.attach(asHandle(42), 0, 0, 1920, 1080));
    EXPECT_TRUE(host.attach(asHandle(43), 1920, 0, 1920, 1080));
}
