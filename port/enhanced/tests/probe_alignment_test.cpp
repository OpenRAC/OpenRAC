// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Analytic thin walls verify grid phase scores, fixed coverage and deterministic placement.

#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <exception>

#include "enhanced/lighting/bake_alignment.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/tests/probe_bake_fixtures.h"

using namespace openrac::lighting;

namespace {

// Creates a grid crossing the centre plane of a thin solid wall.
ProbeGrid wall_grid() {
    ProbeGrid grid;
    grid.origin = {-2, -2, -2};
    grid.spacing = {1, 1, 1};
    grid.dimensions = {5, 5, 5};
    grid.map_resolution = 2;
    return grid;
}

// A shared offset improves capped clearance and removes inside placements without increasing storage.
void test_alignment_improves_thin_wall() {
    BakeScene source;
    source.triangles = bake_test::box({-.05f, -10, -10}, {.05f, 10, 10});
    const bake_detail::Scene scene(source);
    BakeSettings settings;
    auto volume = ProbeVolume::allocate(wall_grid());
    const auto before = volume.grid;
    const auto result = bake_detail::align_probe_grid(scene, settings, volume);
    CHECK(result.after.mean > result.before.mean);
    CHECK(result.before.inside > 0 && result.after.inside == 0);
    CHECK(result.after.near <= result.before.near);
    CHECK(volume.grid.origin == before.origin);
    CHECK(volume.grid.spacing == before.spacing);
    CHECK(volume.grid.dimensions == before.dimensions);
    CHECK(volume.probes.size() == before.probe_count());

    // Every outer face coordinate remains fixed; all offsets stay inside the existing relocation budget.
    for (std::size_t index = 0; index < volume.probes.size(); ++index) {
        std::size_t coordinate = index;
        const auto& offset = volume.probes[index].offset;
        CHECK(bake_detail::dot(offset, offset) <= .45f * .45f);
        bool boundaries_fixed = true;

        for (unsigned axis = 0; axis < offset.size(); ++axis) {
            const auto cell = coordinate % before.dimensions[axis];
            coordinate /= before.dimensions[axis];

            boundaries_fixed &=
                (cell != 0 && cell + 1 != before.dimensions[axis]) || offset[axis] == 0;
        }

        CHECK(boundaries_fixed);
    }

    auto repeated = ProbeVolume::allocate(before);
    const auto again = bake_detail::align_probe_grid(scene, settings, repeated);
    CHECK(result.offset == again.offset);
    CHECK(result.after.mean == again.after.mean);
}

// Compares every scoring decision and downstream relocation input without a tolerance.
void check_alignment(
    const bake_detail::GridAlignment& expected,
    const bake_detail::GridAlignment& actual,
    const ProbeVolume& serial,
    const ProbeVolume& parallel,
    int rounding
) {
    CHECK(std::fegetround() == rounding);
    CHECK(actual.offset == expected.offset);
    CHECK(actual.before.mean == expected.before.mean);
    CHECK(actual.after.mean == expected.after.mean);
    CHECK(actual.before.inside == expected.before.inside);
    CHECK(actual.after.inside == expected.after.inside);
    CHECK(actual.before.near == expected.before.near);
    CHECK(actual.after.near == expected.after.near);

    // Probe positions are the downstream relocation and transport inputs.
    for (std::size_t index = 0; index < serial.probes.size(); ++index) {
        CHECK(parallel.probes[index].offset == serial.probes[index].offset);
    }
}

// Parallel ray queries retain serial phase scores, anchored offsets and directed-rounding results.
void test_parallel_alignment_preserves_scores_and_offsets() {
    BakeScene source;
    source.triangles = bake_test::box({-.05f, -10, -10}, {.05f, 10, 10});
    const bake_detail::Scene scene(source);
    BakeSettings settings;
    BakeThreadPool pool(4);
    const int rounding = std::fegetround();

    // Reused workers must inherit each new caller environment, not a preceding job's rounding mode.
    for (const int mode : {FE_TONEAREST, FE_TOWARDZERO, FE_DOWNWARD, FE_TONEAREST}) {
        CHECK(std::fesetround(mode) == 0);
        auto serial = ProbeVolume::allocate(wall_grid());
        const auto expected = bake_detail::align_probe_grid(scene, settings, serial);

        // Different task budgets must not change the score reduction or phase tie-breaking order.
        for (const unsigned threads : {1u, 2u, 4u}) {
            auto parallel = ProbeVolume::allocate(wall_grid());
            const auto actual =
                bake_detail::align_probe_grid(scene, settings, parallel, {threads, &pool});
            check_alignment(expected, actual, serial, parallel, mode);
        }
    }

    CHECK(std::fesetround(rounding) == 0);
}

// An open sheet still discourages surface-coincident probes without inventing a solid interior.
void test_alignment_open_sheet_and_winding() {
    BakeScene source;
    source.triangles = bake_test::box({0, -10, -10}, {0, 10, 10});
    // Two opposite box faces coincide; retain one rectangular sheet only.
    source.triangles = {source.triangles[6], source.triangles[7]};
    BakeSettings settings;
    auto volume = ProbeVolume::allocate(wall_grid());
    const bake_detail::Scene scene(source);
    const auto result = bake_detail::align_probe_grid(scene, settings, volume);
    CHECK(result.after.mean > result.before.mean);
    CHECK(result.before.near > result.after.near);
    CHECK(result.before.inside == 0 && result.after.inside == 0);
    BakeScene reversed = source;

    for (auto& triangle : reversed.triangles) {
        std::swap(triangle.vertices[0], triangle.vertices[1]);
    }

    const bake_detail::Scene reversed_scene(reversed);
    auto other = ProbeVolume::allocate(wall_grid());
    CHECK(bake_detail::align_probe_grid(reversed_scene, settings, other).offset == result.offset);
}

// Empty space keeps the original phase, and disabling relocation also disables alignment displacement.
void test_alignment_no_unnecessary_movement() {
    BakeScene empty;
    const bake_detail::Scene acceleration(empty);
    auto volume = ProbeVolume::allocate(wall_grid());
    BakeSettings settings;
    CHECK(bake_detail::align_probe_grid(acceleration, settings, volume).offset == Float3{});
    BakeScene source;
    source.triangles = bake_test::box({-.05f, -10, -10}, {.05f, 10, 10});
    const bake_detail::Scene wall(source);
    settings.relocation_max_fraction = 0;
    CHECK(bake_detail::align_probe_grid(wall, settings, volume).offset == Float3{});
}

// Alignment cannot lose a placement recoverable from its nominal location in the same geometry.
void test_alignment_preserves_recoverable_probes() {
    BakeScene source;
    source.triangles = bake_test::box({-.05f, -10, -10}, {.05f, 10, 10});
    const auto obstruction = bake_test::box({-1.1f, -.45f, -.45f}, {.39f, .45f, .45f});
    source.triangles.insert(source.triangles.end(), obstruction.begin(), obstruction.end());
    BakeSettings settings;
    settings.rays_per_probe = 16;
    settings.bounces = 0;
    const auto aligned = bake_probes(wall_grid(), source, settings, {}, 1);
    auto nominal = ProbeVolume::allocate(wall_grid());
    ProbeBaker baker(source, settings);
    std::size_t recoverable = 0;

    // Two-plane grids have no interior coordinates to align, providing the original relocation path.
    for (std::size_t index = 0; index < aligned.probes.size(); ++index) {
        auto local = wall_grid();
        local.origin = nominal.position(index);
        local.dimensions = {2, 2, 2};
        const auto baseline = baker.bake(local, {}, 1);
        recoverable += baseline.probes[0].valid;
        CHECK(!baseline.probes[0].valid || aligned.probes[index].valid);
    }

    CHECK(recoverable > 0);
}

// Shared alignment moves probes away from an open sheet without marking individual relocation.
void test_alignment_is_not_individual_relocation() {
    BakeScene source;
    const auto box = bake_test::box({0, -10, -10}, {0, 10, 10});
    source.triangles = {box[6], box[7]};
    BakeSettings settings;
    settings.rays_per_probe = 16;
    settings.bounces = 0;
    const auto volume = bake_probes(wall_grid(), source, settings, {}, 1);
    std::size_t aligned = 0;

    for (const auto& probe : volume.probes) {
        aligned += probe.offset != Float3{};
        CHECK(!probe.relocated);
    }

    CHECK(aligned > 0);
}

}  // namespace

int main() {
    // Unexpected traversal failures must report a failed fixture instead of escaping the process boundary.
    try {
        test_alignment_improves_thin_wall();
        test_parallel_alignment_preserves_scores_and_offsets();
        test_alignment_open_sheet_and_winding();
        test_alignment_no_unnecessary_movement();
        test_alignment_preserves_recoverable_probes();
        test_alignment_is_not_individual_relocation();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected probe alignment test failure\n");
        return 1;
    }
}
