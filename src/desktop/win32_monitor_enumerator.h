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

#include "desktop/monitor_manager.h"

namespace umbra {

// Real IMonitorEnumerator backed by EnumDisplayMonitors/GetMonitorInfoW.
// Windows-only — verified manually against a live desktop session (see
// TESTING.md), since faking monitor hot-plug convincingly isn't worth it.
class Win32MonitorEnumerator : public IMonitorEnumerator {
   public:
    std::vector<MonitorInfo> enumerate() const override;
};

}  // namespace umbra
