// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstdint>

#include "assets/bytes.h"
#include "common/json.h"
#include "enhanced/lighting/bake.h"

namespace openrac::lighting {
// Going Commando's liquid surfaces, as the exporter writes them (editor/enhanced/liquids.py,
// bake-inputs/liquids.json, manifest bake_inputs "liquids": "rac2-surfaces-v1"): each surface the
// level's draw callback func_00303070 draws, an 8 x 8 vertex grid over four corners at one height.
// Only the surfaces the export marks emissive (a reviewed effect texture) become transport: two
// triangles a cell, no diffuse, emitting their texture's averaged colour at the rest tint.
inline constexpr const char* kRac2SurfacesSource = "rac2-surfaces-v1";
std::vector<BakeTriangle> rac2_liquid_triangles(const json::Value& surfaces);

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
