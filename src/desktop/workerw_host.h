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

namespace umbra {

// An opaque OS window handle. Aliased to HWND by the real Win32
// implementation (Win32WorkerWApi) — kept as void* here so this header (and
// WorkerWHost's orchestration logic) has zero Win32/OS dependency and can be
// unit-tested with a mock on any platform.
using WindowHandle = void*;
inline constexpr WindowHandle kNullWindow = nullptr;

// Abstracts the raw OS calls behind the Progman/WorkerW attach hack (see
// ARCHITECTURE.md for the full sequence). The real Win32WorkerWApi
// implementation is Windows-only and verified manually (see TESTING.md);
// this interface exists so WorkerWHost's orchestration — the order these
// calls happen in — is unit-testable without a live desktop session.
class IWorkerWApi {
   public:
    virtual ~IWorkerWApi() = default;

    // Finds a top-level window by class name, or kNullWindow if none exists.
    virtual WindowHandle findWindowByClass(const char* className) const = 0;

    // Sends the undocumented message that makes Progman spawn a WorkerW
    // window behind the desktop icons.
    virtual void sendSpawnWorkerWMessage(WindowHandle progman) const = 0;

    // Finds the WorkerW window created for the desktop background (as
    // opposed to the decoy one Windows also creates as a sibling of
    // Progman), or kNullWindow if it can't be located.
    virtual WindowHandle findBackgroundWorkerW() const = 0;

    // Reparents child into parent. Returns false on failure.
    virtual bool setParent(WindowHandle child, WindowHandle parent) const = 0;

    // Blocks the calling thread for roughly milliseconds. Used by
    // ensureWorkerWSpawned()'s retry loop below — kept on the interface
    // (rather than calling ::Sleep directly) so tests can run the retry
    // loop instantly instead of actually waiting.
    virtual void sleepMilliseconds(int milliseconds) const = 0;
};

// Orchestrates the Progman -> WorkerW attach sequence and reparents one
// render window per monitor into the resulting WorkerW.
class WorkerWHost {
   public:
    // api must outlive this WorkerWHost — it's stored by reference and
    // called from ensureWorkerWSpawned()/attach(), not just at construction.
    explicit WorkerWHost(const IWorkerWApi& api);

    // Runs the spawn sequence (send message to Progman, locate the
    // resulting WorkerW). Idempotent: a no-op if already spawned. Must
    // succeed before attach() can. Returns false if Progman can't be
    // found, or the resulting WorkerW still can't be located after
    // kMaxSpawnLookupAttempts retries (see workerw_host.cpp — Explorer's
    // window hierarchy can still be settling briefly after the spawn
    // message, especially right after an explorer.exe restart, so a
    // single immediate lookup can miss it).
    bool ensureWorkerWSpawned();

    // Reparents renderWindow into the current WorkerW so it renders behind
    // the desktop icons. Call once per monitor's render window, after
    // ensureWorkerWSpawned() has succeeded. If the cached WorkerW has died
    // since (e.g. an explorer.exe restart) — detected via setParent()
    // itself failing — this transparently re-runs the spawn sequence once
    // before giving up, so a single missed attach doesn't require a
    // process restart to recover from.
    bool attach(WindowHandle renderWindow);

    WindowHandle workerW() const { return workerW_; }

   private:
    const IWorkerWApi& api_;
    WindowHandle workerW_ = kNullWindow;
};

}  // namespace umbra
