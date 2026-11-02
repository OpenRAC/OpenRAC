// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

// Going Commando's tie lighting again at run time, for a tie the game moves (a moby's tie group:
// the Maktar lamps' heads turn). The mover marks the instance (func_L00_00323958 through
// func_L00_0036AE00), and PatchTieVertexLighting (0x2F3E78) lights its slots again by its matrix
// now: each slot's colour plus its two directional lights on the slot's normal turned into the
// world, then the class's remap spreads the slots over the vertices. The export lit each tie once,
// by its matrix at load (editor/gc_tie_colours.py); format 15 keeps what it was lit from
// (LevelData::tie_relight), and this lights it the same way, in the same float32 steps rounded
// toward zero as the editor's, which match retail's cache byte for byte.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "renderer/math.h"

namespace openrac::scene {

// A tie class's relighting data (manifest classes.tie[id].relight).
struct TieRelightClass {
    // Each slot's normal code: azimuth | elevation << 8 (turns / 256).
    std::vector<std::uint16_t> normals;
    // Per vertex of the class's mesh (in its light-colour order): kind, a, b, c, d. Kind 0 none,
    // 1 slot a, 2 (a + b) >> 1, 3 (3a + b) >> 2, 4 (2a + b + c) >> 2, 5 (a + b + c + d) >> 2.
    std::vector<std::uint16_t> vertices;
    // Slot 511's colour, the class's flat colour, when it has one.
    std::optional<std::uint32_t> flat;
    // Each slot's position (s16 x, y, z) and the class scale: the point lights' pass places a slot
    // at the instance's matrix times the position times scale / 1024.
    std::vector<std::int16_t> positions;
    float scale = 1.0f;
};

// One of the game's eight point lights as it holds them (the light bank's 0x20-byte records after
// its 16 sets): colour (byte / 128), falloff, position, reach.
struct TiePointLight {
    std::array<float, 3> colour{};
    float falloff = 0.0f;
    std::array<float, 3> position{};
    float reach = 0.0f;
};

// An instance's: its class's entry in LevelData::tie_relight_classes (-1 none), its slots before
// lighting (RGBA words, 0x80 = 1.0) as a range of LevelData::tie_slots, and its light set.
struct TieRelight {
    int cls = -1;
    std::uint32_t first = 0;
    std::uint16_t count = 0;
    std::uint8_t light_set = 0;
};

// The instance's vertex colours (RGBA words, 0x80 = 1.0) lit by `set` (16 floats: colour A with
// its back factor in w, direction A, colour B, direction B) for the class-to-world matrix `m`,
// into `out` (one per vertex of the class). `points`: the point lights its list names (the
// instance record's +0x1A nibbles, at most four), each added to every slot whose instance's
// bounding sphere `sphere` (centre, radius) the light reaches (PatchTieVertexLighting at 0x2F4688
// and its VU0 program at 0x580): colour x 128 x (1 - d / r) x max(N . (L - P) / d + 1 - d / r, 0),
// truncated, at least 0, the sum clamped at 255.
void relight_tie(
    const TieRelightClass& cls,
    std::span<const std::uint32_t> slots,
    const renderer::Mat4& m,
    std::span<const float, 16> set,
    std::vector<std::uint32_t>& out,
    std::span<const TiePointLight> points = {},
    const std::array<float, 4>& sphere = {}
);

}  // namespace openrac::scene
