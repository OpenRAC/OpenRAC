// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The late camera latch (settings responsive_controls, docs/port/SETTINGS.md section 2): in the
// frames drawn between ticks, the picture's camera turned by the right stick as it is just before the
// frame is drawn, not as it was at the last tick. Only the picture: the game's tick and its camera
// are never touched. Internal to the frontend.

#pragma once

#include <chrono>
#include <cstdint>

#include "game/common/frontend/state.h"

namespace openrac::frontend {

// How a frame's camera is turned: by `yaw` radians about the world's up axis (game axes, z),
// through `pivot`. 0: the camera as interpolated.
struct CameraTurn {
    float yaw = 0.0f;
    renderer::Vec3 pivot{};
};

// The turn for a frame of the kept tick `scene` drawn `alpha` of the way from the last tick's
// camera and shown at `display`. On the window thread while the game waits between ticks (it reads
// the game's memory and the pad, and keeps what the next frame continues from).
CameraTurn latch_camera(
    const Kept& scene, float alpha, std::chrono::steady_clock::time_point display
);

// The right stick's x a scripted run (OPENRAC_PRESS) gave the game's last read: what the latch
// turns by while the pad itself is centred.
void latch_scripted_stick(std::uint8_t right_x);

// `state`'s camera turned by `turn`.
host::GameState turned_camera(const host::GameState& state, const CameraTurn& turn);

}  // namespace openrac::frontend
