// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/volumes.rs,
// crates/rc-formats/src/water.rs (`parse_cuboids`) and the volume tests of
// crates/rc-game/src/moby_update/triggers.rs: ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// The gameplay file's volumes: the shapes trigger tests read (cuboids 0x60,
// spheres 0x64, cylinders 0x68, pills 0x6c) and the grind paths (0x74).
// Specification: ReRAC's docs/plan/triggers.md, the record from
// docs/formats/collision_rac1.md section 6.2.
//
// Each shape section is `s32 count, pad[3]`, then count 0x80-byte records,
// which the loader copies to its tables (NTSC-U level01 0x255958). Pills are the
// exception: the runtime stride is 0x90 and the loader copies 0x90 bytes from
// each 0x80-byte record, so the runtime +0x80 word (the pill's cap radius the
// camera grid reads) is the first word after the record in the file. No rac1
// level has a pill.
//
// The point tests here are the engine's (the same code in all 19 level
// overlays), in IEEE floats: a point on a boundary can differ by an ulp from
// the console's result.

#pragma once

#include <array>
#include <span>
#include <vector>

#include "assets/world/gameplay.h"

namespace openrac::assets {

enum class ShapeKind : u8 {
    Cuboid,
    Sphere,
    Cylinder,
    Pill,
};

inline constexpr std::size_t kShapeRecordSize = 0x80;

// The volume type numbers of the camera collision grid's primitives: 3 cuboid,
// 5 sphere, 6 cylinder, 7 pill.
s32 shape_grid_type(ShapeKind kind);

// One shape record.
struct GameplayShape {
    // +0x00: the forward transform, rows of a row-vector matrix; row 3 is the
    // centre. A cuboid is the unit box [-1, 1]^3 under it.
    std::array<std::array<float, 4>, 4> matrix{};
    // +0x40: world -> local rows the tests apply to p - centre.
    std::array<std::array<float, 4>, 3> inverse{};
    std::array<float, 3> euler{};  // +0x70
    float unused_7c = 0;

    std::array<float, 3> centre() const { return {matrix[3][0], matrix[3][1], matrix[3][2]}; }

    // (p - centre) through the inverse rows: l = d.x * inv0 + d.y * inv1 +
    // d.z * inv2 (VecSub 0x2211b8, MatrixMulVec3 0x2215e0).
    std::array<float, 3> local(const std::array<float, 3>& p) const;

    // A local point back to the world: l.x * row0 + l.y * row1 + l.z * row2 + row3.
    std::array<float, 3> world(const std::array<float, 3>& l) const;
};

std::vector<GameplayShape> read_shapes(const GameplayFile& file, ShapeKind kind);

// Per pill, the word the loader's 0x90-byte copy puts at runtime +0x80.
std::vector<float> read_pill_cap_radii(const GameplayFile& file);

// A grind rail: header records of 0x20 bytes after `s32 count, s32
// data_offset, s32 data_size, pad`, then count s32 spline offsets into the
// data, each spline as a path's.
struct GrindPath {
    std::array<float, 4> bounding_sphere{};
    s32 flag = 0;  // +0x10: 0 or 1 on the disc
    s32 wrap = 0;  // +0x14: a closed loop (Wrench's name)
    s32 inactive = 0;
    std::vector<std::array<float, 4>> points;
};

std::vector<GrindPath> read_grind_paths(const GameplayFile& file);

// -1 <= l <= 1 on all three axes: the bound of every box test.
bool in_unit_box(const std::array<float, 3>& l);

// PointInCuboid (NTSC-U level01 0x274820): the unit box.
bool point_in_cuboid(const GameplayShape& cuboid, const std::array<float, 3>& p);

// PointInCylinder (0x2748f8): |l.xy| < 1 and -1 <= l.z <= 1.
bool point_in_cylinder(const GameplayShape& cylinder, const std::array<float, 3>& p);

// PointInSphere (0x2749b0): |l| < 1.
bool point_in_sphere(const GameplayShape& sphere, const std::array<float, 3>& p);

// PointInPathPolygon (0x26e6c0): the path's points as a closed polygon in XY
// (z ignored), even-odd. Edge k runs from point k to k + 1 (the last back to
// 0) and counts when p.y lies in (min, max] of its y and the crossing x is left
// of p.x.
bool point_in_path_polygon(
    const std::array<float, 3>& p, std::span<const std::array<float, 4>> points
);

}  // namespace openrac::assets
