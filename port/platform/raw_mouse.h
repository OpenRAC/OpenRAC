// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mouse's own motion where SDL's captured motion is not: on macOS SDL reports the cursor's
// motion, which the system's pointer acceleration has already bent, so a fast flick turns further
// than a slow drag of the same distance. There this reads the mouse through Apple's Game
// Controller framework (GCMouse, macOS 11 and later), whose motion is the device's counts. On every
// other system SDL's captured motion is already the device's (Windows' raw input, X11's raw
// XInput2 values, Wayland's unaccelerated motion), and this does nothing.

#pragma once

namespace openrac::platform::raw_mouse {

// Starts reading (once; later calls do nothing). False where there is no such reader: SDL's
// motion is used.
bool start();

// Whether a mouse is being read now: then its motion replaces SDL's while the cursor is captured.
bool active();

// The motion since the last call, in the device's counts (x right, y down), and starts afresh.
void take(float& dx, float& dy);

}  // namespace openrac::platform::raw_mouse
