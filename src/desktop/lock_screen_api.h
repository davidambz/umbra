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

// Abstracts the WinRT lock-screen broker call behind
// Windows::System::UserProfile::LockScreen::SetImageFileAsync (see
// Win32LockScreenApi). This app doesn't target enterprise/MDM-managed
// machines, so a policy-locked lock screen refusing the change is an
// expected outcome, not an error to surface to the user — every caller
// treats a false return as "nothing happened" and moves on silently.
class ILockScreenApi {
   public:
    virtual ~ILockScreenApi() = default;

    // Sets imagePath (an absolute path to an existing image file) as the
    // Windows lock screen background. Returns false on any failure,
    // including a lock screen managed by policy.
    virtual bool setLockScreenImage(const std::wstring& imagePath) const = 0;
};

}  // namespace umbra
