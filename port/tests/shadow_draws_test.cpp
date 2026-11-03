// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <limits>
#include <vector>

#include "scene/shadow_draws.h"
#include "tests/check.h"

namespace {
using namespace openrac::scene;

// Checks geometry order, shader choice and the material needed for alpha testing.
void check_range(
    const ShadowDrawRange& range, unsigned first, unsigned count, int material, bool solid
) {
    CHECK(range.first == first);
    CHECK(range.count == count);
    CHECK(range.material == material);
    CHECK(range.solid == solid);
}

// Adjacent opaque materials can share a depth draw; index gaps and unsafe vertices split it.
void test_opaque_ranges() {
    const std::array<Material, 2> materials{};
    const std::array<Primitive, 4> primitives{{{0, 3, 0}, {3, 6, 1}, {15, 3, 0}, {18, 3, 1}}};
    const std::vector<bool> eligible{true, true, true, false};
    const auto solid = prepare_shadow_draws(primitives, materials, eligible, true);
    CHECK(solid.size() == 3);
    if (solid.size() == 3) {
        check_range(solid[0], 0, 9, -1, true);
        check_range(solid[1], 15, 3, -1, true);
        check_range(solid[2], 18, 3, -1, false);
    }
    const auto alpha = prepare_shadow_draws(primitives, materials, eligible, false);
    CHECK(alpha.size() == 2);
    if (alpha.size() == 2) {
        check_range(alpha[0], 0, 9, -1, false);
        check_range(alpha[1], 15, 6, -1, false);
    }
}

// Cutouts and partial opacity keep their materials; blended, metal and invisible faces do not cast.
void test_material_coverage() {
    std::array<Material, 6> materials{};
    materials[1].cutout = true;
    materials[2].colour[3] = .5f;
    materials[3].blend = true;
    materials[4].metal = true;
    materials[5].colour[3] = .49f;
    const std::array<Primitive, 7> primitives{
        {{0, 3, 0}, {3, 3, 1}, {6, 3, 2}, {9, 3, 3}, {12, 3, 4}, {15, 3, 5}, {18, 3, 0}}
    };
    const auto ranges =
        prepare_shadow_draws(primitives, materials, std::vector<bool>(7, true), true);
    CHECK(ranges.size() == 4);
    if (ranges.size() == 4) {
        check_range(ranges[0], 0, 3, -1, true);
        check_range(ranges[1], 3, 3, 1, false);
        check_range(ranges[2], 6, 3, 2, false);
        check_range(ranges[3], 18, 3, -1, true);
    }
    // A new upload's material changes must produce fresh classification, even for the same geometry.
    materials[1].cutout = false;
    materials[2].colour[3] = 1;
    const auto rebuilt =
        prepare_shadow_draws(primitives, materials, std::vector<bool>(7, true), true);
    CHECK(rebuilt.size() == 2);
    if (rebuilt.size() == 2) {
        check_range(rebuilt[0], 0, 9, -1, true);
        check_range(rebuilt[1], 18, 3, -1, true);
    }
    CHECK(prepare_shadow_draws({}, materials, {}, true).empty());
}

// Bounds cover every referenced position; malformed and exceptional ranges survive.
void test_shadow_geometry() {
    std::vector<Vertex> vertices(4);
    vertices[0].position[0] = -2;
    vertices[1].position[0] = 3;
    vertices[2].position[1] = 4;
    vertices[3].position[2] = std::numeric_limits<float>::quiet_NaN();
    std::vector<std::uint32_t> indices;
    for (unsigned i = 0; i < 129; ++i) {
        indices.insert(indices.end(), {0, 1, 2});
    }
    indices.insert(indices.end(), {0, 1, 3});
    const std::array<ShadowDrawRange, 7> draws{
        {{0, 387, -1, true},
         {0, 3, 2, false},
         {387, 3, -1, true},
         {999, 3, -1, true},
         {387, 3, 2, false},
         {999, 3, 2, false},
         {0, 2, 2, false}}
    };
    const auto geometry = prepare_shadow_geometry(draws, vertices, indices);
    CHECK(geometry.size() == 8);
    if (geometry.size() != 8) {
        return;
    }
    check_range(geometry[0].draw, 0, 384, -1, true);
    check_range(geometry[1].draw, 384, 3, -1, true);
    CHECK(geometry[0].bounded && geometry[1].bounded);
    CHECK((geometry[0].low == openrac::renderer::Vec3{-2, 0, 0}));
    CHECK((geometry[0].high == openrac::renderer::Vec3{3, 4, 0}));
    check_range(geometry[2].draw, 0, 3, 2, false);
    CHECK(geometry[2].bounded && !geometry[3].bounded && !geometry[4].bounded);
    CHECK(geometry[2].low == geometry[0].low && geometry[2].high == geometry[0].high);
    CHECK(!geometry[5].bounded && !geometry[6].bounded && !geometry[7].bounded);
    check_range(geometry[5].draw, 387, 3, 2, false);
    check_range(geometry[6].draw, 999, 3, 2, false);
    check_range(geometry[7].draw, 0, 2, 2, false);
    CHECK(prepare_shadow_geometry({}, vertices, indices).empty());
}
}  // namespace

int main() {
    test_opaque_ranges();
    test_material_coverage();
    test_shadow_geometry();
    return openrac::test::result();
}
