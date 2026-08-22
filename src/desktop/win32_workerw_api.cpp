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

// True if hwnd's own window class is exactly className. Windows-defined
// class names (WorkerW, SHELLDLL_DefView, Progman, ...) are short and
// fixed by the shell itself, well under this buffer's size.
bool windowClassNameIs(HWND hwnd, const wchar_t* className) {
    wchar_t actual[64];
    return ::GetClassNameW(hwnd, actual, ARRAYSIZE(actual)) > 0 && wcscmp(actual, className) == 0;
}

BOOL CALLBACK hasDefViewDescendantCallback(HWND hwnd, LPARAM userData) {
    if (windowClassNameIs(hwnd, L"SHELLDLL_DefView")) {
        *reinterpret_cast<bool*>(userData) = true;
        return FALSE;  // found it, stop enumerating
    }
    return TRUE;
}

// SHELLDLL_DefView isn't always a *direct* child of the WorkerW that hosts
// it — on some shell versions it's nested a level or two deeper — so this
// walks the full descendant tree via EnumChildWindows rather than a single
// FindWindowExW direct-child lookup.
bool hasDefViewDescendant(HWND hwnd) {
    bool found = false;
    ::EnumChildWindows(hwnd, hasDefViewDescendantCallback, reinterpret_cast<LPARAM>(&found));
    return found;
}

BOOL CALLBACK findBackgroundWorkerWCallback(HWND hwnd, LPARAM userData) {
    // Only a WorkerW hosting the desktop icons is a valid match here — on
    // some builds SHELLDLL_DefView ends up nested directly under Progman
    // instead (see issue #20), and Progman must never match this search:
    // "the next WorkerW after Progman in z-order" would then resolve to
    // an unrelated, wrong WorkerW (Windows keeps several small unrelated
    // ones around for other purposes) rather than correctly reporting
    // "not found". Progman's own class is "Progman", never "WorkerW", so
    // this check excludes it without needing a separate explicit guard.
    if (!windowClassNameIs(hwnd, L"WorkerW")) {
        return TRUE;
    }

    // The WorkerW hosting the desktop icons has a SHELLDLL_DefView
    // descendant; the invisible WorkerW spawned by kSpawnWorkerWMessage —
    // the one we want to render behind the icons — is its next z-order
    // sibling.
    if (!hasDefViewDescendant(hwnd)) {
        return TRUE;
    }

    auto* out = reinterpret_cast<HWND*>(userData);
    *out = ::FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
    return FALSE;
}

// On some newer shell versions (confirmed on build 10.0.26200 — see issue
// #20) SHELLDLL_DefView is nested directly under Progman instead of under
// a WorkerW, and the WorkerW that actually hosts the desktop background
// content sits alongside it as another direct child of Progman, rather
// than as Progman's top-level sibling. It already exists persistently in
// that case — confirmed by other apps rendering an animated wallpaper
// through it — with no need to send kSpawnWorkerWMessage at all.
HWND findWorkerWNestedUnderProgman() {
    HWND progman = ::FindWindowW(L"Progman", nullptr);
    if (progman == nullptr) {
        return nullptr;
    }
    HWND defView = ::FindWindowExW(progman, nullptr, L"SHELLDLL_DefView", nullptr);
    if (defView == nullptr) {
        return nullptr;
    }
    return ::FindWindowExW(progman, defView, L"WorkerW", nullptr);
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
    if (workerW != nullptr) {
        return workerW;
    }
    return findWorkerWNestedUnderProgman();
}

bool Win32WorkerWApi::setParent(WindowHandle child, WindowHandle parent) const {
    return ::SetParent(static_cast<HWND>(child), static_cast<HWND>(parent)) != nullptr;
}

void Win32WorkerWApi::sleepMilliseconds(int milliseconds) const {
    ::Sleep(static_cast<DWORD>(milliseconds));
}

}  // namespace umbra
