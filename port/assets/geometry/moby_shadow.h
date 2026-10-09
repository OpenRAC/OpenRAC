// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/moby_shadow.rs
// (spec: docs/plan/shadows.md section 2): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// RAC1 moby class shadow blocks: the proxy shapes a class projects as its
// dynamic shadow. The only reader is BuildShadowList (NTSC-U level 1
// 0x29b948, boot 0x227740).
//
// Class header byte 0x0f is the block's size in quadwords; the block ends at
// the skeleton (class +0x14). A class with 0x0f = 0 casts no shadow. Records
// follow one another until the one whose `last` byte is set:
//
//   0x00 u8    type: 0 sphere (0x20 bytes), 1 capsule (0x30 bytes)
//   0x01 u8    last
//   0x02 u16   size
//   0x04       sphere: s32 joint; capsule: u16 joint A, u16 joint B
//   0x08 s32   sphere: outline segments; capsule: segments at end A
//   0x0c s32   capsule: segments at end B
//   0x10 vec4  sphere: centre (joint space, packed model units), w = radius;
//              capsule: end A, w = radius at A
//   0x20 vec4  capsule: end B, w = radius at B
//
// A capsule end with 0 segments is flat; -k pushes that end away from the
// other by k * 16 / 4096 of the segment and makes it flat (0x29bb98).

#pragma once

#include <array>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "assets/bytes.h"
#include "assets/geometry/core_records.h"

namespace openrac::assets::rac1 {

struct ShadowSphere {
    s32 joint = 0;
    s32 segments = 0;
    std::array<f32, 4> centre{};  // w = radius

    bool operator==(const ShadowSphere&) const = default;
};

struct ShadowCapsule {
    std::array<u16, 2> joints{};
    std::array<s32, 2> segments{};
    std::array<f32, 4> a{};  // w = radius at a
    std::array<f32, 4> b{};

    bool operator==(const ShadowCapsule&) const = default;
};

using ShadowPrimitive = std::variant<ShadowSphere, ShadowCapsule>;

struct ShadowBlock {
    std::vector<ShadowPrimitive> primitives;

    bool operator==(const ShadowBlock&) const = default;
};

// Throws on a type other than 0 and 1, a size word that disagrees with the
// type, a record past the block, or a block that does not end on its last
// record.
ShadowBlock parse_shadow_block(ByteView block);

// The block of a class blob, or nothing when class byte 0x0f is 0.
std::optional<ShadowBlock> moby_class_shadow(ByteView class_blob);

struct LevelMobyShadow {
    s32 o_class = 0;
    u8 joint_count = 0;
    ShadowBlock block;
};

// Every class of a level that casts a shadow, in table order.
std::vector<LevelMobyShadow> parse_level_moby_shadows(
    std::span<const CoreClassEntry> classes, ByteView core_data
);

}  // namespace openrac::assets::rac1
