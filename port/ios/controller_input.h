// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstdint>

namespace openrac::ios {
// Native hosts send edges; the shared launcher owns focus, repeat and activation.
struct ControllerEdges {
    uint32_t down = 0;
    uint32_t up = 0;
};

class ControllerInput {
public:
    ControllerEdges update(uint32_t buttons, bool active) {
        if (!active || !ready_) {
            ControllerEdges result{0, held_};
            blocked_ = buttons;
            held_ = 0;
            ready_ = active;
            return result;
        }
        // A button held through a system dialog or backgrounding must be released first.
        blocked_ &= buttons;
        uint32_t now = buttons & ~blocked_;
        ControllerEdges result{now & ~held_, held_ & ~now};
        held_ = now;
        return result;
    }

private:
    uint32_t held_ = 0;
    uint32_t blocked_ = 0;
    bool ready_ = false;
};
}  // namespace openrac::ios
