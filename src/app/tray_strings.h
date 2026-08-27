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

#include <string>

namespace umbra {

// The tray context menu's strings (application.cpp's AppendMenuW calls) —
// the native-side half of #95's i18n work, mirroring settings-ui/src/i18n's
// locale files but kept as its own small table since the tray menu has no
// dependency on the WebView2/settings-ui bundle at all.
struct TrayStrings {
    const wchar_t* openSettings;
    const wchar_t* pauseAll;
    const wchar_t* resume;
    const wchar_t* quit;
};

// locale should already be resolved against languageOverride (see
// UiBridge::resolveLanguage) — a raw OS/override tag, e.g. "pt-BR", "en-US",
// "zh-Hans-CN". Matches a shipped locale exactly first, then by base
// language (so "pt-PT" or "fr-CA" still find a shipped table), and falls
// back to English for anything else — the same fallback contract
// settings-ui/src/i18n's resolveSupportedLocale uses.
const TrayStrings& trayStringsFor(const std::string& locale);

}  // namespace umbra
