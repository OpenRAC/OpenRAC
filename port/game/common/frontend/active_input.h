// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which of the player's devices is in use (platform/active_device.h), for code built into the
// port that is not the window's, such as a built-in mod.

#pragma once

namespace openrac::frontend {

// True while the player is on the keyboard and mouse, false on a gamepad or with no window.
bool keyboard_mouse_in_use();

}  // namespace openrac::frontend
