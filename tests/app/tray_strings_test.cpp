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

// Regression coverage for the exact bug #103/#78 already hit once with a
// literal (non-\u-escaped) Chinese character in a test: every diacritic-
// bearing Latin string here was ALSO written as a literal character
// rather than escaped, and never had a test checking it specifically
// (the tests above only ever compared ASCII-only fields like quit/
// resume) — so if MSVC's non-BOM source-encoding handling mangled these
// too, nothing would have caught it. \u-escaped here so this test itself
// can't reintroduce the same bug it's checking for.
TEST(TrayStrings, DiacriticBearingStringsSurviveIntact) {
    // "Abrir Configura\u00E7\u00F5es" / "Abrir configuraci\u00F3n" /
    // "Ouvrir les param\u00E8tres"
    EXPECT_EQ(std::wstring(trayStringsFor("pt-BR").openSettings), L"Abrir Configura\u00E7\u00F5es");
    EXPECT_EQ(std::wstring(trayStringsFor("es").openSettings), L"Abrir configuraci\u00F3n");
    EXPECT_EQ(std::wstring(trayStringsFor("fr").openSettings), L"Ouvrir les param\u00E8tres");
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
    // \u9000\u51FA is the Chinese word for "quit" -- \u-escaped, not a
    // literal character, for the same reason tray_strings.cpp's own tables
    // are: a raw non-ASCII literal in this file previously got mangled by
    // MSVC's non-BOM source-encoding handling, failing this exact test on
    // Windows while passing on Linux/GCC.
    const std::wstring kQuit = L"\u9000\u51FA";
    EXPECT_EQ(std::wstring(trayStringsFor("zh").quit), kQuit);
    EXPECT_EQ(std::wstring(trayStringsFor("zh-CN").quit), kQuit);
    EXPECT_EQ(std::wstring(trayStringsFor("zh-SG").quit), kQuit);
    EXPECT_EQ(std::wstring(trayStringsFor("zh-Hans-CN").quit), kQuit);
}

TEST(TrayStrings, DoesNotRenderSimplifiedChineseToATraditionalChineseReader) {
    // zh-TW/zh-HK/zh-Hant aren't shipped (only Simplified is) — this must
    // fall back to English, not to the Simplified table via a plain
    // base-language ("zh") match.
    EXPECT_EQ(std::wstring(trayStringsFor("zh-TW").quit), L"Quit");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-HK").quit), L"Quit");
    EXPECT_EQ(std::wstring(trayStringsFor("zh-Hant").quit), L"Quit");
}

TEST(TrayStrings, EveryShippedLocaleHasEveryNonEmptyString) {
    for (const char* locale : {"en", "pt-BR", "es", "zh-CN", "fr", "ru", "ja", "ko"}) {
        const umbra::TrayStrings& tray = trayStringsFor(locale);
        EXPECT_GT(std::wstring(tray.openSettings).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.pauseAll).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.resume).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.quit).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.checkForUpdates).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.upToDate).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.updateCheckFailed).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.updateAvailableTitle).size(), 0u) << locale;
        EXPECT_GT(std::wstring(tray.updateInstalling).size(), 0u) << locale;
    }
}
