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

#include "desktop/workerw_host.h"

namespace umbra {

// Real IWorkerWApi backed by the actual Win32 calls (FindWindowA,
// SendMessageTimeoutW, EnumWindows, SetParent). Windows-only — verified
// manually against a live desktop session (see TESTING.md); the sequencing
// logic that uses this interface (WorkerWHost) is what's unit-tested.
class Win32WorkerWApi : public IWorkerWApi {
   public:
    WindowHandle findWindowByClass(const char* className) const override;
    void sendSpawnWorkerWMessage(WindowHandle progman) const override;
    WindowHandle findBackgroundWorkerW() const override;
    bool setParent(WindowHandle child, WindowHandle parent) const override;
};

}  // namespace umbra
