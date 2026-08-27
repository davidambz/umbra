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

#include <optional>
#include <string>
#include <tuple>

namespace umbra {

// A parsed X.Y.Z version — see #37/#78. Deliberately just three ints, not
// a general SemVer parser: every tag this repo's release.yml accepts
// already matches ^v[0-9]+\.[0-9]+\.[0-9]+$, so there's no pre-release/
// build-metadata suffix to handle.
using Version = std::tuple<int, int, int>;

// Parses "X.Y.Z" or "vX.Y.Z" (a leading 'v', as GitHub Releases' tag_name
// carries, is stripped) into a Version. Returns std::nullopt for anything
// that doesn't match that shape.
std::optional<Version> parseVersion(const std::string& text);

// True if candidate is a strictly newer version than current — used to
// decide whether a GitHub Release's tag is actually an update worth
// offering, not e.g. the version already running or an older one.
bool isNewerVersion(const Version& current, const Version& candidate);

}  // namespace umbra
