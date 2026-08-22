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

#include "desktop/workerw_host.h"

namespace umbra {

namespace {
// Empirically, a still-settling shell hierarchy right after
// sendSpawnWorkerWMessage() (or an explorer.exe restart) resolves within a
// few hundred milliseconds — kMaxSpawnLookupAttempts retries at
// kSpawnLookupRetryDelayMs apart cover that without adding a
// user-noticeable startup delay if it succeeds on the first try, which is
// the common case.
constexpr int kMaxSpawnLookupAttempts = 5;
constexpr int kSpawnLookupRetryDelayMs = 200;
}  // namespace

WorkerWHost::WorkerWHost(const IWorkerWApi& api) : api_(api) {}

bool WorkerWHost::ensureWorkerWSpawned() {
    if (workerW_ != kNullWindow) {
        return true;
    }

    const WindowHandle progman = api_.findWindowByClass("Progman");
    if (progman == kNullWindow) {
        return false;
    }

    api_.sendSpawnWorkerWMessage(progman);

    for (int attempt = 0; attempt < kMaxSpawnLookupAttempts; ++attempt) {
        workerW_ = api_.findBackgroundWorkerW();
        if (workerW_ != kNullWindow) {
            return true;
        }
        if (attempt + 1 < kMaxSpawnLookupAttempts) {
            api_.sleepMilliseconds(kSpawnLookupRetryDelayMs);
        }
    }
    return false;
}

bool WorkerWHost::attach(WindowHandle renderWindow) {
    if (workerW_ == kNullWindow) {
        return false;
    }
    if (api_.setParent(renderWindow, workerW_)) {
        return true;
    }

    // setParent() can fail because the cached WorkerW died — e.g. an
    // explorer.exe restart destroyed it — since ensureWorkerWSpawned()
    // only runs the spawn sequence once and never re-validates its
    // cached handle afterward (see issue #27). Invalidate the cache and
    // retry the full spawn sequence once before giving up, instead of
    // silently failing every attach() call until the process restarts.
    workerW_ = kNullWindow;
    return ensureWorkerWSpawned() && api_.setParent(renderWindow, workerW_);
}

}  // namespace umbra
