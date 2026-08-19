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

#include "desktop/win32_workerw_api.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace umbra {

namespace {

// The undocumented message that makes Progman spawn a WorkerW window
// behind the desktop icons. See ARCHITECTURE.md for the full sequence.
constexpr UINT kSpawnWorkerWMessage = 0x052C;

BOOL CALLBACK findBackgroundWorkerWCallback(HWND hwnd, LPARAM userData) {
    // The WorkerW hosting the desktop icons has a SHELLDLL_DefView child;
    // the invisible WorkerW spawned by kSpawnWorkerWMessage — the one we
    // want to render behind the icons — is its next z-order sibling.
    if (::FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr) == nullptr) {
        return TRUE;
    }

    auto* out = reinterpret_cast<HWND*>(userData);
    *out = ::FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
    return FALSE;
}

}  // namespace

WindowHandle Win32WorkerWApi::findWindowByClass(const char* className) const {
    return ::FindWindowA(className, nullptr);
}

void Win32WorkerWApi::sendSpawnWorkerWMessage(WindowHandle progman) const {
    ULONG_PTR result = 0;
    ::SendMessageTimeoutW(static_cast<HWND>(progman), kSpawnWorkerWMessage, 0, 0, SMTO_NORMAL, 1000,
                          &result);
}

WindowHandle Win32WorkerWApi::findBackgroundWorkerW() const {
    HWND workerW = nullptr;
    ::EnumWindows(findBackgroundWorkerWCallback, reinterpret_cast<LPARAM>(&workerW));
    return workerW;
}

bool Win32WorkerWApi::setParent(WindowHandle child, WindowHandle parent) const {
    return ::SetParent(static_cast<HWND>(child), static_cast<HWND>(parent)) != nullptr;
}

}  // namespace umbra
