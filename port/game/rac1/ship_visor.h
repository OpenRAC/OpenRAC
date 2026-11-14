// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Thin glass fitted to the animated canopy frames of verified ships.
#pragma once

namespace openrac::scene {
struct LevelData;
}

namespace openrac::game::rac1 {
// Adds the pane when the verified NTSC-U frame is present.
void add_ship_visor(scene::LevelData& level);
}  // namespace openrac::game::rac1
