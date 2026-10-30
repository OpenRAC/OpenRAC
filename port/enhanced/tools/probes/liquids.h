// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

#include "assets/bytes.h"
#include "enhanced/lighting/bake.h"

namespace openrac::lighting {
// The reviewed Gaspar liquid grid's record in each disc's level 9 overlay: the one
// NTSC FUN_L09_002ef750 and PAL func_L09_002F0AA0 draw (gaspar_textured_grid).
inline constexpr std::uint32_t kGasparLiquidNtsc = 0x1fc740;
inline constexpr std::uint32_t kGasparLiquidPal = 0x1fca00;

// Derive static transport from the reviewed Gaspar liquid grid at `record` and its
// animated FX textures. Inputs are ordinary decoded level sections, no game run.
std::vector<BakeTriangle> gaspar_liquid_triangles(
    assets::ByteView overlay,
    assets::ByteView index,
    assets::ByteView core,
    std::uint32_t record = kGasparLiquidNtsc
);
}  // namespace openrac::lighting
