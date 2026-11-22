// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which of the player's devices is in use (platform/active_device.h), for code built into the
// port that is not the window's, such as a built-in mod.

#pragma once

#include <string>

#include "platform/gyro.h"

namespace openrac::frontend {

// True while the player is on the keyboard and mouse, false on a gamepad or with no window.
bool keyboard_mouse_in_use();

// What a built-in keyboard and mouse mod paints over the game's button icons now, in
// key_prompts.h's cell order (Cross, Square, Triangle, Circle, L1, L2, R1, R2, the two sticks): a
// key's name or a label, empty for the game's own icon. For the mods' button labels (interface
// 1.31, button_labels.h). `count` at most 10.
void set_prompt_keys(const std::string* names, int count);

// The button icons the mods' labels show (button_labels.h): -1 the device in use, 0 the gamepad's,
// 1 the keys. Set by the built-in keyboard and mouse ("Button icons").
void set_prompt_device(int device);

// True: in play the cursor stays captured on a gamepad as well, so the mouse (or a gyro a driver
// turns into a mouse) turns the camera beside the pad. False: only on the keyboard and mouse.
void set_mouse_with_gamepad(bool on);

// Gyro aiming (platform/gyro.h): the options, and the turn the last tick read (yaw > 0 left,
// pitch > 0 up, radians). Returns false while no gyro is read.
void set_gyro_options(const platform::GyroOptions& options);
bool gyro_turn(float& yaw, float& pitch);
// The gyro's turn since, which the next tick will read (for the frames between ticks).
void pending_gyro(float& yaw, float& pitch);

}  // namespace openrac::frontend
