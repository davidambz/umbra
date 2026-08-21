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

#include "app/autostart.h"

#include <gtest/gtest.h>

#include "mocks/win32_mocks.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using umbra::Autostart;
using umbra::testing_support::MockRegistryApi;

TEST(Autostart, IsEnabledWhenStoredValueMatchesExeCommand) {
    MockRegistryApi api;
    EXPECT_CALL(api, getRunValue(_))
        .WillOnce(DoAll(SetArgPointee<0>(L"\"C:\\Umbra\\umbra.exe\""), Return(true)));

    Autostart autostart(api, L"\"C:\\Umbra\\umbra.exe\"");
    EXPECT_TRUE(autostart.isEnabled());
}

TEST(Autostart, IsNotEnabledWhenNoValueIsStored) {
    MockRegistryApi api;
    EXPECT_CALL(api, getRunValue(_)).WillOnce(Return(false));

    Autostart autostart(api, L"\"C:\\Umbra\\umbra.exe\"");
    EXPECT_FALSE(autostart.isEnabled());
}

TEST(Autostart, IsNotEnabledWhenStoredValueIsAStaleDifferentPath) {
    MockRegistryApi api;
    EXPECT_CALL(api, getRunValue(_))
        .WillOnce(DoAll(SetArgPointee<0>(L"\"D:\\OldLocation\\umbra.exe\""), Return(true)));

    Autostart autostart(api, L"\"C:\\Umbra\\umbra.exe\"");
    EXPECT_FALSE(autostart.isEnabled());
}

TEST(Autostart, EnableWritesTheExeCommand) {
    MockRegistryApi api;
    EXPECT_CALL(api, setRunValue(std::wstring(L"\"C:\\Umbra\\umbra.exe\""))).WillOnce(Return(true));

    Autostart autostart(api, L"\"C:\\Umbra\\umbra.exe\"");
    EXPECT_TRUE(autostart.enable());
}

TEST(Autostart, DisableDeletesTheRunValue) {
    MockRegistryApi api;
    EXPECT_CALL(api, deleteRunValue()).WillOnce(Return(true));

    Autostart autostart(api, L"\"C:\\Umbra\\umbra.exe\"");
    EXPECT_TRUE(autostart.disable());
}
