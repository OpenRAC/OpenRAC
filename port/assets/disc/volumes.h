// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/volumes.rs and
// crates/rc-formats/src/gameplay.rs (the path section): ISC License, Copyright (c) 2026
// ReRAC contributors.
//
// RAC1's gameplay volumes: the four shape sections the trigger tests read
// (cuboids 0x60, spheres 0x64, cylinders 0x68, pills 0x6c, pointers in the
// gameplay file's header), the paths (0x70) and the grind paths (0x74).
// NTSC-U's loader (InitLevelRenderGlobals 0x255958) copies them into tables:
//
//   0x60 cuboids     count 0x1600f0, base 0x1600ec, stride 0x80, PointInCuboid 0x274820
//   0x64 spheres     count 0x1600f8, base 0x1600f4, stride 0x80, PointInSphere 0x2749b0
//   0x68 cylinders   count 0x160100, base 0x1600fc, stride 0x80, PointInCylinder 0x2748f8
//   0x6c pills       count 0x1600e8, base 0x1600e4, stride 0x90, the camera grid only
//   0x70 paths       count 0x160104, 0x1b0930[i]
//   0x74 grind paths count 0x15f710, base 0x15f70c, stride 0x20
//
// A shape section is `s32 count, pad[3]` then count 0x80-byte records. Pills
// are copied 0x90 bytes from each 0x80-byte record, so a pill's runtime +0x80
// word (the cap radius the camera grid reads) is the first word after its
// record in the file. No RAC1 level has a pill.

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

inline constexpr std::size_t kCuboidsPointer = 0x60;
inline constexpr std::size_t kSpheresPointer = 0x64;
inline constexpr std::size_t kCylindersPointer = 0x68;
inline constexpr std::size_t kPillsPointer = 0x6c;
inline constexpr std::size_t kPathsPointer = 0x70;
inline constexpr std::size_t kGrindPathsPointer = 0x74;
inline constexpr std::size_t kShapeSize = 0x80;
inline constexpr std::size_t kPillRuntimeSize = 0x90;
inline constexpr std::size_t kGrindPathSize = 0x20;

enum class ShapeKind : u8 {
    Cuboid,
    Sphere,
    Cylinder,
    Pill,
};

std::size_t shape_pointer(ShapeKind kind);

// The camera-collision grid's type tag: 3 cuboid, 5 sphere, 6 cylinder, 7 pill.
s32 shape_grid_type(ShapeKind kind);
std::optional<ShapeKind> shape_from_grid_type(s32 type);

using Vec3 = std::array<f32, 3>;
using Vec4 = std::array<f32, 4>;

// One shape record (0x80 bytes, the same for all four kinds): a canonical
// primitive in local space mapped to the world by `matrix`. Cuboid: |l.x|,
// |l.y|, |l.z| <= 1; sphere |l| < 1; cylinder |l.xy| < 1, |l.z| <= 1; pill: the
// cylinder plus cap spheres at l.z = +-1.
struct Shape {
    // Local to world, row-vector form: world = l.x row0 + l.y row1 + l.z row2
    // + row3; row 3 is the centre.
    std::array<Vec4, 4> matrix;
    // World to local rotation and scale, applied to p - centre.
    std::array<Vec4, 3> inverse;
    Vec3 euler;  // read only by classes that use a cuboid as a marker
    f32 unused_7c;

    Vec3 centre() const;
    Vec3 local(Vec3 p) const;
    Vec3 world(Vec3 l) const;

    bool operator==(const Shape&) const = default;
};

static_assert(sizeof(Shape) == kShapeSize);

struct GrindPath {
    Vec4 bsphere;  // x, y, z, radius
    s32 flag;      // 0 or 1 on the disc
    std::vector<Vec4> points;

    bool operator==(const GrindPath&) const = default;
};

struct Volumes {
    std::vector<Shape> cuboids;
    std::vector<Shape> spheres;
    std::vector<Shape> cylinders;
    std::vector<Shape> pills;
    std::vector<f32> pill_tail;  // each pill's runtime +0x80 word
    std::vector<std::vector<Vec4>> paths;
    std::vector<GrindPath> grind_paths;

    std::span<const Shape> shapes(ShapeKind kind) const;
    const Shape* shape(ShapeKind kind, s32 index) const;
    std::optional<f32> pill_cap_radius(s32 index) const;
};

std::vector<Shape> parse_shapes(ByteView gameplay, ShapeKind kind);

// Section 0x70: {s32 count, s32 data offset, s32 data size, pad}, count s32
// spline offsets into the data; a spline is s32 count, pad[3], count vec4.
std::vector<std::vector<Vec4>> parse_paths(ByteView gameplay);

// Section 0x74: {s32 count, s32 data offset, s32 data size, pad}, count 0x20
// records {f32 bsphere[4], s32 flag, pad[3]}, count s32 spline offsets.
std::vector<GrindPath> parse_grind_paths(ByteView gameplay);

Volumes parse_volumes(ByteView gameplay);

}  // namespace openrac::assets::disc
