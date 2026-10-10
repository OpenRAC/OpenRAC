// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/game_camera.rs
// (LevelFog, TfragFog), crates/rc-engine/src/fog_state.rs and crates/rc-engine/src/tie_lod.rs
// (fog_value): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The world's fog as the vector-unit programs compute it and the GS applies
// it. The game keeps four fog values in its view context (UpdateFog, ReRAC's
// reading of the NTSC-U executable): the depths, in integer units (game units
// x 1024), at which the fog starts and ends, and the GS fog coefficient F at
// those depths (255 = no fog). UpdateViewContext turns them into a line in the
// projection's w lane, so a program writes, per vertex,
//
//     F = trunc(clamp(depth * slope + offset, far_intensity, near_intensity))
//
// (the tfrag program: w = depth * slope, plus an offset, clamped, then ftoi4
// into XYZF2's F), and the GS blends RGB only: C = FOGCOL + (C - FOGCOL) * F / 255.
// The tie program writes one F per instance, from the depth of the instance's
// bounding-sphere centre, computed on the EE with the console's float
// rounding (tie_fog_value). Which four values apply each frame (fog zones,
// underwater) is game state; the world renderers take them from WorldFrame.

#pragma once

#include <array>
#include <cstdint>

namespace openrac::renderer::world {

// Integer units per game unit: the world programs work in game units x 1024.
inline constexpr float kUnits = 1024.0f;
// The view context's near plane (integer units). The GS's Q is near / depth,
// so the GS mip rule is LOD = log2(depth / near) + K.
inline constexpr float kNear = 32.0f;
// The view context's far plane (integer units).
inline constexpr float kFar = 745472.0f;

struct WorldFog {
    std::array<std::uint8_t, 3> colour{0, 0, 0};  // FOGCOL, display bytes
    float near_dist = 0.0f;                       // integer units
    float far_dist = 1.0f;
    float near_intensity = 255.0f;  // F at near_dist (255 = no fog)
    float far_intensity = 255.0f;   // F at far_dist
    bool enabled = true;

    // UpdateViewContext's terms in f32, the EE's order: the Q numerator, the
    // w-lane slope per integer unit of depth and the offset.
    struct Terms {
        float q_numerator;
        float slope;
        float offset;
    };

    Terms terms() const;

    // F for a depth in integer units, as the tfrag program computes it
    // (0..255, before the truncation).
    float vertex_f(float depth_int) const;
};

// The four floats the shaders read (WorldView.fog_params): slope, offset,
// lower clamp (far intensity), upper clamp (near intensity).
std::array<float, 4> fog_params(const WorldFog& fog);

// The tie program's per-instance F (TieProc, ReRAC's reading: cf20 and cf24
// from UpdateViewContext, depth * cf20 + cf24 clamped between the
// intensities, ftoi4), computed with the console's float arithmetic.
// `depth` in game units (the clamped view depth of the bounding-sphere
// centre).
std::uint32_t tie_fog_value(float depth, const WorldFog& fog);

}  // namespace openrac::renderer::world
