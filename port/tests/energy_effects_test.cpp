// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the energy material's effects (game/common/frontend/energy_effects.h, interface 1.25): which
// materials give a look, the colour an effect quad takes at its own brightness, brighter with the
// level, and a particle's corners spread from its centre. Needs no game and no window.

#include <cmath>
#include <cstdint>

#include "game/common/frontend/energy_effects.h"
#include "tests/check.h"

namespace {

namespace energy = openrac::frontend::energy;

// A quad as the renderer's EffectQuad holds one: four corners and four colour words.
struct Quad {
    float corner[4][4];
    std::uint32_t rgba[4];
};

// Returns channel `shift` of the chip's word `abgr`.
std::uint32_t channel(std::uint32_t abgr, int shift) {
    return (abgr >> shift) & 0xFFu;
}

}  // namespace

int main() {
    // Only energy gives a look; its level is bits 8 to 11, its colour the rgb.
    CHECK(!energy::look_of(OPENRAC_MOBY_MATERIAL_TINT, 0x20FF40));
    CHECK(!energy::look_of(OPENRAC_MOBY_MATERIAL_OBSIDIAN, 0x20FF40));
    const auto look = energy::look_of(OPENRAC_MOBY_MATERIAL_ENERGY_AT(8), 0x20FF40);
    CHECK(look && look->level == 8 && look->rgb == 0x20FF40);
    CHECK(energy::look_of(OPENRAC_MOBY_MATERIAL_ENERGY, 0xFF0000)->level == 0);

    // A blue flash (the chip's word: R low, 0x80 = 1.0) comes out green at its own brightness, its
    // alpha as it was.
    const std::uint32_t blue = 0x60FF4010u;
    const std::uint32_t green = energy::colour(blue, *look);
    CHECK(channel(green, 24) == 0x60);
    CHECK(channel(green, 8) > 2 * channel(green, 0) && channel(green, 8) > 2 * channel(green, 16));
    // White at the top level saturates the colour's own channel, never past 0xFF.
    const energy::Look top{0x20FF40, OPENRAC_MOBY_MATERIAL_ENERGY_TOP};
    CHECK(channel(energy::colour(0x80FFFFFFu, top), 8) == 0xFF);
    // A higher level is brighter; black stays black.
    const energy::Look low{0x20FF40, 0};
    CHECK(
        channel(energy::colour(0x80404040u, top), 8) > channel(energy::colour(0x80404040u, low), 8)
    );
    CHECK((energy::colour(0x80000000u, top) & 0x00FFFFFFu) == 0);
    CHECK(energy::gain(OPENRAC_MOBY_MATERIAL_ENERGY_TOP) / energy::gain(0) > 1.3f);
    CHECK(std::fabs(energy::size(OPENRAC_MOBY_MATERIAL_ENERGY_TOP) - 4.0f / 3.0f) < 1e-4f);
    CHECK(energy::size(0) == 1.0f);

    // A particle grows from its centre; a callback's quad keeps its corners.
    Quad quad{
        {{-1, -1, 2, 1}, {1, -1, 2, 1}, {-1, 1, 2, 1}, {1, 1, 2, 1}}, {blue, blue, blue, blue}
    };
    Quad ribbon = quad;
    energy::apply(quad, top, true);
    energy::apply(ribbon, top, false);
    CHECK(std::fabs(quad.corner[3][0] - 4.0f / 3.0f) < 1e-4f);
    CHECK(std::fabs(quad.corner[0][1] + 4.0f / 3.0f) < 1e-4f);
    CHECK(quad.corner[2][2] == 2.0f);
    CHECK(ribbon.corner[3][0] == 1.0f);
    CHECK(quad.rgba[0] == energy::colour(blue, top) && ribbon.rgba[3] == quad.rgba[3]);
    return openrac::test::result();
}
