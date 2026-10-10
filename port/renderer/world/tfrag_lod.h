// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tfrag_lod.rs and
// the LOD rule of crates/rc-formats/src/tfrag.rs (Tfrag::draw_mode):
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Terrain level of detail as the game decides it, replayed on the CPU each
// frame for every tfrag: TfragProc's per-tfrag draw mode (NTSC-U 0x233fb0 in
// ReRAC's reading) and SetTfragDists' VU1 morph constants (NTSC-U 0x233068).
// RENDERER.md section 2 has the PAL entry points (TfragProc func_002352C8,
// SetTfragDists func_00234380).
//
// A draw mode is 0 (culled), 2 (the clipping path: LOD 0, no morph) or the
// VU1 MSCAL entry TfragProc starts the program at:
//
//   6     LOD 2 list, no morph
//   8     LOD 1 lists; LOD-01 vertices morph and collapse onto parent 1 when
//         both parents are beyond D0
//   0xa   LOD 1 lists; LOD-01 vertices morph
//   0xe   LOD 0 lists; LOD-01 morph, LOD-0 morph with collapse at D1
//   0x10  LOD 0 lists; LOD-0 morph
//   0x14  LOD 0 lists, no morph
//
// with D0, D1, D2 = 6L, 4L, 2L from the tfrag block's base distance L. The
// vertex shader (shaders/world/tfrag.vert) replays the chosen entry's morph
// and collapse passes.

#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "renderer/math.h"
#include "renderer/world/draw_data.h"
#include "renderer/world/fog.h"
#include "renderer/world/world_frame.h"

namespace openrac::renderer::world {

inline constexpr std::uint32_t kTfragCulled = 0;
inline constexpr std::uint32_t kTfragClip = 2;

// The strip list a draw mode draws: 2 for MSCAL 6, 1 for 8 and 0xa, else 0.
int tfrag_mode_list(std::uint32_t mode);

// D0, D1, D2 (game units) and TfragProc's integer thresholds trunc(D * 1024).
std::array<float, 3> tfrag_lod_distances(float lod_base);
std::array<std::int32_t, 3> tfrag_lod_thresholds(float lod_base);

// The LOD rule for a tfrag inside the view: near / far = centre depth -/+
// radius (integer units), truncated.
std::uint32_t tfrag_lod_mode(
    float centre_depth, float radius, bool base_only, const std::array<std::int32_t, 3>& thresholds
);

// TfragProc for one tfrag (after its occlusion test), the camera in integer
// units (eye x 1024) and its VU rows (WorldCamera::vu_rows):
//   - cull if the sphere is wholly beyond the far cull or in front of the
//     near plane, or wholly outside a side plane (tan * z - |x| + r * k < 0,
//     k = 1 / cos(atan tan));
//   - wholly inside the view: the LOD rule;
//   - otherwise the 8 box corners are transformed by the view frustum as a
//     clip volume (w = z / n) and CLIP-tested twice, as they are and with x, y
//     x 0.25 (the 4x guard band): all outside one plane culls; any outside the
//     guard band takes the clipping path; else the LOD rule.
std::uint32_t tfrag_proc(
    const TfragCull& tfrag,
    const Vec3& camera_int,
    const std::array<Vec3, 3>& rows,
    const std::array<std::int32_t, 3>& thresholds,
    float tan_x,
    float tan_y,
    float far_cull
);

// SetTfragDists' VU1 constants qw666..669 (f32, the EE's operation order)
// and the w slope per integer unit of depth. With cf20 = (If - In) / ((Df -
// Dn) / 1024), fi = Di * cf20, a = 1 / (f0 - f1), b = 1 / (f1 - f2):
// qw666 = (a/2, -a, 0, f0), qw667 = (b/2, -b, 0, f1),
// qw668 = (-f1 a/2, f0 a, 0, 0), qw669 = (-f2 b/2, f1 b, 0, 0).
// The VU then takes t.xy = clamp(w * qw66x.xy + qw66y.xy, 0, (0.5, 1)) =
// (u / 2, 1 - u) and collapses when both parents have w < qw66x.w.
struct TfragLodConstants {
    std::array<float, 4> k666{};
    std::array<float, 4> k667{};
    std::array<float, 4> k668{};
    std::array<float, 4> k669{};
    float w_slope = 0.0f;
};

TfragLodConstants tfrag_lod_constants(float lod_base, const WorldFog& fog);

// Every tfrag's mode this frame, occlusion first (as TfragProc tests the
// mask before the sphere).
void tfrag_modes(
    std::span<const TfragCull> tfrags,
    float lod_base,
    const WorldCamera& camera,
    std::span<const std::uint8_t> occlusion,
    float far_cull,
    std::span<std::uint32_t> out
);

}  // namespace openrac::renderer::world
