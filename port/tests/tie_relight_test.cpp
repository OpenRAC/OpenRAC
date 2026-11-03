// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's tie relighting (scene/tie_relight.h): the directional pass, the remap kinds,
// the point lights' pass and its first slot, which the game turns by vf17 to vf19 (zero with both
// back factors 0) instead of the unit columns.

#include "scene/tie_relight.h"
#include "tests/check.h"

using namespace openrac::scene;

namespace {

// A class of two slots facing +z (normal code: elevation 64 = a quarter turn), at the origin and at
// (0, 0, 1024) (one unit at scale 1), each a vertex of its own, then their average.
TieRelightClass two_slots() {
    TieRelightClass c;
    c.normals = {0x4000, 0x4000};
    c.positions = {0, 0, 0, 0, 0, 1024};
    c.scale = 1.0f;
    c.vertices = {1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 2, 0, 1, 0, 0};
    return c;
}

constexpr std::uint32_t kGrey = 0x80404040u;  // 0.5 a channel, alpha 0x80

openrac::renderer::Mat4 identity() {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

void directional() {
    // A white light A along -z (shining down on the +z normals), B dark: + 128 a channel.
    const float set[16] = {1, 1, 1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0};
    std::vector<std::uint32_t> out;
    const std::uint32_t slots[2] = {kGrey, kGrey};
    relight_tie(two_slots(), slots, identity(), std::span<const float, 16>(set, 16), out);
    CHECK(out.size() == 3);
    CHECK(out[0] == 0x80C0C0C0u);
    CHECK(out[1] == 0x80C0C0C0u);
    CHECK(out[2] == 0x80C0C0C0u);
}

void point_light() {
    // No directional light; a red light 1 unit above slot 1 with reach 4, the sphere reaching it.
    const float set[16] = {};
    const std::uint32_t slots[2] = {kGrey, kGrey};
    TiePointLight p;
    p.colour = {1, 0, 0};
    p.position = {0, 0, 2};
    p.reach = 4;
    std::vector<std::uint32_t> out;
    relight_tie(
        two_slots(),
        slots,
        identity(),
        std::span<const float, 16>(set, 16),
        out,
        std::span<const TiePointLight>(&p, 1),
        {0, 0, 0.5f, 1}
    );
    // Slot 1: d = 1, fall 0.75, N . L 1: 128 x 0.75 x 1.75 = 168 added.
    CHECK((out[1] & 0xFF) == 0x40 + 168);
    CHECK(((out[1] >> 8) & 0xFF) == 0x40);
    // Slot 0 (its normal taken as 0): d = 2, fall 0.5: 128 x 0.5 x 0.5 = 32 added.
    CHECK((out[0] & 0xFF) == 0x40 + 32);
    // The average of the two, and a brighter light clamps at 255.
    CHECK((out[2] & 0xFF) == (0x60 + 0xE8) / 2);
    p.colour = {4, 0, 0};
    std::vector<std::uint32_t> bright;
    relight_tie(
        two_slots(),
        slots,
        identity(),
        std::span<const float, 16>(set, 16),
        bright,
        std::span<const TiePointLight>(&p, 1),
        {0, 0, 0.5f, 1}
    );
    CHECK((bright[1] & 0xFF) == 0xFF);
    p.colour = {1, 0, 0};
    // Out of the sphere's reach: nothing added.
    std::vector<std::uint32_t> far;
    relight_tie(
        two_slots(),
        slots,
        identity(),
        std::span<const float, 16>(set, 16),
        far,
        std::span<const TiePointLight>(&p, 1),
        {0, 0, 50, 1}
    );
    CHECK(far[1] == kGrey);
}

}  // namespace

int main() {
    directional();
    point_light();
    return openrac::test::result();
}
