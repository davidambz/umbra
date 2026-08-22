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

#include "power/power_state.h"

namespace umbra {

// Abstracts the actual OS power query (GetSystemPowerStatus on Windows) so
// PowerWatcher's poll-and-decide logic is unit-testable without a live
// power subsystem — see Win32PowerApi for the real implementation.
class IPowerApi {
   public:
    virtual ~IPowerApi() = default;
    virtual PowerState queryState() const = 0;
};

// Polls IPowerApi (call refresh() periodically — e.g. from app/'s tick,
// per ARCHITECTURE.md's Power/Focus Watcher) and reports whether the
// throttle action a caller should apply has changed since the last call,
// per the user's PowerThrottleConfig.
class PowerWatcher {
   public:
    // api must outlive this PowerWatcher — it's stored by reference and
    // called from refresh(), not just at construction.
    PowerWatcher(const IPowerApi& api, PowerThrottleConfig config);

    // Re-queries power state and re-decides the throttle action. Returns
    // true if the action differs from currentAction()'s prior value
    // (which starts at Normal) — callers can use this to only react on
    // actual transitions instead of every tick.
    bool refresh();

    ThrottleAction currentAction() const { return currentAction_; }

    // The fps cap a Reduced action implies, per the config passed at
    // construction — exposed so a caller combining this with its own fps
    // policy (see app/render_policy.h) reads it from one place instead of
    // duplicating the configured value in a second constant.
    int reducedFpsCap() const { return config_.reducedFpsCap; }

    // Lets a caller apply a live settings change (e.g. the user flipping
    // Settings::pauseOnBattery in settings-ui/) without tearing down and
    // reconstructing the whole PowerWatcher.
    void setConfig(PowerThrottleConfig config) { config_ = config; }

   private:
    const IPowerApi& api_;
    PowerThrottleConfig config_;
    ThrottleAction currentAction_ = ThrottleAction::Normal;
};

}  // namespace umbra
