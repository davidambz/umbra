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

#include "desktop/win32_lock_screen_api.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.UserProfile.h>

namespace umbra {

bool Win32LockScreenApi::setLockScreenImage(const std::wstring& imagePath) const {
    using winrt::Windows::Storage::StorageFile;
    using winrt::Windows::System::UserProfile::LockScreen;

    // The caller (LockScreenSync's background sync thread) initializes its
    // own COM apartment as COINIT_MULTITHREADED before calling this — not
    // winrt::init_apartment(), which would try to (re-)initialize the same
    // thread's apartment a second time. Blocking on .get() here is safe
    // specifically because that thread has no window/message queue of its
    // own to deadlock: MTA threads don't need message pumping while
    // blocked the way an STA thread would.
    try {
        StorageFile file = StorageFile::GetFileFromPathAsync(imagePath).get();
        LockScreen::SetImageFileAsync(file).get();
        return true;
    } catch (const winrt::hresult_error&) {
        // Expected on a lock screen locked down by an MDM/domain policy —
        // see ILockScreenApi's contract — as well as on a plain missing
        // file if the caller's capture step silently failed upstream.
        return false;
    }
}

}  // namespace umbra
