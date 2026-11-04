// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A free fly camera in the game's axes: Z up, yaw about Z (0 looks along +X),
// pitch up from the horizon.

#pragma once

#include "renderer/math.h"

namespace openrac::scene {

using renderer::Mat4;
using renderer::Vec3;

// Display-frame movement and look input; up/down are world vertical, forward/back follow the view.
struct FlyControls {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool fast = false;
    bool slow = false;  // Precision movement takes precedence over the fast modifier.
    bool normalise_movement =
        false;  // Diagonals no faster than one axis; forward follows pitch, up stays vertical.
    // A gamepad's analog movement, -1..1, added to the keys above: forward, right and up positive.
    float ahead = 0.0f;
    float sideways = 0.0f;
    float vertical = 0.0f;
    float look_x = 0.0f;  // mouse movement, pixels (the right stick too, as pixels)
    float look_y = 0.0f;
};

// A gamepad stick's two axes (-1..1) past a round dead zone: 0 inside it, then rising from 0 at its
// edge to 1 at the rim, the direction kept.
void stick_past_dead_zone(float& x, float& y, float dead_zone);

// Z-up fly pose and lens, owned and advanced by the thread drawing the scene.
struct FlyCamera {
    Vec3 position{};
    float yaw = 0.0f;       // radians
    float pitch = 0.0f;     // radians
    float speed = 20.0f;    // game units a second
    float fov_y = 1.0472f;  // 60 degrees
    float near_z = 0.1f;
    float far_z = 4000.0f;

    // Returns the unit direction defined by yaw and pitch.
    Vec3 forward() const;
    // Returns the world-to-view transform in game axes.
    Mat4 view() const;
    // Returns a reversed-depth lens for the drawable aspect ratio.
    Mat4 projection(float aspect) const;  // reversed depth

    // Applies display-frame input over `seconds`, independently of game simulation ticks.
    void update(const FlyControls& controls, float seconds);

    // Stand back from a box so all of it is in view, looking down at it.
    void frame(const Vec3& low, const Vec3& high);
};

}  // namespace openrac::scene
