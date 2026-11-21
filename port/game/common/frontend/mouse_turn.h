// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mouse's turn of the camera in the frames between ticks. A mouse that turns the camera itself
// (rac1's keyboard and mouse) turns it by exactly its motion at the next tick. Until then the
// frames show that turn already (frontend/camera_latch.h), so the picture follows the mouse at the
// display's rate, not the game's 60 or 50 ticks a second. Only the picture: the game's tick is not
// touched.

#pragma once

namespace openrac::frontend {

// What the next tick does with mouse motion it has not read yet.
struct MouseTurn {
    float yaw = 0.0f;    // radians about `up`, right-handed
    float pitch = 0.0f;  // radians, > 0 looks down
    float up[3] = {0.0f, 0.0f, 1.0f};
    float pitch_limit = 0.0f;   // the view's elevation stays within +-this
    bool first_person = false;  // turns about the eye; else about the hero
};

// Set by whatever turns the camera with the mouse: true, with the turn for (dx, dy) pixels of
// motion, while the mouse turns the camera itself now; false otherwise. Null: nothing does.
using MouseTurnFn = bool (*)(float dx, float dy, MouseTurn* out);
void set_mouse_turn(MouseTurnFn fn);

}  // namespace openrac::frontend
