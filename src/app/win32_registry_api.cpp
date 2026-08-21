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

#include "app/win32_registry_api.h"

#include <windows.h>

namespace umbra {

namespace {

constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValueName[] = L"Umbra";

}  // namespace

bool Win32RegistryApi::getRunValue(std::wstring* outCommand) const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    DWORD sizeBytes = 0;
    LONG status = RegQueryValueExW(key, kValueName, nullptr, &type, nullptr, &sizeBytes);
    if (status != ERROR_SUCCESS || type != REG_SZ || sizeBytes == 0) {
        RegCloseKey(key);
        return false;
    }

    std::wstring value(sizeBytes / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(key, kValueName, nullptr, &type,
                              reinterpret_cast<BYTE*>(value.data()), &sizeBytes);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    // Trim the trailing NUL RegQueryValueExW includes in sizeBytes.
    const size_t nulPos = value.find(L'\0');
    if (nulPos != std::wstring::npos) {
        value.resize(nulPos);
    }
    *outCommand = std::move(value);
    return true;
}

bool Win32RegistryApi::setRunValue(const std::wstring& command) const {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD sizeBytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
    const LONG status = RegSetValueExW(key, kValueName, 0, REG_SZ,
                                       reinterpret_cast<const BYTE*>(command.c_str()), sizeBytes);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

bool Win32RegistryApi::deleteRunValue() const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    const LONG status = RegDeleteValueW(key, kValueName);
    RegCloseKey(key);
    // Deleting a value that's already absent isn't a failure for our
    // purposes — disable() should be idempotent.
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

}  // namespace umbra
