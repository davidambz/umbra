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

#include "power/fullscreen_watcher.h"

#include <gtest/gtest.h>

#include "mocks/win32_mocks.h"

using ::testing::Return;
using umbra::ForegroundWindowInfo;
using umbra::FullscreenWatcher;
using umbra::isFullscreenForeground;
using umbra::isShellWindowClass;
using umbra::Rect;
using umbra::testing_support::MockFullscreenApi;

namespace {
ForegroundWindowInfo fullscreenAppInfo() {
    ForegroundWindowInfo info;
    info.windowRect = Rect{0, 0, 1920, 1080};
    info.monitorRect = Rect{0, 0, 1920, 1080};
    info.windowClassName = "SomeGameWindowClass";
    return info;
}
}  // namespace

TEST(IsFullscreenForeground, TrueWhenWindowRectExactlyMatchesMonitorRect) {
    EXPECT_TRUE(isFullscreenForeground(fullscreenAppInfo()));
}

TEST(IsFullscreenForeground, FalseWhenWindowIsSmallerThanMonitor) {
    ForegroundWindowInfo info = fullscreenAppInfo();
    info.windowRect = Rect{0, 0, 1280, 720};
    EXPECT_FALSE(isFullscreenForeground(info));
}

TEST(IsFullscreenForeground, FalseWhenMinimized) {
    ForegroundWindowInfo info = fullscreenAppInfo();
    info.isMinimized = true;
    EXPECT_FALSE(isFullscreenForeground(info));
}

TEST(IsFullscreenForeground, FalseWhenCloaked) {
    ForegroundWindowInfo info = fullscreenAppInfo();
    info.isCloaked = true;
    EXPECT_FALSE(isFullscreenForeground(info));
}

TEST(IsFullscreenForeground, FalseForUmbraSOwnWorkerWWindow) {
    ForegroundWindowInfo info = fullscreenAppInfo();
    info.windowClassName = "WorkerW";
    EXPECT_FALSE(isFullscreenForeground(info));
}

TEST(IsFullscreenForeground, FalseWhenMonitorRectIsUnknown) {
    ForegroundWindowInfo info = fullscreenAppInfo();
    info.monitorRect = Rect{};
    EXPECT_FALSE(isFullscreenForeground(info));
}

TEST(IsShellWindowClass, RecognizesKnownShellClasses) {
    EXPECT_TRUE(isShellWindowClass("Progman"));
    EXPECT_TRUE(isShellWindowClass("WorkerW"));
    EXPECT_TRUE(isShellWindowClass("Shell_TrayWnd"));
    EXPECT_TRUE(isShellWindowClass("Shell_SecondaryTrayWnd"));
    EXPECT_FALSE(isShellWindowClass("SomeGameWindowClass"));
}

TEST(FullscreenWatcher, RefreshReportsChangeWhenFullscreenAppAppears) {
    MockFullscreenApi api;
    EXPECT_CALL(api, queryForegroundWindow()).WillOnce(Return(fullscreenAppInfo()));

    FullscreenWatcher watcher(api);
    EXPECT_TRUE(watcher.refresh());
    EXPECT_TRUE(watcher.isFullscreenActive());
}

TEST(FullscreenWatcher, RefreshReportsNoChangeWhileFullscreenStateIsStable) {
    MockFullscreenApi api;
    ForegroundWindowInfo notFullscreen = fullscreenAppInfo();
    notFullscreen.windowRect = Rect{0, 0, 1280, 720};
    EXPECT_CALL(api, queryForegroundWindow()).WillRepeatedly(Return(notFullscreen));

    FullscreenWatcher watcher(api);
    EXPECT_FALSE(watcher.refresh());
    EXPECT_FALSE(watcher.refresh());
}

TEST(FullscreenWatcher, RefreshReportsChangeWhenFullscreenAppCloses) {
    MockFullscreenApi api;
    ForegroundWindowInfo notFullscreen = fullscreenAppInfo();
    notFullscreen.windowRect = Rect{0, 0, 1280, 720};

    {
        testing::InSequence sequence;
        EXPECT_CALL(api, queryForegroundWindow()).WillOnce(Return(fullscreenAppInfo()));
        EXPECT_CALL(api, queryForegroundWindow()).WillOnce(Return(notFullscreen));
    }

    FullscreenWatcher watcher(api);
    ASSERT_TRUE(watcher.refresh());
    ASSERT_TRUE(watcher.isFullscreenActive());

    EXPECT_TRUE(watcher.refresh());
    EXPECT_FALSE(watcher.isFullscreenActive());
}
