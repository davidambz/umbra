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

#include <windows.h>

#include <shellapi.h>
#include <shlobj.h>

#include <cwchar>
#include <filesystem>

#include "app/application.h"

namespace {

constexpr wchar_t kSingleInstanceMutexName[] = L"Local\\UmbraSingleInstanceMutex";
constexpr wchar_t kAutostartArg[] = L"--autostart";

// A plain wcsstr() substring check would also match a hypothetical future
// flag like --autostart-minimized, or an install path that happens to
// contain the text "--autostart" — parse into real argv tokens instead and
// compare each one exactly. lpCmdLine (unlike argv from main()) excludes
// the program name, but CommandLineToArgvW doesn't know that; it just
// treats whatever's first as argv[0], which is harmless here since every
// token is still compared for an exact match.
bool commandLineHasAutostartFlag(PWSTR commandLine) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(commandLine != nullptr ? commandLine : L"", &argc);
    if (argv == nullptr) {
        return false;
    }
    bool found = false;
    for (int i = 0; i < argc; ++i) {
        if (std::wcscmp(argv[i], kAutostartArg) == 0) {
            found = true;
            break;
        }
    }
    LocalFree(argv);
    return found;
}

std::filesystem::path resolveLocalAppDataDir() {
    PWSTR localAppData = nullptr;
    std::filesystem::path baseDir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
        baseDir = std::filesystem::path(localAppData);
        CoTaskMemFree(localAppData);
    } else {
        // SHGetKnownFolderPath failing at all is rare, but silently
        // falling through would leave baseDir empty and persist state to
        // whatever the process's current working directory happens to be
        // — an explicit, documented last resort instead.
        baseDir = std::filesystem::current_path();
    }
    return baseDir / L"Umbra";
}

// %LOCALAPPDATA%\Umbra\settings.json.
std::filesystem::path resolveSettingsPath(const std::filesystem::path& umbraDir) {
    std::error_code ec;
    std::filesystem::create_directories(umbraDir, ec);
    return umbraDir / L"settings.json";
}

// %LOCALAPPDATA%\Umbra\Wallpapers — LibraryManager's storage root.
std::filesystem::path resolveStorageRoot(const std::filesystem::path& umbraDir) {
    const std::filesystem::path storageRoot = umbraDir / L"Wallpapers";
    std::error_code ec;
    std::filesystem::create_directories(storageRoot, ec);
    return storageRoot;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*previousInstance*/, PWSTR commandLine,
                    int /*showCommand*/) {
    // Only one Umbra instance should ever run: relaunching the exe (Start
    // Menu, desktop shortcut, running it again) would otherwise spawn a
    // second full orchestrator on top of the first — a second tray icon, a
    // second attempt to spawn/SetParent into the same WorkerW. If another
    // instance already holds this mutex, ask it to open its Settings
    // window (see Application::notifyRunningInstance) and exit immediately
    // instead. The handle stays open for the rest of this function — the
    // whole process lifetime — so a second launch keeps detecting it.
    HANDLE singleInstanceMutex = CreateMutexW(nullptr, TRUE, kSingleInstanceMutexName);
    const bool alreadyRunning =
        singleInstanceMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS;
    if (alreadyRunning) {
        umbra::Application::notifyRunningInstance();
        CloseHandle(singleInstanceMutex);
        return 0;
    }

    // Needed by every COM-based adapter this process uses: WIC (ImageEngine),
    // the native file/folder picker (Application::pickImportSource), and
    // WebView2's own internals.
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Registers this process with Windows Restart Manager so it gets
    // relaunched after being closed to let an update replace its files —
    // installer/umbra.iss's CloseApplications=force + RestartApplications=yes,
    // per #78. Without this call, Restart Manager can still force-close
    // the process (that part works on its own), but has no signal that
    // relaunching it afterward is safe or wanted, so RestartApplications=yes
    // silently does nothing — confirmed by testing: the app closed for the
    // silent install but never came back until this was added.
    //
    // The RESTART_NO_* flags are opt-*outs*, not opt-ins: each one
    // excludes restarting for that specific reason (crash, hang, an
    // OS-patch-triggered reboot, or any other reboot). An earlier version
    // of this call passed all four together, meaning "don't restart for
    // any reason" — which included the exact Restart-Manager-driven
    // shutdown this feature depends on, so the app never came back even
    // after adding this call. Passing 0 (no exclusions) is what actually
    // fixed it: every restart reason stays enabled, this one included. A
    // nullptr command line reuses whatever this process was actually
    // launched with (e.g. --autostart, if that's how it started).
    RegisterApplicationRestart(nullptr, 0);

    // Autostart (see Autostart::enable() in application.cpp) launches with
    // this flag so signing in to Windows doesn't pop the Settings window —
    // every other launch path (Start Menu, desktop shortcut, running the
    // exe directly) should show it immediately.
    const bool isAutostartLaunch = commandLineHasAutostartFlag(commandLine);

    // Application is scoped so it's destroyed (releasing every COM object
    // it owns, transitively) before CoUninitialize() runs.
    int exitCode = 1;
    {
        const std::filesystem::path umbraDir = resolveLocalAppDataDir();
        umbra::Application app(resolveSettingsPath(umbraDir), resolveStorageRoot(umbraDir));
        if (app.initialize(instance)) {
            if (!isAutostartLaunch) {
                app.openSettingsWindow();
            }
            exitCode = app.run();
        }
    }

    CoUninitialize();
    if (singleInstanceMutex != nullptr) {
        CloseHandle(singleInstanceMutex);
    }
    return exitCode;
}
