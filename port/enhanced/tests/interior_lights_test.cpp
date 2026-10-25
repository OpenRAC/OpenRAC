// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic rooms and finite-source arithmetic verify interior placement and bounded emission.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <numbers>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/lighting/interior_lights.h"
#include "enhanced/tests/probe_bake_fixtures.h"

using namespace openrac::lighting;

namespace {
// Builds an empty room with no ambient, sunlight or original emitters.
BakeScene room() {
    BakeScene scene;
    scene.environment = {};
    scene.triangles = bake_test::box({-10, -10, 0}, {10, 10, 8});
    return scene;
}

// A white Lambertian floor needs pi times the desired reflected radiance as irradiance.
void test_ceiling_target_uses_irradiance_units() {
    auto scene = room();
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {.1f, .05f, .025f}}};
    InteriorLightSettings settings;
    const auto report = prepare_interior_lighting(scene, seeds, settings);
    CHECK(report && report->lights.size() == 1);

    // Report failure without dereferencing an absent generated source.
    if (!report || report->lights.empty()) {
        return;
    }

    const auto& light = report->lights[0];
    const float height = light.position[2] - report->receivers[0][2];
    const float area = 4 * settings.half_extent * settings.half_extent;
    for (unsigned c = 0; c < 3; ++c) {
        const float reflected =
            light.radiance[c] * area / (height * height * std::numbers::pi_v<float>);
        CHECK(std::abs(reflected - seeds[0].colour[c]) < 1e-6f);
        CHECK(
            std::abs(report->targets[0][c] - seeds[0].colour[c] * std::numbers::pi_v<float>) < 1e-6f
        );
    }
}

// A ceiling source is finite, downward facing and bounded without changing its RGB ratios.
void test_ceiling_source_bounds() {
    auto scene = room();
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, .5f, .25f}, 0}};
    InteriorLightSettings settings;
    settings.ceiling_limit = .1f;
    const auto report = prepare_interior_lighting(scene, seeds, settings);
    CHECK(report.has_value());
    CHECK(report->lights.size() == 1);
    CHECK(report->capped == 1);
    CHECK(scene.triangles.size() == bake_test::box({-10, -10, 0}, {10, 10, 8}).size());
    const auto& light = scene.area_emitters.at(0);
    CHECK(light.position[2] < 8 && light.position[2] > 0);
    CHECK(light.radiance == (Float3{.1f, .05f, .025f}));
    CHECK(bake_detail::cross(light.axis_u, light.axis_v)[2] < 0);
}

// Sloping ceilings orient sources geometrically, independent of authored light direction and winding.
void test_ceiling_orientation_follows_geometry() {
    auto scene = room();
    constexpr float kSlope = .25f;  // Roof rises one unit for four horizontal units.

    for (auto& triangle : scene.triangles) {
        for (auto& vertex : triangle.vertices) {
            vertex[2] += kSlope * vertex[0];
        }
    }

    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report.has_value());
    CHECK(report->lights.size() == 1);
    const auto& light = report->lights.at(0);
    const auto normal = bake_detail::unit(bake_detail::cross(light.axis_u, light.axis_v));
    const auto expected = bake_detail::unit({kSlope, 0, -1});
    CHECK(bake_detail::dot(normal, expected) > .9999f);
    scene.area_emitters.clear();

    // Reversed winding must still emit into the room.
    for (auto& triangle : scene.triangles) {
        std::swap(triangle.vertices[0], triangle.vertices[1]);
    }

    const auto reversed = prepare_interior_lighting(scene, seeds);
    CHECK(reversed.has_value());
    CHECK(reversed->lights.size() == 1);
    CHECK(reversed->lights.at(0).axis_u == light.axis_u);
    CHECK(reversed->lights.at(0).axis_v == light.axis_v);
}

// A folded ceiling contributes both neighbouring face normals rather than selecting one triangle.
void test_ceiling_orientation_averages_nearby_faces() {
    auto scene = room();
    const auto left = bake_test::box({-10, -10, 0}, {0, 10, 8});
    const auto right = bake_test::box({0, -10, 0}, {10, 10, 8});
    // Split the two roof triangles at X=0 so each half has its own slope.
    scene.triangles[2] = left[2];
    scene.triangles[3] = left[3];
    scene.triangles.push_back(right[2]);
    scene.triangles.push_back(right[3]);

    for (auto& triangle : scene.triangles) {
        for (auto& vertex : triangle.vertices) {
            // Only roof vertices form the fold; walls keep their original floor positions.
            if (vertex[2] == 8) {
                vertex[2] += .5f * std::max(0.f, vertex[0]);
            }
        }
    }

    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report.has_value());
    CHECK(report->lights.size() == 1);
    const auto& light = report->lights.at(0);
    const auto normal = bake_detail::unit(bake_detail::cross(light.axis_u, light.axis_v));
    // The mean lies between the flat face (X=0) and the slope normal (X approximately 0.447).
    CHECK(normal[0] > .05f && normal[0] < .4f);
    CHECK(normal[2] < -.9f);
}

// Missing ceilings produce no sources, even when a caller supplies an interior label.
void test_outdoor_rejection() {
    BakeScene scene;
    scene.environment = {};
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report.has_value());
    CHECK(report->lights.empty());
    CHECK(report->rejected == seeds.size());
}

// A small roof above low surrounding walls is an outdoor shelter, not an enclosed room.
void test_low_walls_under_overhang_rejected() {
    auto scene = room();
    scene.triangles = bake_test::box({-10, -10, 0}, {10, 10, 2});
    // Remove the low box's roof so only its surrounding walls provide horizontal occlusion.
    scene.triangles.erase(scene.triangles.begin() + 2, scene.triangles.begin() + 4);
    const auto canopy = bake_test::box({-3, -3, 8}, {3, 3, 9});
    scene.triangles.insert(scene.triangles.end(), canopy.begin(), canopy.end());
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report && report->lights.empty());
}

// An open side exposes the upper hemisphere even when five horizontal directions hit walls.
void test_open_sided_shelter_rejected() {
    auto scene = room();
    // Removing the whole south wall leaves a roofed outdoor bay with three full-height walls.
    scene.triangles.erase(scene.triangles.begin() + 4, scene.triangles.begin() + 6);
    const std::array seeds{InteriorLightSeed{{0, -5, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report && report->lights.empty());
}

// A small doorway remains compatible with an otherwise enclosed room and two-sided geometry.
void test_room_with_doorway_accepted() {
    auto scene = room();
    scene.triangles.erase(scene.triangles.begin() + 4, scene.triangles.begin() + 6);

    // Side posts and lintel leave a four-unit-wide, three-unit-high opening in the south wall.
    for (const auto& bounds : std::array{
             std::array<Float3, 2>{{{-10, -10, 0}, {-2, -9.9f, 8}}},
             std::array<Float3, 2>{{{2, -10, 0}, {10, -9.9f, 8}}},
             std::array<Float3, 2>{{{-2, -10, 3}, {2, -9.9f, 8}}}
         }) {
        const auto wall = bake_test::box(bounds[0], bounds[1]);
        scene.triangles.insert(scene.triangles.end(), wall.begin(), wall.end());
    }

    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {1, 1, 1}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report && report->lights.size() == 1);
}

// Existing point sources are boosted by one scalar and stay below their intensity limit.
void test_existing_emitter_boost() {
    auto scene = room();
    scene.point_emitters.push_back({{0, 0, 4}, {3, 1.5f, .75f}});
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {.2f, .2f, .2f}}};
    InteriorLightSettings settings;
    settings.point_limit = 4;
    const auto report = prepare_interior_lighting(scene, seeds, settings);
    CHECK(report.has_value());
    CHECK(report->boosted_points == 1);
    CHECK(scene.point_emitters[0].intensity == (Float3{4, 2, 1}));
}

// Explicit bake intensity survives automatic interior boosts even below the configured cap.
void test_fixed_emission_retains_radiance() {
    auto scene = room();
    auto& surface = scene.triangles.front();
    surface.emission = {1, 1, 1};
    surface.fixed_emission = true;
    const std::array seeds{InteriorLightSeed{{0, 0, 0}, {.2f, .2f, .2f}}};
    const auto report = prepare_interior_lighting(scene, seeds);
    CHECK(report.has_value());
    CHECK(report->boosted_surfaces == 0);
    CHECK((surface.emission == Float3{1, 1, 1}));
    surface.fixed_emission = false;
    const auto automatic = prepare_interior_lighting(scene, seeds);
    CHECK(automatic.has_value());
    CHECK(automatic->boosted_surfaces == 1);
    CHECK((surface.emission == Float3{2, 2, 2}));
}

// Source counts remain bounded and invalid settings do not modify the scene.
void test_source_budgets_and_invalid_input() {
    auto scene = room();
    const std::array seeds{
        InteriorLightSeed{{-5, 0, 0}, {1, 1, 1}}, InteriorLightSeed{{5, 0, 0}, {1, 1, 1}}
    };
    InteriorLightSettings settings;
    settings.max_per_region = 1;
    const auto report = prepare_interior_lighting(scene, seeds, settings);
    CHECK(report.has_value());
    CHECK(report->lights.size() == 1);
    CHECK(report->budget_skipped == 1);
    settings.emitter_boost = -1;
    CHECK(!prepare_interior_lighting(scene, seeds, settings));
    CHECK(scene.area_emitters.size() == 1);
}

// A 128-source region budget is accepted, while values above that supported limit are rejected.
void test_expanded_region_budget() {
    auto scene = room();
    const std::array seeds{
        InteriorLightSeed{{-5, 0, 0}, {1, 1, 1}}, InteriorLightSeed{{5, 0, 0}, {1, 1, 1}}
    };
    InteriorLightSettings settings;
    settings.max_per_region = 128;
    CHECK(settings.max_lights == 1024);
    CHECK(settings.separation == 6);
    const auto report = prepare_interior_lighting(scene, seeds, settings);
    CHECK(report && report->lights.size() == seeds.size());
    // Out-of-range configuration must fail without adding sources.
    settings.max_per_region = 129;
    CHECK(!prepare_interior_lighting(scene, seeds, settings));
    CHECK(scene.area_emitters.size() == seeds.size());
    // The level budget is independent of the smaller connected-region budget.
    settings.max_per_region = 128;
    settings.max_lights = 1025;
    CHECK(!prepare_interior_lighting(scene, seeds, settings));
    CHECK(scene.area_emitters.size() == seeds.size());
}

// A small rectangle approaches I/r^2, emits on one side and is blocked by ordinary geometry.
void test_rectangle_transport() {
    BakeScene scene;
    scene.environment = {};
    scene.area_emitters.push_back({{0, 0, 5}, {.1f, 0, 0}, {0, -.1f, 0}, {2, 1, .5f}});
    bake_detail::Random random{1};
    {
        const bake_detail::Scene acceleration(scene);
        const auto light = bake_detail::sample_emitter(acceleration, {0, 0, 0}, .002f, random);
        constexpr float kArea = 4 * .1f * .1f;
        constexpr float kExpected = 2 * kArea / (5 * 5);
        CHECK(std::abs(light.radiance_weight[0] - kExpected) < kExpected * .01f);
        const auto back = bake_detail::sample_emitter(acceleration, {0, 0, 10}, .002f, random);
        CHECK(back.radiance_weight == Float3{});
    }
    scene.triangles = bake_test::box({-10, -10, 2}, {10, 10, 3});
    const bake_detail::Scene acceleration(scene);
    const auto blocked = bake_detail::sample_emitter(acceleration, {0, 0, 0}, .002f, random);
    CHECK(blocked.radiance_weight == Float3{});
}

// A probe sees finite-area direct irradiance with zero surface bounces, approaching the small-source analytic limit.
void test_area_light_direct_probe_irradiance() {
    ProbeGrid grid;
    grid.map_resolution = 4;
    BakeSettings settings;
    settings.rays_per_probe = 512;
    settings.bounces = 0;
    BakeScene scene;
    scene.environment = {};
    const auto dark = bake_probes(grid, scene, settings);
    scene.area_emitters.push_back({{0, 0, 2}, {.05f, 0, 0}, {0, -.05f, 0}, {2, 1, .5f}});
    const auto direct = bake_probes(grid, scene, settings);
    constexpr float kArea = 4 * .05f * .05f;
    constexpr float kSquaredDistance = 2 * 2;
    constexpr unsigned kX = 1;
    constexpr unsigned kY = 1;
    const unsigned side = grid.map_resolution + 2;
    const unsigned at = (kY + 1) * side + kX + 1;
    const float cosine = bake_test::texel_up_cosine(kX, kY, grid.map_resolution);

    // A small distant patch approaches emitted radiance times area and receiving cosine over squared distance.
    for (unsigned channel = 0; channel < 3; ++channel) {
        const float expected =
            scene.area_emitters[0].radiance[channel] * kArea * cosine / kSquaredDistance;
        CHECK(expected > 0);
        CHECK(std::abs(direct.irradiance[at][channel] / expected - 1) < .03f);
    }

    CHECK(direct.distance_moments == dark.distance_moments);
    settings.bounces = 3;
    const auto deeper = bake_probes(grid, scene, settings);
    CHECK(deeper.irradiance == direct.irradiance);
}

// Direct area-light energy is blocked by real geometry even when no reflected lighting is requested.
void test_area_light_direct_occlusion() {
    ProbeGrid grid;
    grid.map_resolution = 4;
    BakeSettings settings;
    settings.rays_per_probe = 128;
    settings.bounces = 0;
    BakeScene scene;
    scene.environment = {};
    scene.area_emitters.push_back({{0, 0, 2}, {.05f, 0, 0}, {0, -.05f, 0}, {2, 1, .5f}});
    const auto visible = bake_probes(grid, scene, settings);
    scene.triangles = bake_test::box({-10, -10, .5f}, {10, 10, 1});
    const auto blocked = bake_probes(grid, scene, settings);
    bool received = false;

    // Every receiving direction must lose direct energy when the entire source is behind the blocker.
    for (std::size_t n = 0; n < visible.irradiance.size(); ++n) {
        received |= visible.irradiance[n][0] > 0;
        CHECK(blocked.irradiance[n] == Float3{});
    }

    CHECK(received);
}
}  // namespace

int main() {
    // Unexpected traversal failures must fail the test instead of escaping the program boundary.
    try {
        test_ceiling_source_bounds();
        test_ceiling_target_uses_irradiance_units();
        test_ceiling_orientation_follows_geometry();
        test_ceiling_orientation_averages_nearby_faces();
        test_outdoor_rejection();
        test_low_walls_under_overhang_rejected();
        test_open_sided_shelter_rejected();
        test_room_with_doorway_accepted();
        test_existing_emitter_boost();
        test_fixed_emission_retains_radiance();
        test_source_budgets_and_invalid_input();
        test_expanded_region_budget();
        test_rectangle_transport();
        test_area_light_direct_probe_irradiance();
        test_area_light_direct_occlusion();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected interior light test failure\n");
        return 1;
    }
}
