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

#include "app/tray_strings.h"

#include <gtest/gtest.h>

using umbra::trayStringsFor;

TEST(TrayStrings, MatchesAShippedLocaleExactly) {
    EXPECT_EQ(std::wstring(trayStringsFor("pt-BR").quit), L"Sair");
    EXPECT_EQ(std::wstring(trayStringsFor("fr").quit), L"Quitter");
}

TEST(TrayStrings, MatchIsCaseInsensitive) {
    EXPECT_EQ(std::wstring(trayStringsFor("PT-br").quit), L"Sair");
    EXPECT_EQ(std::wstring(trayStringsFor("EN").quit), L"Quit");
}

TEST(TrayStrings, FallsBackToTheBaseLanguageWhenTheRegionDoesNotMatch) {
    // "pt-PT" isn't shipped (only "pt-BR" is) — falls back to the one
    // Portuguese table we do have rather than straight to English.
    EXPECT_EQ(std::wstring(trayStringsFor("pt-PT").quit), L"Sair");
    // "en-GB"/"en-US" aren't shipped verbatim, but "en" is.
    EXPECT_EQ(std::wstring(trayStringsFor("en-GB").quit), L"Quit");
    EXPECT_EQ(std::wstring(trayStringsFor("en-US").quit), L"Quit");
}

TEST(TrayStrings, FallsBackToEnglishForAnUnshippedLanguage) {
    EXPECT_EQ(std::wstring(trayStringsFor("de-DE").openSettings), L"Open Settings");
    EXPECT_EQ(std::wstring(trayStringsFor("").openSettings), L"Open Settings");
}

TEST(TrayStrings, MatchesSimplifiedChineseTagsToTheSimplifiedTable) {
    EXPECT_EQ(std::wstring(trayStringsFor("zh").quit), L"退出");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-CN").quit), L"退出");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-SG").quit), L"退出");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-Hans-CN").quit), L"退出");
}

TEST(TrayStrings, DoesNotRenderSimplifiedChineseToATraditionalChineseReader) {
    // zh-TW/zh-HK/zh-Hant aren't shipped (only Simplified is) — this must
    // fall back to English, not to the Simplified table via a plain
    // base-language ("zh") match.
    EXPECT_EQ(std::wstring(trayStringsFor("zh-TW").quit), L"Quit");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-HK").quit), L"Quit");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-Hant").quit), L"Quit");
}

TEST(TrayStrings, EveryShippedLocaleHasAllFourNonEmptyStrings) {
    for (const char* locale : {"en", "pt-BR", "es", "zh-CN", "fr", "ru", "ja", "ko"}) {
        const umbra::TrayStrings& tray = trayStringsFor(locale);
        EXPECT_GT(std::wstring(tray.openSettings).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.pauseAll).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.resume).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.quit).size(), 0u) << locale;
    }
}
