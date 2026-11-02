// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// ShrubProc's choice of what to draw of each shrub (NTSC-U boot 0x228BE8; each level program has
// the same code at other addresses), replayed from the game's memory: which instances it culls,
// the alpha it draws each mesh at, and the billboards it draws in place of the far ones, with the
// sprites' corners. Nothing here draws.

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "game/common/profile.h"
#include "game/rac1/game_state.h"

namespace openrac::game::rac1 {

/**
 * The class pointer table ShrubProc walks (the decomp's shrub_class_table) in the program
 * loaded: `level` 0..18, or the boot program's for any other.
 */
std::uint32_t shrub_class_table(bool ntsc, int level);

/**
 * ShrubProc for the frame: one entry per shrub instance, in the game's instance order (the
 * loader's, the export's), seen from the camera in `a`'s blocks. Empty when the shrub globals or
 * `class_table` are not usable.
 */
std::vector<host::ShrubDraw> shrub_proc(
    std::span<const std::uint8_t> ram, const GameAddresses& a, std::uint32_t class_table
);

}  // namespace openrac::game::rac1
