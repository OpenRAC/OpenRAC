// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// raw_mouse.h where SDL's captured motion is already the device's: nothing to read.

#include "platform/raw_mouse.h"

namespace openrac::platform::raw_mouse {

bool start() {
    return false;
}

bool active() {
    return false;
}

void take(float& dx, float& dy) {
    dx = 0.0f;
    dy = 0.0f;
}

}  // namespace openrac::platform::raw_mouse
