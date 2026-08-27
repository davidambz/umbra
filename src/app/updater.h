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

#include "ui/ui_bridge.h"

namespace umbra {

// Windows-only (WinHTTP): checks GitHub Releases for a newer Umbra build
// and can silently download + launch the new installer. Per #78 — the
// in-app half of #37's release pipeline. Application's real
// IUiBridgeHost::checkForUpdate()/applyUpdate() delegate straight here;
// kept as its own class rather than inlined into application.cpp so the
// WinHTTP plumbing has one obvious, self-contained home.
class Updater {
   public:
    // owner/repo identify the GitHub repository whose
    // /releases/latest is checked (davidambz/umbra in production;
    // overridable for the sake of not hardcoding it twice).
    Updater(std::string owner, std::string repo);

    // Compares GitHub's latest release tag against currentVersion
    // (version_compare.h's X.Y.Z parsing — anything else already
    // published there, like a non-version tag, is treated as "no
    // update" rather than an error).
    UpdateCheckResult checkForUpdate(const std::string& currentVersion);

    // Downloads the installer at downloadUrl to a temp file and launches
    // it detached with /VERYSILENT /SUPPRESSMSGBOXES /NORESTART.
    // installer/umbra.iss's CloseApplications/RestartApplications
    // directives (driven by AppMutex) handle closing this running
    // process and relaunching it once the silent install completes.
    // Returns whether the download and process launch both succeeded —
    // not whether the install itself succeeds, which this process may
    // not survive to observe.
    bool applyUpdate(const std::string& downloadUrl);

   private:
    std::string owner_;
    std::string repo_;
};

}  // namespace umbra
