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

#include "app/version_compare.h"

#include <cctype>
#include <sstream>

namespace umbra {

namespace {

// Parses one "N" component starting at pos, requiring at least one digit
// and no leading/trailing garbage before the next '.' or the string's end.
// Returns std::nullopt on anything else (empty, non-digit, leading zero
// handled fine since atoi-style parsing doesn't care about that).
std::optional<int> parseComponent(const std::string& text, size_t start, size_t end) {
    if (start >= end) {
        return std::nullopt;
    }
    for (size_t i = start; i < end; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return std::nullopt;
        }
    }
    try {
        return std::stoi(text.substr(start, end - start));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

std::optional<Version> parseVersion(const std::string& text) {
    const std::string body = (!text.empty() && text[0] == 'v') ? text.substr(1) : text;

    const size_t firstDot = body.find('.');
    if (firstDot == std::string::npos) {
        return std::nullopt;
    }
    const size_t secondDot = body.find('.', firstDot + 1);
    if (secondDot == std::string::npos || body.find('.', secondDot + 1) != std::string::npos) {
        return std::nullopt;
    }

    const std::optional<int> major = parseComponent(body, 0, firstDot);
    const std::optional<int> minor = parseComponent(body, firstDot + 1, secondDot);
    const std::optional<int> patch = parseComponent(body, secondDot + 1, body.size());
    if (!major || !minor || !patch) {
        return std::nullopt;
    }
    return Version{*major, *minor, *patch};
}

bool isNewerVersion(const Version& current, const Version& candidate) {
    return candidate > current;
}

}  // namespace umbra
