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

#include "desktop/lock_screen_api.h"

namespace umbra {

// Real ILockScreenApi backed by the WinRT
// Windows.System.UserProfile.LockScreen API — the same broker the Photos
// app's "Set as lock screen" uses, so it works for a normal signed-in user
// without needing admin rights. Windows-only, verified manually against a
// live desktop session (see TESTING.md).
class Win32LockScreenApi : public ILockScreenApi {
   public:
    bool setLockScreenImage(const std::wstring& imagePath) const override;
};

}  // namespace umbra
