// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A plain triangle mesh: what the geometry readers hand on once the console's
// packet machinery (strips, GS slots, vertex caches) has been resolved. The
// renderer converts it into its own buffers; the readers keep the raw records
// too for anyone who needs the game's exact data.
//
// Also the packed normal every RAC1 geometry kind uses: two bytes of
// spherical angles (ReRAC docs/formats/moby_rac1.md, docs/plan/tfrag_lighting.md).

#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

struct MeshVertex {
    std::array<f32, 3> position{};  // world or class units, as the producer says
    std::array<f32, 2> uv{};
    std::array<u8, 4> rgba{0x80, 0x80, 0x80, 0x80};  // 0x80 = 1.0, as the GS modulates
    std::array<f32, 3> normal{};  // unit length, or zero when the format has none
};

struct MeshTriangle {
    std::array<u32, 3> vertices{};
    // The texture the triangle is drawn with, in the producer's terms (a
    // level texture table index, a class texture slot, ...); -1 for none.
    s32 texture = -1;
};

struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> triangles;
};

// The unit normal of a packed (azimuth, elevation) pair, 2 pi / 256 per step:
// (cos a cos e, sin a cos e, sin e), Z up. This is the convention of the VU0
// code that reads it (Wrench swaps x and y). Computed in double and rounded;
// the game's own float table differs from this by at most 3.5e-7 per entry.
inline std::array<f32, 3> spherical_normal(u8 azimuth, u8 elevation) {
    const double k = std::numbers::pi / 128.0;
    const double a = azimuth * k;
    const double e = elevation * k;
    return {
        static_cast<f32>(std::cos(a) * std::cos(e)),
        static_cast<f32>(std::sin(a) * std::cos(e)),
        static_cast<f32>(std::sin(e)),
    };
}

}  // namespace openrac::assets
