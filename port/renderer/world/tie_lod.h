// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tie_lod.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Tie level of detail as the game decides it: TieProc (NTSC-U 0x235be8 in
// ReRAC's reading; PAL func_00236F00, RENDERER.md section 2) replayed on the
// CPU each frame for every tie instance, and the per-instance VU1 quadword it
// uploads: the fat vertices' morph factor k and the colour weights of the tie
// programs (13507, 224979).
//
// Per instance, after the occlusion test:
//   pass 1 (cull) on the camera-space bounding sphere: not drawn when the
//     draw distance is 0, when the centre is beyond the draw distance (capped
//     at WorldConstants::tie_draw_cap), when the sphere is wholly in front of
//     the near plane, or wholly outside a side plane; else depth = max(0, z);
//   pass 2 (LOD) against the class's near, mid and far distances:
//     near - depth >= 0: LOD 0, k = 0;  far - depth < 0: LOD 2, k = 0;
//     mid - depth >= 0: LOD 0, k = (depth - near) / (mid - near);
//     otherwise: LOD 1, k = (depth - mid) / (far - mid);
//   the colour weights qw4.w = 256k and qw4.z = 256 - 256k, computed on the
//   VU (its adder keeps w + z within [256, 256 + 2^-16), so a lane never wraps);
//   the GS fog F from the centre depth (fog.h tie_fog_value).
// Not modelled: the guard-band test that sends an instance to the clipping
// program 224979 (same LOD and weights; the GPU clips instead).

#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "renderer/math.h"
#include "renderer/world/fog.h"

namespace openrac::renderer::world {

inline constexpr std::uint32_t kTieCulled = 3;

struct TieLodPick {
    std::uint32_t lod = 0;
    float k = 0.0f;  // fat vertices are drawn at position + k * delta
    float w = 0.0f;  // 256 k: the weight of the averaged second and third colours
    float z = 256.0f;  // 256 - 256 k: the weight of the first colour

    bool operator==(const TieLodPick&) const = default;
};

// TieProc pass 2 for the clamped centre depth (game units) and the class's
// near, mid and far distances, in the console's float arithmetic.
TieLodPick tie_lod(float depth, const std::array<float, 3>& distances);

// TieProc pass 1: `p` the camera-space sphere centre (x right, y down, z
// forward; game units), `radius`, `draw_distance` (capped). Returns the depth
// pass 2 uses, or nothing when culled.
std::optional<float> tie_cull(const Vec3& p, float radius, float draw_distance, float tan_x, float tan_y);

// The per-instance record the tie shader reads: LOD (kTieCulled: not
// drawn) | F << 8, then k, w, z as float bits.
std::array<std::uint32_t, 4> tie_record(
    const Vec3& camera_centre,
    float radius,
    float draw_distance,
    const std::array<float, 3>& distances,
    const WorldFog& fog,
    float tan_x,
    float tan_y
);

}  // namespace openrac::renderer::world
