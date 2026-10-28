// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
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
}  // namespace

int main() {
    test_opaque_ranges();
    test_material_coverage();
    return openrac::test::result();
}
