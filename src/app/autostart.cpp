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

namespace umbra {

Autostart::Autostart(const IRegistryApi& api, std::wstring exeCommand)
    : api_(api), exeCommand_(std::move(exeCommand)) {}

bool Autostart::isEnabled() const {
    std::wstring stored;
    return api_.getRunValue(&stored) && stored == exeCommand_;
}

bool Autostart::enable() {
    // An empty exeCommand_ means the caller couldn't resolve the running
    // executable's own path (e.g. GetModuleFileNameW failed) — writing it
    // anyway would register a broken/empty autostart entry instead of
    // just not enabling autostart.
    if (exeCommand_.empty()) {
        return false;
    }
    return api_.setRunValue(exeCommand_);
}

bool Autostart::disable() { return api_.deleteRunValue(); }

}  // namespace umbra
