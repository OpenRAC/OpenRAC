// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// MobyProc's distance test and fade, which Ratchet & Clank and Going Commando share: the same
// instructions in both games' moby renderers, fed from each game's own view block, near plane and
// draw-distance cap (GameState::moby_view, which each game's profile reads).

#pragma once

#include <cstdint>
#include <optional>

#include "game/common/profile.h"

namespace openrac::host {

/**
 * MobyProc's distance test and fade for the moby `moby` (its 0x100 bytes: the sphere at +0x00, the
 * draw distance at +0x32), seen from `state`'s camera: none when it leaves the moby undrawn (past
 * its draw distance or the cap, wholly nearer than the near plane, or, as the port tests it, wholly
 * outside the view's sides by far), else the fade of its vertex alpha in the GS's units, 0x80
 * nearer than 16 units inside its draw distance, falling to 0 at it. Bit for bit the game's when
 * `state` has the view block MobyProc reads (moby_view); the same tests in the host's floats
 * without one.
 */
std::optional<std::uint8_t> moby_proc_distance_fade(
    const std::uint8_t* moby, const GameState& state
);

/**
 * The same test for the freecam, which flies away from the game's camera: whether the moby is
 * within its draw distance (and the cap, scaled as Draw distance scales it) of `state`'s camera
 * in any direction, the near plane and the sides left to the window's own view of the freecam.
 * None, as above, when it is not, or when its draw distance is 0 or its sphere is not a number.
 */
std::optional<std::uint8_t> moby_free_camera_reach(
    const std::uint8_t* moby, const GameState& state
);

}  // namespace openrac::host
