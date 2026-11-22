// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which of the player's devices is in use (platform/active_device.h), for code built into the
// port that is not the window's, such as a built-in mod.

#pragma once

#include <string>

namespace openrac::frontend {

// True while the player is on the keyboard and mouse, false on a gamepad or with no window.
bool keyboard_mouse_in_use();

// What a built-in keyboard and mouse mod paints over the game's button icons now, in
// key_prompts.h's cell order (Cross, Square, Triangle, Circle, L1, L2, R1, R2, the two sticks): a
// key's name or a label, empty for the game's own icon. For the mods' button labels (interface
// 1.31, button_labels.h). `count` at most 10.
void set_prompt_keys(const std::string* names, int count);

}  // namespace openrac::frontend
