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

// Abstracts the registry calls behind Windows startup registration
// (HKCU\...\Run) so Autostart's enable/disable/isEnabled decisions are
// unit-testable without touching the real registry — see
// Win32RegistryApi for the real implementation.
class IRegistryApi {
   public:
    virtual ~IRegistryApi() = default;

    // Reads the Run value; returns false (leaving *outCommand untouched)
    // if it doesn't exist.
    virtual bool getRunValue(std::wstring* outCommand) const = 0;
    virtual bool setRunValue(const std::wstring& command) const = 0;
    virtual bool deleteRunValue() const = 0;
};

// Registers/unregisters Umbra to launch on Windows startup, per the PRD's
// "automatically start with Windows" requirement.
class Autostart {
   public:
    // api must outlive this Autostart. exeCommand is the exact command
    // line to register (the running executable's own path, quoted as
    // needed) — isEnabled() compares the stored Run value against this
    // exact string, so a stale entry left over from a different install
    // path/location correctly reports as not enabled rather than a false
    // positive.
    Autostart(const IRegistryApi& api, std::wstring exeCommand);

    bool isEnabled() const;
    bool enable();
    bool disable();

   private:
    const IRegistryApi& api_;
    std::wstring exeCommand_;
};

}  // namespace umbra
