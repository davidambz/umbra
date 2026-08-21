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

#include "app/render_policy.h"

#include <algorithm>

namespace umbra {

RenderPolicy computeRenderPolicy(bool fullscreenActive, bool pauseOnFullscreen,
                                 ThrottleAction powerAction, int profileFpsCap, int reducedFpsCap) {
    RenderPolicy policy;
    policy.fpsCap = profileFpsCap;

    if (fullscreenActive && pauseOnFullscreen) {
        policy.paused = true;
        return policy;
    }

    if (powerAction == ThrottleAction::Paused) {
        policy.paused = true;
        return policy;
    }

    if (powerAction == ThrottleAction::Reduced) {
        policy.fpsCap = std::min(profileFpsCap, reducedFpsCap);
    }

    return policy;
}

}  // namespace umbra
