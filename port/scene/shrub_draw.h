// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the game's shrub renderer draws of one shrub instance in a frame, as Draw distance:
// Original draws it (GameProfile::shrub_draws, LevelScene::choose_scenery).

#pragma once

#include <cstdint>

#include "renderer/math.h"

namespace openrac::scene {

// The alphas in the GS's units, 0x80 opaque, 0 not drawn.
struct ShrubDraw {
    std::uint8_t mesh = 0;       // the mesh's vertex alpha; below 0x80 the fading pass
    std::uint8_t billboard = 0;  // the billboard's alpha; 0x80 its first pass, below its second
    // The billboard sprite: the unit quad (x 0, y -0.5 to 0.5 across, z 0 to 1 up) to the world.
    renderer::Mat4 billboard_matrix = renderer::identity();
};

}  // namespace openrac::scene
