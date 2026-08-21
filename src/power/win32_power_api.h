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

#include "power/power_watcher.h"

namespace umbra {

// Real IPowerApi backed by GetSystemPowerStatus(). Windows-only, verified
// manually against a live desktop session (see TESTING.md) — PowerWatcher's
// own decision logic is what's unit-tested, via a mock of this interface.
class Win32PowerApi : public IPowerApi {
   public:
    PowerState queryState() const override;
};

}  // namespace umbra
