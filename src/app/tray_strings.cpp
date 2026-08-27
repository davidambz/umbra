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

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace umbra {

namespace {

// Every non-ASCII string below is written as \u-escaped code points rather
// than literal characters: MSVC only reliably treats a non-ASCII source
// file as UTF-8 when it carries a BOM or the project passes /utf-8, neither
// of which this file can guarantee -- a \u escape is parsed by the
// compiler independent of the source file's own encoding, so it can't
// silently turn into mojibake the way a raw literal could. The comment
// above each table spells out what it actually says in plain text, since
// the escaped literal itself isn't readable.

constexpr TrayStrings kEnglish{
    .openSettings = L"Open Settings",
    .pauseAll = L"Pause all",
    .resume = L"Resume",
    .quit = L"Quit",
};

// "Abrir Configurações" / "Pausar tudo" / "Retomar" / "Sair"
constexpr TrayStrings kPortugueseBR{
    .openSettings = L"Abrir Configura\u00E7\u00F5es",
    .pauseAll = L"Pausar tudo",
    .resume = L"Retomar",
    .quit = L"Sair",
};

// "Abrir configuración" / "Pausar todo" / "Reanudar" / "Salir"
constexpr TrayStrings kSpanish{
    .openSettings = L"Abrir configuraci\u00F3n",
    .pauseAll = L"Pausar todo",
    .resume = L"Reanudar",
    .quit = L"Salir",
};

// "打开设置" / "全部暂停" / "恢复" / "退出"
constexpr TrayStrings kChineseSimplified{
    .openSettings = L"\u6253\u5F00\u8BBE\u7F6E",
    .pauseAll = L"\u5168\u90E8\u6682\u505C",
    .resume = L"\u6062\u590D",
    .quit = L"\u9000\u51FA",
};

// "Ouvrir les paramètres" / "Tout mettre en pause" / "Reprendre" / "Quitter"
constexpr TrayStrings kFrench{
    .openSettings = L"Ouvrir les param\u00E8tres",
    .pauseAll = L"Tout mettre en pause",
    .resume = L"Reprendre",
    .quit = L"Quitter",
};

// "Открыть настройки" / "Приостановить всё" / "Возобновить" / "Выход"
constexpr TrayStrings kRussian{
    .openSettings =
        L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C "
        L"\u043D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438",
    .pauseAll =
        L"\u041F\u0440\u0438\u043E\u0441\u0442\u0430\u043D\u043E\u0432\u0438\u0442\u044C "
        L"\u0432\u0441\u0451",
    .resume = L"\u0412\u043E\u0437\u043E\u0431\u043D\u043E\u0432\u0438\u0442\u044C",
    .quit = L"\u0412\u044B\u0445\u043E\u0434",
};

// "設定を開く" / "すべて一時停止" / "再開" / "終了"
constexpr TrayStrings kJapanese{
    .openSettings = L"\u8A2D\u5B9A\u3092\u958B\u304F",
    .pauseAll = L"\u3059\u3079\u3066\u4E00\u6642\u505C\u6B62",
    .resume = L"\u518D\u958B",
    .quit = L"\u7D42\u4E86",
};

// "설정 열기" / "모두 일시정지" / "재개" / "종료"
constexpr TrayStrings kKorean{
    .openSettings = L"\uC124\uC815 \uC5F4\uAE30",
    .pauseAll = L"\uBAA8\uB450 \uC77C\uC2DC\uC815\uC9C0",
    .resume = L"\uC7AC\uAC1C",
    .quit = L"\uC885\uB8CC",
};

// Kept as an array of (tag, table) pairs rather than a std::map -- the set
// is small and fixed, and this keeps every table a constexpr TrayStrings
// with no static-initialization-order concerns.
//
// This set of tags is hand-duplicated in two other places with no way to
// share a single source of truth across languages: settings-ui/src/types.ts's
// Locale type and src/ui/ui_bridge.cpp's isKnownLanguageOverride. Adding or
// removing a shipped language needs the same change in all three, or this
// table falls behind and the tray menu silently falls back to English for a
// language settings-ui shows correctly.
constexpr std::array<std::pair<const char*, const TrayStrings*>, 8> kTables{{
    {"en", &kEnglish},
    {"pt-BR", &kPortugueseBR},
    {"es", &kSpanish},
    {"zh-CN", &kChineseSimplified},
    {"fr", &kFrench},
    {"ru", &kRussian},
    {"ja", &kJapanese},
    {"ko", &kKorean},
}};

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) ==
                      std::tolower(static_cast<unsigned char>(y));
           });
}

std::string baseLanguage(const std::string& tag) {
    const size_t separator = tag.find_first_of("-_");
    return separator == std::string::npos ? tag : tag.substr(0, separator);
}

std::string toLower(const std::string& value) {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

}  // namespace

const TrayStrings& trayStringsFor(const std::string& locale) {
    for (const auto& [tag, table] : kTables) {
        if (equalsIgnoreCase(locale, tag)) {
            return *table;
        }
    }

    // Chinese is a deliberate exception to plain base-language matching:
    // "zh" alone covers both Simplified and Traditional, and the only
    // Chinese table shipped here is Simplified (zh-CN) — matching
    // "zh-TW"/"zh-HK"/"zh-Hant" to it would render the wrong script to a
    // Traditional-Chinese reader instead of falling back to English. Only
    // tags that are actually Simplified (zh, zh-CN, zh-SG, or an explicit
    // -Hans- subtag) match zh-CN; mirrors resolveSupportedLocale's own
    // exception in settings-ui/src/i18n/index.ts.
    const std::string lowerLocale = toLower(locale);
    if (lowerLocale == "zh" || lowerLocale == "zh-cn" || lowerLocale == "zh-sg" ||
        lowerLocale.find("-hans") != std::string::npos) {
        return kChineseSimplified;
    }
    if (lowerLocale.rfind("zh", 0) == 0) {
        return kEnglish;
    }

    const std::string language = baseLanguage(locale);
    for (const auto& [tag, table] : kTables) {
        if (equalsIgnoreCase(baseLanguage(tag), language)) {
            return *table;
        }
    }

    return kEnglish;
}

}  // namespace umbra
