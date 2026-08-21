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

#include "app/autostart.h"

namespace umbra {

// Real IRegistryApi backed by HKCU\Software\Microsoft\Windows\
// CurrentVersion\Run, value name "Umbra". Windows-only, verified manually
// against a live desktop session (see TESTING.md) — Autostart's own
// enable/disable/isEnabled decisions are what's unit-tested, via a mock
// of this interface.
class Win32RegistryApi : public IRegistryApi {
   public:
    bool getRunValue(std::wstring* outCommand) const override;
    bool setRunValue(const std::wstring& command) const override;
    bool deleteRunValue() const override;
};

}  // namespace umbra
