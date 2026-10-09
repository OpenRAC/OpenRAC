// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/moby_collision.rs
// (spec: docs/plan/collision_queries.md, "Moby collision in the port"): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// RAC1 moby class collision: the blob at class header +0x10, which the game
// copies to moby +0x94 when it creates an instance. Its only readers are the
// collision query kernels (NTSC-U level 1: the line 0x211870, sphere
// 0x212960 and capsule 0x2135a0 tests, coll_sphere_mobys 0x214468).
//
//   0x00 u16   joints to pose for queries with flag 0x4 (mask bit 0)
//   0x02 u16   joints to pose for the other queries (mask bit 1)
//   0x04 s32   primitive section bytes (0x20 per primitive)
//   0x08 s32   face section bytes (4 per triangle)
//   0x0c s32   vertex section bytes (8 per vertex)
//   0x10       primitives, then vertices, then faces
//
// A primitive is 0x20 bytes: s8 kind at +0, a byte at +1 (4 on every retail
// primitive, not read), s16 mask at +2 (bit 0 and bit 1 choose the queries;
// bit 15 ends the list: the kernels walk until a negative mask), then by kind:
//
//   1  sphere                  centre xyz and radius w at +0x10 (model units)
//   2  sphere on a joint       s32 joint at +4, radius f32 at +0xc, offset xyz at +0x10
//   3  vertical cylinder       base centre xyz and radius w at +0x10, height f32 at +4
//   4  capsule between joints  s16 joints at +4 and +6, radius f32 at +0xc
//
// Vertices are s16 x, y, z, pad in model units (the class scale applies at
// run time); faces are the world mesh's 4-byte {v0, v1, v2, type}, normal
// (v2 - v0) x (v1 - v0). On the NTSC-U disc ReRAC counts 1064 blobs.

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "assets/bytes.h"
#include "assets/geometry/core_records.h"

namespace openrac::assets::rac1 {

// One primitive, kept as its eight words so floats keep their bits.
struct MobyCollisionPrimitive {
    std::array<u32, 8> raw{};

    s8 kind() const { return static_cast<s8>(raw[0] & 0xff); }

    u8 byte1() const { return static_cast<u8>(raw[0] >> 8); }

    s16 mask() const { return static_cast<s16>(raw[0] >> 16); }

    // +4 as s32: kind 2's joint.
    s32 word4() const { return static_cast<s32>(raw[1]); }

    // +4 and +6 as s16: kind 4's joints.
    std::array<s16, 2> joints() const {
        return {static_cast<s16>(raw[1] & 0xffff), static_cast<s16>(raw[1] >> 16)};
    }

    // The float at word i (+4 * i).
    f32 f(std::size_t i) const;

    bool operator==(const MobyCollisionPrimitive&) const = default;
};

struct MobyCollision {
    std::array<u16, 2> joint_counts{};  // posed for flag-0x4 queries, for the others
    std::vector<MobyCollisionPrimitive> primitives;
    std::vector<std::array<s16, 4>> vertices;  // x, y, z, pad
    std::vector<std::array<u8, 4>> faces;      // v0, v1, v2, type

    // The blob's size in bytes.
    std::size_t size() const;

    bool operator==(const MobyCollision&) const = default;
};

// Throws on section sizes that are not whole records, a primitive list
// without its end bit (or with an early one), or a face past the vertices.
MobyCollision parse_moby_collision(ByteView blob);

// The collision of a class blob, or nothing when class +0x10 is 0.
std::optional<MobyCollision> moby_class_collision(ByteView class_blob);

struct LevelMobyCollision {
    s32 o_class = 0;
    MobyCollision collision;
};

// Every class of a level that has a collision blob, in table order.
std::vector<LevelMobyCollision> parse_level_moby_collisions(
    std::span<const CoreClassEntry> classes, ByteView core_data
);

}  // namespace openrac::assets::rac1
