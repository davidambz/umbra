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

#include "engines/win32_text.h"

#include <windows.h>
#include <wrl/client.h>

#include <cstdio>

#include "resource.h"

namespace umbra {

namespace {

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), utf8.data(),
                        length, nullptr, nullptr);
    return utf8;
}

bool isUnreservedUrlByte(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_' || c == '.' || c == '~' || c == '/' || c == ':';
}

}  // namespace

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), wide.data(),
                        length);
    return wide;
}

std::wstring toFileUrl(const std::filesystem::path& path) {
    std::wstring nativePath = path.wstring();
    for (wchar_t& c : nativePath) {
        if (c == L'\\') {
            c = L'/';
        }
    }

    std::string encoded;
    const std::string utf8Path = wideToUtf8(nativePath);
    encoded.reserve(utf8Path.size());
    for (unsigned char c : utf8Path) {
        if (isUnreservedUrlByte(c)) {
            encoded += static_cast<char>(c);
        } else {
            char escaped[4];
            std::snprintf(escaped, sizeof(escaped), "%%%02X", c);
            encoded += escaped;
        }
    }

    return L"file:///" + utf8ToWide(encoded);
}

void navigateToLocalFolder(ICoreWebView2* webView, const std::filesystem::path& folder,
                           const wchar_t* virtualHostName) {
    Microsoft::WRL::ComPtr<ICoreWebView2_3> webView3;
    if (SUCCEEDED(webView->QueryInterface(IID_PPV_ARGS(&webView3))) &&
        SUCCEEDED(webView3->SetVirtualHostNameToFolderMapping(
            virtualHostName, folder.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY))) {
        const std::wstring url = L"https://" + std::wstring(virtualHostName) + L"/index.html";
        webView->Navigate(url.c_str());
        return;
    }
    webView->Navigate(toFileUrl(folder / "index.html").c_str());
}

HICON loadAppIcon(HINSTANCE instance) {
    HICON icon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (icon == nullptr) {
        icon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    return icon;
}

}  // namespace umbra
