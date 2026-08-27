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

#include "app/updater.h"

#include <windows.h>
#include <winhttp.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "app/version_compare.h"

namespace umbra {

namespace {

using json = nlohmann::json;

constexpr wchar_t kUserAgent[] = L"Umbra-Updater/1.0";

// Closes a WinHTTP handle on scope exit — every WinHttpOpen/Connect/
// OpenRequest call below returns one of these, and there are several
// early-return failure paths where forgetting to close one would leak it
// for the lifetime of the process.
class ScopedHInternet {
   public:
    explicit ScopedHInternet(HINTERNET handle = nullptr) : handle_(handle) {}
    ~ScopedHInternet() {
        if (handle_ != nullptr) {
            WinHttpCloseHandle(handle_);
        }
    }
    ScopedHInternet(const ScopedHInternet&) = delete;
    ScopedHInternet& operator=(const ScopedHInternet&) = delete;

    operator HINTERNET() const { return handle_; }
    HINTERNET* addressOf() { return &handle_; }

   private:
    HINTERNET handle_;
};

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return L"";
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(),
                        length);
    return wide;
}

// Reads an entire HTTP response body off an already-sent, already-
// "received response"-completed hRequest handle. Returns std::nullopt on
// any read error partway through — a truncated body is as unusable as no
// body at all for both the JSON-parsing and the binary-download callers.
std::optional<std::vector<char>> readResponseBody(HINTERNET hRequest) {
    std::vector<char> body;
    DWORD available = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &available)) {
            return std::nullopt;
        }
        if (available == 0) break;

        std::vector<char> chunk(available);
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, chunk.data(), available, &read)) {
            return std::nullopt;
        }
        body.insert(body.end(), chunk.begin(), chunk.begin() + read);
    } while (available > 0);
    return body;
}

// One GET request against url (any https URL — api.github.com's JSON
// endpoints and a release asset's redirect-following download both go
// through this same path). WinHTTP follows 3xx redirects for GET
// automatically, which is exactly what a GitHub Releases asset URL
// needs (it 302s to the actual CDN object). Returns std::nullopt on any
// failure to connect/send/receive.
std::optional<std::vector<char>> httpGet(const std::wstring& url,
                                         const std::vector<std::wstring>& extraHeaders) {
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    wchar_t hostBuffer[256]{};
    wchar_t pathBuffer[2048]{};
    components.lpszHostName = hostBuffer;
    components.dwHostNameLength = static_cast<DWORD>(std::size(hostBuffer));
    components.lpszUrlPath = pathBuffer;
    components.dwUrlPathLength = static_cast<DWORD>(std::size(pathBuffer));
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &components)) {
        return std::nullopt;
    }

    ScopedHInternet hSession(WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!static_cast<HINTERNET>(hSession)) return std::nullopt;

    ScopedHInternet hConnect(
        WinHttpConnect(hSession, components.lpszHostName, components.nPort, 0));
    if (!static_cast<HINTERNET>(hConnect)) return std::nullopt;

    const bool isHttps = components.nScheme == INTERNET_SCHEME_HTTPS;
    ScopedHInternet hRequest(WinHttpOpenRequest(hConnect, L"GET", components.lpszUrlPath, nullptr,
                                                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                isHttps ? WINHTTP_FLAG_SECURE : 0));
    if (!static_cast<HINTERNET>(hRequest)) return std::nullopt;

    for (const std::wstring& header : extraHeaders) {
        WinHttpAddRequestHeaders(hRequest, header.c_str(), static_cast<DWORD>(header.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD);
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
                            0, 0)) {
        return std::nullopt;
    }
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        return std::nullopt;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr,
                        &statusCode, &statusCodeSize, nullptr);
    if (statusCode < 200 || statusCode >= 300) {
        return std::nullopt;
    }

    return readResponseBody(hRequest);
}

// The installer asset's name always follows release.yml's own
// UmbraSetup-X.Y.Z.exe convention — matched by prefix/suffix rather than
// an exact version-embedded name so this doesn't need its own copy of
// version-string formatting.
std::string findInstallerAssetUrl(const json& releaseJson) {
    if (!releaseJson.contains("assets")) return {};
    for (const auto& asset : releaseJson.at("assets")) {
        const std::string name = asset.value("name", "");
        if (name.rfind("UmbraSetup-", 0) == 0 && name.size() > 4 &&
            name.substr(name.size() - 4) == ".exe") {
            return asset.value("browser_download_url", "");
        }
    }
    return {};
}

}  // namespace

Updater::Updater(std::string owner, std::string repo)
    : owner_(std::move(owner)), repo_(std::move(repo)) {}

UpdateCheckResult Updater::checkForUpdate(const std::string& currentVersion) {
    UpdateCheckResult result;

    const std::optional<Version> current = parseVersion(currentVersion);
    if (!current) {
        result.error = "current version \"" + currentVersion + "\" isn't a parseable X.Y.Z";
        return result;
    }

    const std::wstring url = L"https://api.github.com/repos/" + utf8ToWide(owner_) + L"/" +
                             utf8ToWide(repo_) + L"/releases/latest";
    const std::optional<std::vector<char>> body =
        httpGet(url, {L"Accept: application/vnd.github+json"});
    if (!body) {
        result.error = "couldn't reach GitHub Releases";
        return result;
    }

    json releaseJson;
    try {
        releaseJson = json::parse(body->begin(), body->end());
    } catch (const std::exception&) {
        result.error = "GitHub's response wasn't valid JSON";
        return result;
    }

    const std::string tagName = releaseJson.value("tag_name", "");
    const std::optional<Version> latest = parseVersion(tagName);
    if (!latest) {
        // The latest release isn't tagged like a version this app
        // understands (shouldn't happen against release.yml's own tag
        // filter, but this is talking to a network response, not this
        // repo's own CI) — report success with nothing to offer rather
        // than a confusing error.
        result.checkSucceeded = true;
        return result;
    }

    result.checkSucceeded = true;
    result.latestVersion = tagName.empty() || tagName[0] != 'v' ? tagName : tagName.substr(1);
    result.updateAvailable = isNewerVersion(*current, *latest);
    if (result.updateAvailable) {
        result.downloadUrl = findInstallerAssetUrl(releaseJson);
        if (result.downloadUrl.empty()) {
            // A release exists but has no UmbraSetup-*.exe asset attached
            // yet (e.g. release.yml is still mid-run) — don't offer an
            // update with nowhere to download it from.
            result.updateAvailable = false;
        }
    }
    return result;
}

bool Updater::applyUpdate(const std::string& downloadUrl) {
    const std::optional<std::vector<char>> installerBytes = httpGet(utf8ToWide(downloadUrl), {});
    if (!installerBytes || installerBytes->empty()) {
        return false;
    }

    wchar_t tempDir[MAX_PATH]{};
    if (GetTempPathW(static_cast<DWORD>(std::size(tempDir)), tempDir) == 0) {
        return false;
    }
    const std::wstring installerPath = std::wstring(tempDir) + L"UmbraSetup-update.exe";

    {
        std::ofstream file(installerPath, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        file.write(installerBytes->data(), static_cast<std::streamsize>(installerBytes->size()));
        if (!file) return false;
    }

    // /VERYSILENT /SUPPRESSMSGBOXES: no installer UI at all.
    // /NORESTART: this is an Inno Setup flag about *machine* reboots
    // (irrelevant here) — installer/umbra.iss's own CloseApplications/
    // RestartApplications directives are what close and relaunch this
    // running umbra.exe, via Restart Manager and AppMutex, independent of
    // this flag.
    std::wstring commandLine =
        L"\"" + installerPath + L"\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART";

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    const BOOL launched = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0,
                                         nullptr, nullptr, &startupInfo, &processInfo);
    if (!launched) {
        return false;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

}  // namespace umbra
