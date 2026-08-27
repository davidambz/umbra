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
// silently turn into mojibake the way a raw literal could. This applies to
// every locale here, not just the ones using a non-Latin script -- a
// previous version of this file left the Latin-script locales (pt-BR, es,
// fr) as literal accented characters, on the mistaken assumption that only
// scripts like Chinese/Japanese/Korean/Cyrillic were at risk; nothing
// caught it because the tests only ever compared ASCII-only fields for
// those locales (see tray_strings_test.cpp's
// DiacriticBearingStringsSurviveIntact). The comment above each non-English
// table spells out what it actually says in plain text, since the escaped
// literal itself isn't readable.

constexpr TrayStrings kEnglish{
    .openSettings = L"Open Settings",
    .pauseAll = L"Pause all",
    .resume = L"Resume",
    .quit = L"Quit",
    .checkForUpdates = L"Check for updates",
    .upToDate = L"Umbra is up to date.",
    .updateCheckFailed = L"Couldn't check for updates \u2014 check your connection and try again.",
    .updateAvailableTitle = L"Umbra update available",
    .updateInstalling = L"Installing the update \u2014 Umbra will restart automatically.",
};

// "Abrir Configurações" / "Pausar tudo" / "Retomar" / "Sair" / "Verificar atualizações" / "O Umbra
// está atualizado." / "Não foi possível verificar atualizações — verifique a conexão e tente
// novamente." / "Atualização do Umbra disponível" / "Instalando a atualização — o Umbra vai
// reiniciar automaticamente."
constexpr TrayStrings kPortugueseBR{
    .openSettings = L"Abrir Configura\u00E7\u00F5es",
    .pauseAll = L"Pausar tudo",
    .resume = L"Retomar",
    .quit = L"Sair",
    .checkForUpdates = L"Verificar atualiza\u00E7\u00F5es",
    .upToDate = L"O Umbra est\u00E1 atualizado.",
    .updateCheckFailed =
        L"N\u00E3o foi poss\u00EDvel verificar atualiza\u00E7\u00F5es \u2014 verifique a "
        L"conex\u00E3o e tente novamente.",
    .updateAvailableTitle = L"Atualiza\u00E7\u00E3o do Umbra dispon\u00EDvel",
    .updateInstalling =
        L"Instalando a atualiza\u00E7\u00E3o \u2014 o Umbra vai reiniciar automaticamente.",
};

// "Abrir configuración" / "Pausar todo" / "Reanudar" / "Salir" / "Buscar actualizaciones" / "Umbra
// está actualizado." / "No se pudieron buscar actualizaciones — revisa la conexión e inténtalo de
// nuevo." / "Actualización de Umbra disponible" / "Instalando la actualización — Umbra se
// reiniciará automáticamente."
constexpr TrayStrings kSpanish{
    .openSettings = L"Abrir configuraci\u00F3n",
    .pauseAll = L"Pausar todo",
    .resume = L"Reanudar",
    .quit = L"Salir",
    .checkForUpdates = L"Buscar actualizaciones",
    .upToDate = L"Umbra est\u00E1 actualizado.",
    .updateCheckFailed =
        L"No se pudieron buscar actualizaciones \u2014 revisa la conexi\u00F3n e int\u00E9ntalo de "
        L"nuevo.",
    .updateAvailableTitle = L"Actualizaci\u00F3n de Umbra disponible",
    .updateInstalling =
        L"Instalando la actualizaci\u00F3n \u2014 Umbra se reiniciar\u00E1 autom\u00E1ticamente.",
};

// "打开设置" / "全部暂停" / "恢复" / "退出" / "检查更新" / "Umbra 已是最新版本。" / "无法检查更新 —
// 请检查网络连接后重试。" / "有可用的 Umbra 更新" / "正在安装更新 — Umbra 将自动重启。"
constexpr TrayStrings kChineseSimplified{
    .openSettings = L"\u6253\u5F00\u8BBE\u7F6E",
    .pauseAll = L"\u5168\u90E8\u6682\u505C",
    .resume = L"\u6062\u590D",
    .quit = L"\u9000\u51FA",
    .checkForUpdates = L"\u68C0\u67E5\u66F4\u65B0",
    .upToDate = L"Umbra \u5DF2\u662F\u6700\u65B0\u7248\u672C\u3002",
    .updateCheckFailed =
        L"\u65E0\u6CD5\u68C0\u67E5\u66F4\u65B0 \u2014 "
        L"\u8BF7\u68C0\u67E5\u7F51\u7EDC\u8FDE\u63A5\u540E\u91CD\u8BD5\u3002",
    .updateAvailableTitle = L"\u6709\u53EF\u7528\u7684 Umbra \u66F4\u65B0",
    .updateInstalling =
        L"\u6B63\u5728\u5B89\u88C5\u66F4\u65B0 \u2014 Umbra \u5C06\u81EA\u52A8\u91CD\u542F\u3002",
};

// "Ouvrir les paramètres" / "Tout mettre en pause" / "Reprendre" / "Quitter" / "Vérifier les mises
// à jour" / "Umbra est à jour." / "Impossible de vérifier les mises à jour — vérifiez la connexion
// et réessayez." / "Mise à jour d'Umbra disponible" / "Installation de la mise à jour — Umbra
// redémarrera automatiquement."
constexpr TrayStrings kFrench{
    .openSettings = L"Ouvrir les param\u00E8tres",
    .pauseAll = L"Tout mettre en pause",
    .resume = L"Reprendre",
    .quit = L"Quitter",
    .checkForUpdates = L"V\u00E9rifier les mises \u00E0 jour",
    .upToDate = L"Umbra est \u00E0 jour.",
    .updateCheckFailed =
        L"Impossible de v\u00E9rifier les mises \u00E0 jour \u2014 v\u00E9rifiez la connexion et "
        L"r\u00E9essayez.",
    .updateAvailableTitle = L"Mise \u00E0 jour d'Umbra disponible",
    .updateInstalling =
        L"Installation de la mise \u00E0 jour \u2014 Umbra red\u00E9marrera automatiquement.",
};

// "Открыть настройки" / "Приостановить всё" / "Возобновить" / "Выход" / "Проверить обновления" /
// "Umbra обновлена до последней версии." / "Не удалось проверить обновления — проверьте подключение
// и попробуйте снова." / "Доступно обновление Umbra" / "Установка обновления — Umbra перезапустится
// автоматически."
constexpr TrayStrings kRussian{
    .openSettings =
        L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C "
        L"\u043D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438",
    .pauseAll =
        L"\u041F\u0440\u0438\u043E\u0441\u0442\u0430\u043D\u043E\u0432\u0438\u0442\u044C "
        L"\u0432\u0441\u0451",
    .resume = L"\u0412\u043E\u0437\u043E\u0431\u043D\u043E\u0432\u0438\u0442\u044C",
    .quit = L"\u0412\u044B\u0445\u043E\u0434",
    .checkForUpdates =
        L"\u041F\u0440\u043E\u0432\u0435\u0440\u0438\u0442\u044C "
        L"\u043E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u044F",
    .upToDate =
        L"Umbra \u043E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0430 \u0434\u043E "
        L"\u043F\u043E\u0441\u043B\u0435\u0434\u043D\u0435\u0439 "
        L"\u0432\u0435\u0440\u0441\u0438\u0438.",
    .updateCheckFailed =
        L"\u041D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C "
        L"\u043F\u0440\u043E\u0432\u0435\u0440\u0438\u0442\u044C "
        L"\u043E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u044F \u2014 "
        L"\u043F\u0440\u043E\u0432\u0435\u0440\u044C\u0442\u0435 "
        L"\u043F\u043E\u0434\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u0435 \u0438 "
        L"\u043F\u043E\u043F\u0440\u043E\u0431\u0443\u0439\u0442\u0435 "
        L"\u0441\u043D\u043E\u0432\u0430.",
    .updateAvailableTitle =
        L"\u0414\u043E\u0441\u0442\u0443\u043F\u043D\u043E "
        L"\u043E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u0435 Umbra",
    .updateInstalling =
        L"\u0423\u0441\u0442\u0430\u043D\u043E\u0432\u043A\u0430 "
        L"\u043E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u044F \u2014 Umbra "
        L"\u043F\u0435\u0440\u0435\u0437\u0430\u043F\u0443\u0441\u0442\u0438\u0442\u0441\u044F "
        L"\u0430\u0432\u0442\u043E\u043C\u0430\u0442\u0438\u0447\u0435\u0441\u043A\u0438.",
};

// "設定を開く" / "すべて一時停止" / "再開" / "終了" / "アップデートを確認" / "Umbra
// は最新の状態です。" / "アップデートを確認できませんでした —
// 接続を確認してもう一度お試しください。" / "Umbra のアップデートがあります" /
// "アップデートをインストールしています — Umbra は自動的に再起動します。"
constexpr TrayStrings kJapanese{
    .openSettings = L"\u8A2D\u5B9A\u3092\u958B\u304F",
    .pauseAll = L"\u3059\u3079\u3066\u4E00\u6642\u505C\u6B62",
    .resume = L"\u518D\u958B",
    .quit = L"\u7D42\u4E86",
    .checkForUpdates = L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u3092\u78BA\u8A8D",
    .upToDate = L"Umbra \u306F\u6700\u65B0\u306E\u72B6\u614B\u3067\u3059\u3002",
    .updateCheckFailed =
        L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u3092\u78BA\u8A8D\u3067\u304D\u307E\u305B\u3093"
        L"\u3067\u3057\u305F \u2014 "
        L"\u63A5\u7D9A\u3092\u78BA\u8A8D\u3057\u3066\u3082\u3046\u4E00\u5EA6\u304A\u8A66\u3057"
        L"\u304F\u3060\u3055\u3044\u3002",
    .updateAvailableTitle =
        L"Umbra \u306E\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u304C\u3042\u308A\u307E\u3059",
    .updateInstalling =
        L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u3092\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3057"
        L"\u3066\u3044\u307E\u3059 \u2014 Umbra "
        L"\u306F\u81EA\u52D5\u7684\u306B\u518D\u8D77\u52D5\u3057\u307E\u3059\u3002",
};

// "설정 열기" / "모두 일시정지" / "재개" / "종료" / "업데이트 확인" / "Umbra가 최신 상태입니다." /
// "업데이트를 확인하지 못했습니다 — 연결을 확인한 후 다시 시도하세요." / "Umbra 업데이트 사용 가능"
// / "업데이트를 설치하는 중입니다 — Umbra가 자동으로 재시작됩니다."
constexpr TrayStrings kKorean{
    .openSettings = L"\uC124\uC815 \uC5F4\uAE30",
    .pauseAll = L"\uBAA8\uB450 \uC77C\uC2DC\uC815\uC9C0",
    .resume = L"\uC7AC\uAC1C",
    .quit = L"\uC885\uB8CC",
    .checkForUpdates = L"\uC5C5\uB370\uC774\uD2B8 \uD655\uC778",
    .upToDate = L"Umbra\uAC00 \uCD5C\uC2E0 \uC0C1\uD0DC\uC785\uB2C8\uB2E4.",
    .updateCheckFailed =
        L"\uC5C5\uB370\uC774\uD2B8\uB97C \uD655\uC778\uD558\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4 "
        L"\u2014 \uC5F0\uACB0\uC744 \uD655\uC778\uD55C \uD6C4 \uB2E4\uC2DC "
        L"\uC2DC\uB3C4\uD558\uC138\uC694.",
    .updateAvailableTitle = L"Umbra \uC5C5\uB370\uC774\uD2B8 \uC0AC\uC6A9 \uAC00\uB2A5",
    .updateInstalling =
        L"\uC5C5\uB370\uC774\uD2B8\uB97C \uC124\uCE58\uD558\uB294 \uC911\uC785\uB2C8\uB2E4 \u2014 "
        L"Umbra\uAC00 \uC790\uB3D9\uC73C\uB85C \uC7AC\uC2DC\uC791\uB429\uB2C8\uB2E4.",
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
    // Chinese table shipped here is Simplified (zh-CN) -- matching
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
