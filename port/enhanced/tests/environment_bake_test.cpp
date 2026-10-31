// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures consume the completed GI field and visible emission, with deterministic parallel output.
#include <cfenv>
#include <cmath>
#include <numbers>

#include "enhanced/lighting/environment_bake.h"
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_placement.h"
#include "enhanced/lighting/environment_placement_internal.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
// A supported low passage needs reflection influence even when a capture cannot fit above its floor.
void test_low_passage_receives_reflections(const BakeScene& source, const ProbeVolume& volume) {
    using namespace bake_detail;
    auto scene = source;
    BakeTriangle roof;
    // The player fits beneath this roof, but a three-unit capture plus clearance does not.
    roof.vertices = {Float3{.5f, -.5f, 2.54f}, Float3{1.5f, -.5f, 2.54f}, Float3{1.5f, .5f, 2.54f}};
    scene.triangles.push_back(roof);
    roof.vertices = {Float3{.5f, -.5f, 2.54f}, Float3{1.5f, .5f, 2.54f}, Float3{.5f, .5f, 2.54f}};
    scene.triangles.push_back(roof);
    std::vector<EnvironmentFloorSample> samples(2);

    for (unsigned i = 0; i < samples.size(); ++i) {
        samples[i].position = {i ? 1.f : -1.f, 0, 0};
        samples[i].anchor = true;
        samples[i].neighbours = {1 - i};
        samples[i].lighting_labels = std::vector<unsigned>{0};
    }

    Scene acceleration(scene);
    EnvironmentPlacementReport report;
    const auto placed =
        place_environment_probes(acceleration, std::span(&volume, 1), samples, {}, {}, &report);
    CHECK(placed.size() == 1);
    CHECK(report.receiver_only_floors > 0);
    CHECK(report.uncovered.empty() && report.tolerated_misses.empty());
    CHECK(report.playable_floors == 3);  // Two original floor samples plus their edge midpoint.
    CHECK(placed.front().position[0] < .5f);

    for (float x : {-1.5f, 0.f, 1.5f}) {
        for (float z : {.15f, 1.25f, 2.5f}) {
            CHECK(
                environment_probe_coverage(placed.front(), {x, 0, z}) >= kEnvironmentUsefulCoverage
            );
        }
    }
}

// Substantial adjacent spaces merge only when their representative GI retains the same mood.
void test_same_mood_spaces(const BakeScene& scene) {
    using namespace bake_detail;
    Scene acceleration(scene);
    std::vector<EnvironmentFloorSample> samples(26);
    std::vector<EnvironmentFloor> floors(26);
    std::vector<std::vector<unsigned>> rooms(2);

    for (unsigned i = 0; i < samples.size(); ++i) {
        auto& sample = samples[i];
        sample.lighting_labels = std::vector<unsigned>{0};
        // Thirteen cells per side exceed the tiny-fragment threshold of twelve.
        auto& floor = floors[i];
        floor.source = i;
        floor.room = i / 13;
        floor.position = {float(i) - 13, 0, 0};
        floor.capture = floor.position;
        floor.capture[2] = 3;
        floor.light = {1, 1, 1};
        floor.clearance = 4;
        rooms[floor.room].push_back(i);

        if (i) {
            sample.neighbours.push_back(i - 1);
            samples[i - 1].neighbours.push_back(i);
        }
    }

    const auto original_floors = floors;
    const auto original_rooms = rooms;
    EnvironmentPlacementReport report;
    merge_environment_spaces(acceleration, {}, floors, samples, rooms, report);
    CHECK(rooms.size() == 1);
    floors = original_floors;
    rooms = original_rooms;

    for (auto id : rooms[1]) {
        floors[id].light = {0, 4, 0};
    }

    merge_environment_spaces(acceleration, {}, floors, samples, rooms, report);
    CHECK(rooms.size() == 2);
}

// Rejected capture samples must not split an open room; lighting and geometry still constrain merging.
void fragment_merging(const BakeScene& scene) {
    using namespace bake_detail;
    Scene acceleration(scene);
    auto fixture = [] {
        EnvironmentFloorMap map;
        map.floors.resize(14);
        map.rooms = {{}, {0}};

        for (unsigned i = 0; i < 14; ++i) {
            auto& floor = map.floors[i];
            floor.source = i;
            floor.room = i == 0 ? 1 : 0;
            floor.sky_band = 1;
            floor.position = {float(i), 0, 0};
            floor.capture = {float(i), 0, 3};
            floor.light = {2, 2, 2};
            floor.clearance = 8;

            if (i) {
                map.rooms[0].push_back(i);
            }
        }

        map.floors[1].position = {-1, 0, 0};
        map.floors[2].position = {1, 0, 0};
        return map;
    };
    std::vector<EnvironmentFloorSample> samples(14);

    for (auto& sample : samples) {
        sample.region = 7;
    }

    samples[0].neighbours = {1, 2};
    samples[1].neighbours = {0};
    samples[2].neighbours = {0};
    EnvironmentPlacementReport report;
    auto map = fixture();
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 1 && report.merged_graph_gaps == 0 && report.merged_fragments == 1);
    CHECK(
        map.floors[0].light == Float3({2, 2, 2})
    );  // Classification cleanup never changes the actual GI data.
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 1 && report.merged_fragments == 0 && report.spaces.size() == 1);
    map = fixture();
    map.floors[2].position = {-2, 1, 0};
    map.floors[0].light = {};
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2 && report.merged_graph_gaps == 0);  // One-sided doorway.
    map = fixture();
    map.floors[0].sky_band = 0;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // Indoor/outdoor enclosure boundary.
    map = fixture();
    samples[0].region = 9;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // Authored lighting cut.
    samples[0].region = 7;
    map = fixture();
    report = {};

    for (auto& floor : map.floors) {
        floor.light = {2, 0, 0};
    }

    map.floors[0].light = {0, 8, 0};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // Different lighting hue is not a brightness outlier.
    map = fixture();
    map.floors[0].position[2] = 4;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // Stacked floors cannot share a fragment.
    // A single exposure/brightness outlier is too small to reserve a separate capture.
    map = fixture();
    map.floors[0].sky_band = 0;
    map.floors[0].light = {.01f, .01f, .01f};
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 1);
    CHECK(map.floors[0].light == Float3({.01f, .01f, .01f}));
    map = fixture();
    map.floors[0].sky_band = 1;
    map.floors[0].clearance = .5f;
    map.floors[0].light = {.01f, .01f, .01f};

    for (unsigned i = 1; i < map.floors.size(); ++i) {
        map.floors[i].sky_band = 0;
    }

    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 1);  // An exposed wall sliver is not an enclosed room.

    // The area threshold is independent of floor-sample count and can be disabled.
    for (float area : {4.f, 8.f}) {
        map = fixture();
        map.floors[0].area = area;
        map.floors[0].sky_band = 0;
        report = {};
        merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
        CHECK(map.rooms.size() == 2);
    }

    map = fixture();
    map.floors[0].sky_band = 0;
    map.floors[0].clearance = .5f;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 2);  // A narrow doorway still separates a small room.
    map = fixture();
    map.floors[0].light = {0, 8, 0};

    for (unsigned i = 1; i < map.floors.size(); ++i) {
        map.floors[i].light = {2, 0, 0};
    }

    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 2);
    map = fixture();
    map.floors[0].sky_band = 0;
    samples[0].region = 9;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 2);
    samples[0].region = 7;
    auto blocked = scene;
    BakeTriangle wall;
    wall.vertices = {Float3{.5f, -2, 0}, Float3{.5f, 2, 0}, Float3{.5f, 2, 4}};
    blocked.triangles.push_back(wall);
    wall.vertices = {Float3{.5f, -2, 0}, Float3{.5f, 2, 4}, Float3{.5f, -2, 4}};
    blocked.triangles.push_back(wall);
    Scene obstructed(blocked);
    map = fixture();
    samples[0].neighbours = {2};
    report = {};
    merge_environment_spaces(obstructed, {}, map.floors, samples, map.rooms, report);
    CHECK(
        map.rooms.size() == 2
    );  // Close samples separated by visible geometry remain independent.
    map = fixture();
    map.floors[0].sky_band = 0;
    report = {};
    merge_environment_spaces(obstructed, {}, map.floors, samples, map.rooms, report, 4);
    CHECK(map.rooms.size() == 2);  // Even a tiny enclosed room remains independent.
    // A single unsupported intermediate sample is a sampling gap, not a new room.
    samples.resize(16);
    samples[14].region = samples[15].region = 7;
    samples[0].neighbours = {14};
    samples[14].neighbours = {0, 1, 2};
    samples[1].neighbours = samples[2].neighbours = {14};
    map = fixture();
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 1 && report.merged_graph_gaps == 1);
    map = fixture();
    samples[14].region = 9;
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // A gap cannot bypass an authored cut.
    samples[14].region = 7;
    samples[14].neighbours = {0, 15};
    samples[15].neighbours = {14, 1, 2};
    map = fixture();
    report = {};
    merge_environment_spaces(acceleration, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // Longer missing runs remain separate.
    samples[14].neighbours = {0, 2};
    map = fixture();
    report = {};
    merge_environment_spaces(obstructed, {}, map.floors, samples, map.rooms, report);
    CHECK(map.rooms.size() == 2);  // A short graph gap cannot tunnel through visible geometry.
}
}  // namespace

int main() {
    ProbeGrid grid;
    grid.origin = {-8, -8, 0};
    grid.spacing = {16, 16, 16};
    grid.max_distance = 100;
    auto volume = ProbeVolume::allocate(grid);

    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    for (auto& pixel : volume.irradiance) {
        pixel =
            {2 * std::numbers::pi_v<float>,
             2 * std::numbers::pi_v<float>,
             2 * std::numbers::pi_v<float>};
    }

    for (auto& moment : volume.distance_moments) {
        moment = {100, 10000};
    }

    BakeScene scene;
    scene.environment = {0, 0, 0};
    BakeTriangle floor;
    floor.vertices = {Float3{-20, -20, 0}, Float3{20, -20, 0}, Float3{20, 20, 0}};
    floor.diffuse = {.5f, .25f, .1f};
    floor.emission = {100, 100, 100};
    floor.visible_emission = {.125f, 0, 0};
    scene.triangles.push_back(floor);
    floor.vertices = {Float3{-20, -20, 0}, Float3{20, 20, 0}, Float3{-20, 20, 0}};
    scene.triangles.push_back(floor);
    fragment_merging(scene);
    test_same_mood_spaces(scene);
    test_low_passage_receives_reflections(scene, volume);
    const std::array<EnvironmentFloorSample, 2> seeds{
        {{{0, 0, 0}, 0, 0, true, {}}, {{3, 0, 0}, 1, 0, true, {}}}
    };
    EnvironmentBakeSettings options;
    options.resolution = 8;
    options.maximum_probes = 2;
    options.spacing = 2;
    options.filter_samples = 16;
    const auto first = bake_environment(scene, {}, std::span(&volume, 1), seeds, options, {1});
    const auto parallel = bake_environment(scene, {}, std::span(&volume, 1), seeds, options, {2});
    // Open floor fragments three units apart share one capture under the six-unit spacing rule.
    CHECK(first.probes.size() == 1);

    for (const auto& p : first.probes) {
        CHECK(std::abs(p.position[2] - kEnvironmentCaptureHeight) < 1e-5f);
    }

    CHECK(encode_environment(first) == encode_environment(parallel));
    BakeThreadPool pool(2);
    // Reused GI workers may retain directed rounding; captures must restore the caller's mode.
    CHECK(pool.run(2, [] { std::fesetround(FE_DOWNWARD); }).has_value());
    const auto reused =
        bake_environment(scene, {}, std::span(&volume, 1), seeds, options, {2, &pool});
    CHECK(encode_environment(first) == encode_environment(reused));
    const unsigned side = first.face_resolution() + 2;
    const unsigned centre = first.face_resolution() / 2 + 1;
    const auto value = unpack_environment_colour(
        first.pixels[std::size_t(centre) * first.width() + 5 * side + centre]
    );
    CHECK(std::abs(value[0] - 1.125f) < .04f);
    CHECK(std::abs(value[1] - .5f) < .02f);
    CHECK(std::abs(value[2] - .2f) < .01f);
    // Redundant supplements disappear, but unique reach and each space's central capture survive.
    {
        auto wide_grid = grid;
        wide_grid.origin = {-32, -32, -8};
        wide_grid.spacing = {64, 64, 64};
        auto wide = ProbeVolume::allocate(wide_grid);

        for (auto& probe : wide.probes) {
            probe.valid = true;
        }

        for (auto& moment : wide.distance_moments) {
            moment = {100, 10000};
        }

        const std::array<EnvironmentFloorSample, 3> hall{
            {{{-10, 0, 0}, 0, 1, true, {1}},
             {{0, 0, 0}, 0, 1, false, {0, 2}},
             {{10, 0, 0}, 0, 1, false, {1}}}
        };
        auto prune_options = options;
        prune_options.maximum_probes = 16;
        bake_detail::Scene acceleration(scene);
        EnvironmentPlacementReport report;
        const auto pruned = bake_detail::place_environment_probes(
            acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(pruned.size() == 1 && pruned[0].position == Float3({0, 0, 3}));
        CHECK(report.unpruned_probes == 5 && report.pruned_probes == 4);
        CHECK(report.validation_samples == 15 && report.minimum_coverage >= .2f);
        CHECK(report.coverage_hash == environment_capture_hash(pruned));
        CHECK(report.receivers.size() == 15);

        for (const auto& receiver : report.receivers) {
            CHECK(receiver.probes == std::vector<unsigned>{0});
        }

        CHECK(report.uncovered.empty() && report.ownership_conflicts == 0);
        const auto serial =
            bake_environment(scene, {}, std::span(&wide, 1), hall, prune_options, {1});
        const auto threaded =
            bake_environment(scene, {}, std::span(&wide, 1), hall, prune_options, {2});
        CHECK(encode_environment(serial) == encode_environment(threaded));
        prune_options.box_distance = 6;
        const auto limited = bake_detail::place_environment_probes(
            acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(limited.size() == 2 && report.pruned_probes == 3);
        CHECK(report.uncovered.empty() && report.minimum_coverage >= .2f);
    }
    // A closed room is valid air even when parity alone would interpret it as a solid.
    // Its capture stays centred and floor-relative, independent of triangle winding.
    auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
        BakeTriangle triangle;
        triangle.vertices = {a, b, c};
        scene.triangles.push_back(triangle);
        triangle.vertices = {a, c, d};
        scene.triangles.push_back(triangle);
    };
    quad({-6, -6, 0}, {-6, 6, 0}, {-6, 6, 8}, {-6, -6, 8});
    quad({6, -6, 0}, {6, 6, 0}, {6, 6, 8}, {6, -6, 8});
    quad({-6, -6, 0}, {6, -6, 0}, {6, -6, 8}, {-6, -6, 8});
    quad({-6, 6, 0}, {6, 6, 0}, {6, 6, 8}, {-6, 6, 8});
    quad({-6, -6, 8}, {6, -6, 8}, {6, 6, 8}, {-6, 6, 8});
    std::vector<EnvironmentFloorSample> room;

    for (float y : {-4.f, 0.f, 4.f}) {
        for (float x : {-4.f, 0.f, 4.f}) {
            room.push_back({{x, y, 0}, 0, 0, true, {}});
        }
    }

    for (unsigned a = 0; a < room.size(); ++a) {
        for (unsigned b = 0; b < room.size(); ++b) {
            const auto dx = std::abs(room[a].position[0] - room[b].position[0]);
            const auto dy = std::abs(room[a].position[1] - room[b].position[1]);

            if (dx + dy == 4) {
                room[a].neighbours.push_back(b);
            }
        }
    }

    auto check_room = [&] {
        openrac::lighting::bake_detail::Scene acceleration(scene);
        const auto placed = openrac::lighting::bake_detail::place_environment_probes(
            acceleration, std::span(&volume, 1), room, {}, options
        );
        CHECK(placed.size() == 1);
        CHECK(std::abs(placed[0].position[0]) < .01f);
        CHECK(std::abs(placed[0].position[1]) < .01f);
        CHECK(std::abs(placed[0].position[2] - 3) < .01f);
        CHECK(std::abs(placed[0].projection_maximum[2] - 8) < .01f);
    };
    options.spacing = 24;
    check_room();

    for (auto& triangle : scene.triangles) {
        std::swap(triangle.vertices[0], triangle.vertices[2]);
    }

    check_room();
    // Adjacent open fragments share a capture without cutting off either receiver footprint.
    const std::array<EnvironmentFloorSample, 2> adjacent{
        {{{-.5f, 0, 0}, 0, 0, true, {}}, {{.5f, 0, 0}, 1, 1, true, {}}}
    };
    {
        bake_detail::Scene acceleration(scene);
        EnvironmentPlacementReport report;
        const auto placed = bake_detail::place_environment_probes(
            acceleration, std::span(&volume, 1), adjacent, {}, options, &report
        );
        CHECK(placed.size() == 1 && report.uncovered.empty() && report.ownership_conflicts == 0);
    }
    // An unanchored lower storey remains excluded even when GI covers it and it has valid air.
    const std::array<EnvironmentFloorSample, 2> layers{
        {{{0, 0, 0}, 0, 0, false, {}}, {{0, 0, 8}, 1, 1, true, {}}}
    };
    bake_detail::Scene acceleration(scene);
    EnvironmentPlacementReport report;
    auto placed = bake_detail::place_environment_probes(
        acceleration, std::span(&volume, 1), layers, {}, options, &report
    );
    CHECK(placed.size() == 1 && std::abs(placed[0].position[2] - 11) < .01f);
    CHECK(report.rejected_floors == 1 && report.playable_floors == 1 && report.uncovered.empty());
    CHECK(report.minimum_coverage >= .2f);

    // Authored lighting boundaries retain independent captures across a walkable connection.
    auto separate = layers;
    separate[0] = {{-3, 0, 8}, 0, 0, true, {1}};
    separate[1] = {{3, 0, 8}, 1, 1, false, {0}};
    placed = bake_detail::place_environment_probes(
        acceleration, std::span(&volume, 1), separate, {}, options, &report
    );
    CHECK(placed.size() == 2 && report.lighting_spaces == 2 && report.uncovered.empty());
    options.maximum_probes = 1;
    bool rejected = false;

    try {
        (void)bake_detail::place_environment_probes(
            acceleration, std::span(&volume, 1), separate, {}, options, &report
        );
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    CHECK(rejected && !report.uncovered.empty());  // Limits cannot silently drop a playable room.
    return openrac::test::result();
}
