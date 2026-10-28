// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exported probe transport and conversion statistics; allocation and identity have separate headers.

#pragma once
#include <array>
#include <stdexcept>

#include "enhanced/lighting/bake.h"
#include "scene/level.h"

namespace openrac::lighting {

// A level with nothing for light to bounce off (the flight between planets, level 98: only its
// sky): no bake to make, not a failure. The tool exits with kNothingToBake for it.
struct NothingToBake : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline constexpr int kNothingToBake = 3;

struct BakeSceneExport {
    BakeScene scene;
    Float3 bounds_min{}, bounds_max{};
    std::array<std::size_t, 4> instances{};  // terrain, ties, shrubs, selected static mobys
    std::size_t automatic_liquid_triangles = 0;
    std::size_t surface_instances = 0;  // animated surfaces supplied for static transport
    std::size_t skipped_blended = 0, skipped_degenerate = 0;
    std::size_t skipped_mobys = 0, skipped_glow = 0;
    std::size_t emissive_triangles = 0;
    std::uint64_t scene_hash = 0;
};

// Static exported render meshes plus explicitly selected unskinned mobys.
// Moving mobys, stand-in boxes and sky meshes are excluded from
// occlusion/transport geometry; the exported sky panorama supplies environment.
// Activation passes analyze_liquids=false: it fingerprints the same raw inputs
// without performing bake-only liquid analysis. Existing proxy surfaces still load.
// Format 3 has no terrain light selector, so the initial experiment explicitly
// selects one light set instead of summing every set in the level.
BakeSceneExport export_probe_scene(
    const scene::LevelData& level,
    unsigned light_set,
    Float3 environment,
    bool analyze_liquids = true
);
}  // namespace openrac::lighting
