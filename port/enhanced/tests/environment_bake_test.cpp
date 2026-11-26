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
#include "enhanced/lighting/spatial_placement.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
void test_influence_ceiling_coverage() {
    BakeScene scene;
    BakeTriangle roof;
    roof.vertices = {Float3{-20, -20, 18}, Float3{20, -20, 18}, Float3{0, 20, 18}};
    scene.triangles.push_back(roof);
    EnvironmentFloorSample floor;
    EnvironmentInfluence original;
    original.capture = 7;
    original.frame.pitch = {std::cos(.1f), std::sin(.1f)};
    original.minimum = {-2, -2, -1};
    original.maximum = {2, 2, 4};
    original.projection = EnvironmentProjection{};
    const auto contains = [](const EnvironmentInfluence& v, Float3 p, float margin = 0) {
        p = v.frame.to_local(p);
        for (unsigned k = 0; k < 3; ++k) {
            if (p[k] < v.minimum[k] + margin || p[k] > v.maximum[k] - margin) {
                return false;
            }
        }
        return true;
    };
    auto region = original;
    extend_environment_influence_coverage(scene, std::span(&floor, 1), {}, std::span(&region, 1));
    CHECK(contains(region, {0, 0, 18}));
    CHECK(region.capture == 7 && region.frame.pitch == original.frame.pitch);
    CHECK(region.projection->maximum == original.projection->maximum);
    scene.triangles.clear();
    region = original;
    extend_environment_influence_coverage(scene, std::span(&floor, 1), {}, std::span(&region, 1));
    CHECK(contains(region, {0, 0, 30}));
    floor.underwater = true;
    region = original;
    extend_environment_influence_coverage(scene, std::span(&floor, 1), {}, std::span(&region, 1));
    CHECK(region.minimum == original.minimum && region.maximum == original.maximum);
    floor.underwater = false;
    auto upper = floor;
    upper.position = {0, 0, 12};
    std::array floors{floor, upper};
    region = original;
    extend_environment_influence_coverage(scene, floors, {}, std::span(&region, 1));
    CHECK(!contains(region, {0, 0, 13.25f}, .25f));
}

// Collision decks remain playable receivers even when the rendered deck is absent.
void test_collision_supported_capture_centres() {
    using namespace bake_detail;
    ProbeGrid grid;
    grid.origin = {-16, -16, -16};
    grid.spacing = {16, 16, 16};
    grid.dimensions = {3, 3, 3};
    auto volume = ProbeVolume::allocate(grid);
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }
    for (auto& moments : volume.distance_moments) {
        moments = {100, 10000};
    }
    for (unsigned variant = 0; variant < 4; ++variant) {
        BakeScene render, collision;
        const auto quad = [](BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle first, second;
            first.vertices = {a, b, c};
            second.vertices = {a, c, d};
            scene.triangles.push_back(first);
            scene.triangles.push_back(second);
        };
        quad(render, {-5, -3, 0}, {5, -3, 0}, {5, 3, 0}, {-5, 3, 0});
        if (variant == 1) {
            quad(render, {-5, -3, 4.7f}, {5, -3, 4.7f}, {5, 3, 4.7f}, {-5, 3, 4.7f});
        }
        if (variant == 3) {
            quad(collision, {-5, -3, 2}, {-.5f, -3, 2}, {-.5f, 3, 2}, {-5, 3, 2});
            quad(collision, {.5f, -3, 2}, {5, -3, 2}, {5, 3, 2}, {.5f, 3, 2});
        } else {
            quad(collision, {-5, -3, 2}, {5, -3, 2}, {5, 3, 2}, {-5, 3, 2});
        }
        if (variant == 2) {
            quad(collision, {-5, -3, 4.54f}, {5, -3, 4.54f}, {5, 3, 4.54f}, {-5, 3, 4.54f});
        }
        std::vector<EnvironmentFloorSample> samples(3);
        for (unsigned a = 0; a < 3; ++a) {
            auto& floor = samples[a];
            floor.position = {float(a) * 2 - 2, 0, 2};
            floor.anchor = true;
            floor.island = 0;
            floor.island_clearance = a == 0 ? 8 : a == 1 ? 4 : 2;
            floor.lighting_labels = std::vector<unsigned>{0};
            if (a) {
                floor.neighbours.push_back(a - 1);
            }
            if (a + 1 < 3) {
                floor.neighbours.push_back(a + 1);
            }
        }
        Scene visible(render), physical(collision);
        EnvironmentPlacementReport report;
        const auto map = environment_floor_map(
            visible, std::span(&volume, 1), samples, {}, {}, report, &physical
        );
        CHECK(!map.floors.empty());
        for (const auto& floor : map.floors) {
            CHECK(std::abs(floor.position[2] - 2) < .001f);
        }
        if (variant == 2) {
            CHECK(std::none_of(map.floors.begin(), map.floors.end(), [](const auto& floor) {
                return floor.capture_candidate;
            }));
            const auto low = place_environment_probes(
                visible, std::span(&volume, 1), samples, {}, {}, &report, &physical
            );
            CHECK(low.size() == 1 && report.uncovered.empty());
            CHECK(low.front().position[2] < 4.54f && low.front().position[2] > 4.4f);
            continue;
        }
        const auto captures = place_environment_probes(
            visible, std::span(&volume, 1), samples, {}, {}, &report, &physical
        );
        CHECK(!captures.empty() && report.uncovered.empty());
        if (variant < 2) {
            CHECK(captures.size() == 1);
            CHECK(std::abs(captures.front().position[0]) < .01f);
            CHECK(std::abs(captures.front().position[2] - (variant == 1 ? 4.6f : 5.f)) < .01f);
        } else {
            CHECK(report.unsupported_floors > 0);
            for (auto capture : captures) {
                CHECK(std::abs(capture.position[0]) > .5f);
            }
        }
    }
}

// Partial coverage must not pull a room's capture toward its far edge. When the
// neighbouring capture becomes redundant, the surviving room centre stays put.
void test_partially_covered_island_centre() {
    using namespace bake_detail;
    BakeScene geometry;
    BakeTriangle triangle;
    triangle.vertices = {Float3{-20, -20, 0}, Float3{40, -20, 0}, Float3{0, 40, 0}};
    geometry.triangles.push_back(triangle);
    Scene scene(geometry);
    ProbeGrid grid;
    grid.origin = {-16, -16, -16};
    grid.spacing = {32, 32, 32};
    grid.dimensions = {2, 2, 2};
    auto volume = ProbeVolume::allocate(grid);
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }
    std::vector<EnvironmentFloorSample> samples(16);
    for (unsigned i = 0; i < samples.size(); ++i) {
        auto& sample = samples[i];
        sample.position = {float(i) - 4, 0, 0};
        sample.anchor = true;
        sample.island = i < 7 ? 0 : 1;
        sample.island_clearance = 3;
        sample.lighting_labels = std::vector<unsigned>{0};
        if (i) {
            sample.neighbours.push_back(i - 1);
        }
        if (i + 1 < samples.size()) {
            sample.neighbours.push_back(i + 1);
        }
    }
    EnvironmentPlacementReport report;
    const auto captures =
        place_environment_probes(scene, std::span(&volume, 1), samples, {}, {}, &report);
    CHECK(captures.size() == 1 && report.uncovered.empty());
    CHECK(report.owners.front() == 1);
    CHECK(std::abs(captures.front().position[0] - 7) < .01f);
}

void test_local_receiver_assignment() {
    using namespace bake_detail;
    std::vector<EnvironmentFloor> floors(5);
    const std::array<Float3, 5> positions{{{0, 0, 0}, {2, 0, 0}, {2, 8, 0}, {0, 8, 0}, {0, 1, 0}}};
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = positions[i];
        floors[i].capture = positions[i];
        floors[i].capture[2] += 3;
        floors[i].component = 0;
        floors[i].lighting_labels = std::vector<unsigned>{0};
        if (i) {
            floors[i].neighbours.push_back(i - 1);
        }
        if (i + 1 < floors.size()) {
            floors[i].neighbours.push_back(i + 1);
        }
    }
    // The last floor is one unit away through a wall, but nineteen along the floor graph.
    EnvironmentNeighbourhoods blocked(floors, 6);
    CHECK(!blocked.contains(0, 4, true));
    const std::array<unsigned, 2> origins{0, 4};
    std::array<EnvironmentProbe, 2> probes{
        EnvironmentProbe{floors[0].capture, {-20, -20, -1}, {20, 20, 5}},
        EnvironmentProbe{floors[4].capture, {-20, -20, -1}, {20, 20, 5}}
    };
    const auto around_wall = assign_environment_receivers(floors, probes, origins, blocked);
    CHECK(std::find(around_wall[0].begin(), around_wall[0].end(), 12) == around_wall[0].end());
    CHECK(std::find(around_wall[1].begin(), around_wall[1].end(), 12) != around_wall[1].end());
    BakeScene wall_geometry;
    BakeTriangle wall;
    wall.vertices = {Float3{-1, .5f, 0}, Float3{1, .5f, 0}, Float3{1, .5f, 4}};
    wall_geometry.triangles.push_back(wall);
    wall.vertices = {Float3{-1, .5f, 0}, Float3{1, .5f, 4}, Float3{-1, .5f, 4}};
    wall_geometry.triangles.push_back(wall);
    Scene wall_scene(wall_geometry);
    BakeSettings settings;
    EnvironmentNeighbourhoods solid_wall(floors, 6, &wall_scene, &settings);
    CHECK(!solid_wall.contains(0, 4, true));
    BakeScene empty_geometry;
    Scene visible_air(empty_geometry);
    EnvironmentNeighbourhoods clear(floors, 6, &visible_air, &settings);
    CHECK(clear.contains(0, 4));  // A clear visible route avoids the sparse grid detour.
    floors[2].component = 1;
    floors[2].lighting_labels = std::vector<unsigned>{1};
    EnvironmentNeighbourhoods lighting_cut(floors, 24);
    CHECK(!lighting_cut.contains(0, 2, true));
    EnvironmentNeighbourhoods visible_lighting_cut(floors, 24, &visible_air, &settings);
    CHECK(!visible_lighting_cut.contains(0, 2, true));
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = {float(i) * 4, 0, 0};
        floors[i].capture = {float(i) * 4, 0, 3};
        floors[i].component = 0;
        floors[i].lighting_labels = std::vector<unsigned>{0};
    }
    probes[1].position = floors[4].capture;
    EnvironmentNeighbourhoods open(floors, 24);
    const auto assigned = assign_environment_receivers(floors, probes, origins, open);
    // Both captures blend at the midpoint, but the broad box cannot claim the far endpoint.
    CHECK(std::find(assigned[0].begin(), assigned[0].end(), 6) != assigned[0].end());
    CHECK(std::find(assigned[1].begin(), assigned[1].end(), 6) != assigned[1].end());
    CHECK(std::find(assigned[0].begin(), assigned[0].end(), 12) == assigned[0].end());
    CHECK(std::find(assigned[1].begin(), assigned[1].end(), 0) == assigned[1].end());

    // A probe with unique receivers is redundant when its neighbour can grow
    // safely. Reassignment must consider receivers outside the old box.
    std::vector<EnvironmentProbe> compact{
        EnvironmentProbe{floors[0].capture, {-1, -1, -1}, {9, 1, 4}},
        EnvironmentProbe{floors[4].capture, {7, -1, -1}, {17, 1, 4}}
    };
    std::vector<unsigned> compact_origins{0, 4};
    auto compact_assignments = assign_environment_receivers(floors, compact, compact_origins, open);
    EnvironmentNeighbourhoods too_far(floors, 6);
    CHECK(!remove_environment_capture(
        visible_air, floors, settings, {}, 1, compact, compact_origins, compact_assignments, too_far
    ));
    CHECK(compact.size() == 2);
    CHECK(remove_environment_capture(
        visible_air, floors, settings, {}, 1, compact, compact_origins, compact_assignments, open
    ));
    CHECK(compact.size() == 1);
    CHECK(compact_assignments.front().size() == floors.size() * 3);
    for (const auto& floor : floors) {
        for (unsigned h = 0; h < 3; ++h) {
            CHECK(
                environment_probe_coverage(compact.front(), environment_receiver_position(floor, h))
                >= kEnvironmentUsefulCoverage
            );
        }
    }
}

void test_constrained_reflection_fit() {
    using namespace bake_detail;
    BakeScene geometry;
    Scene scene(geometry);
    std::vector<EnvironmentFloor> floors;
    for (auto p :
         {Float3{0, 0, 0},
          Float3{8, 0, 0},
          Float3{8, 2, 0},
          Float3{2, 2, 0},
          Float3{2, 8, 0},
          Float3{0, 8, 0}}) {
        EnvironmentFloor floor;
        floor.position = p;
        floor.capture = p;
        floor.capture[2] += 3;
        floor.component = 0;
        floor.lighting_labels = std::vector<unsigned>{0};
        floors.push_back(floor);
    }
    EnvironmentFloor foreign;
    foreign.position = {7, 7, 0};
    foreign.component = 1;
    foreign.lighting_labels = std::vector<unsigned>{1};
    floors.push_back(foreign);
    EnvironmentProbe original;
    original.position = floors.front().capture;
    original.frame.axis = {std::cos(.5235987756f), std::sin(.5235987756f)};
    original.minimum = {INFINITY, INFINITY, INFINITY};
    original.maximum = {-INFINITY, -INFINITY, -INFINITY};
    for (unsigned f = 0; f + 1 < floors.size(); ++f) {
        for (unsigned h = 0; h < 3; ++h) {
            const auto p = original.frame.to_local(environment_receiver_position(floors[f], h));
            for (unsigned a = 0; a < 3; ++a) {
                original.minimum[a] = std::min(original.minimum[a], p[a] - .75f);
                original.maximum[a] = std::max(original.maximum[a], p[a] + .75f);
            }
        }
    }
    original.projection_minimum = original.minimum;
    original.projection_maximum = original.maximum;
    const auto volume = [](const EnvironmentProbe& p) {
        return (p.maximum[0] - p.minimum[0]) * (p.maximum[1] - p.minimum[1])
               * (p.maximum[2] - p.minimum[2]);
    };
    std::vector<EnvironmentProbe> probes{original};
    const unsigned origin = 0;
    fit_environment_capture_boxes(scene, floors, {}, {}, probes, std::span(&origin, 1));
    CHECK(volume(probes.front()) < volume(original) * .98f);
    for (unsigned h = 0; h < 3; ++h) {
        CHECK(
            environment_probe_coverage(probes.front(), environment_receiver_position(foreign, h))
            < kEnvironmentUsefulCoverage
        );
        for (unsigned f = 0; f + 1 < floors.size(); ++f) {
            const auto p = environment_receiver_position(floors[f], h);
            CHECK(
                environment_probe_coverage(probes.front(), p) + .0001f
                >= environment_probe_coverage(original, p)
            );
        }
    }
}

void test_shared_capture_plan() {
    using namespace bake_detail;
    BakeScene geometry;
    Scene scene(geometry);
    BakeSettings settings;
    std::vector<EnvironmentFloor> floors(4);
    for (unsigned i = 0; i < 4; ++i) {
        floors[i].position = {float(i < 2 ? i * 2 : 10 + (i - 2) * 2), 0, 0};
        floors[i].capture = floors[i].position;
        floors[i].capture[2] += 3;
        floors[i].component = 0;
        floors[i].lighting_labels = std::vector<unsigned>{0};
    }
    const std::vector<EnvironmentProbe> regions{
        EnvironmentProbe{floors[0].capture, {-1, -1, -1}, {3, 1, 4}},
        EnvironmentProbe{floors[3].capture, {9, -1, -1}, {13, 1, 4}}
    };
    const std::array<unsigned, 2> origins{0, 3};
    EnvironmentPlacementReport report;
    report.owners = {0, 1};
    std::vector<EnvironmentInfluence> influences;
    EnvironmentNeighbourhoods open(floors, 24, &scene, &settings);
    const auto captures = select_environment_captures(
        scene, settings, {}, floors, regions, origins, open, report, influences
    );
    CHECK(captures.size() == 1 && influences.size() == 2);
    CHECK(influences[0].capture == 0 && influences[1].capture == 0);
    CHECK(influences[1].minimum == regions[1].minimum);
    CHECK(influences[1].minimum[0] > captures.front().position[0]);
    CHECK(influences[1].projection.has_value());
    CHECK(influences[1].projection->minimum[0] <= regions[1].minimum[0] - .5f);
    CHECK(influences[1].projection->maximum[0] >= regions[1].maximum[0] + .5f);
    CHECK(report.influence_owners == std::vector<unsigned>({0, 1}));
    // A connected, visible slope can share a photograph even when its floor
    // rises more than the old near-replacement helper's two-unit limit.
    auto ramp_floors = floors;
    for (unsigned i = 2; i < 4; ++i) {
        ramp_floors[i].position[2] = 4;
        ramp_floors[i].capture[2] = 7;
    }
    auto ramp_regions = regions;
    ramp_regions[1].position = ramp_floors[3].capture;
    ramp_regions[1].minimum[2] = ramp_regions[1].projection_minimum[2] = 3;
    ramp_regions[1].maximum[2] = ramp_regions[1].projection_maximum[2] = 8;
    EnvironmentNeighbourhoods ramp(ramp_floors, 24, &scene, &settings);
    report.owners = {0, 1};
    const auto sloped = select_environment_captures(
        scene, settings, {}, ramp_floors, ramp_regions, origins, ramp, report, influences
    );
    CHECK(sloped.size() == 1 && influences.size() == 2);
    BakeTriangle wall;
    wall.vertices = {Float3{6, -10, 0}, Float3{6, 10, 0}, Float3{6, 10, 8}};
    geometry.triangles.push_back(wall);
    wall.vertices = {Float3{6, -10, 0}, Float3{6, 10, 8}, Float3{6, -10, 8}};
    geometry.triangles.push_back(wall);
    Scene blocked(geometry);
    EnvironmentNeighbourhoods wall_distances(floors, 24, &blocked, &settings);
    report.owners = {0, 1};
    const auto divided = select_environment_captures(
        blocked, settings, {}, floors, regions, origins, wall_distances, report, influences
    );
    CHECK(divided.size() == 2 && influences[0].capture != influences[1].capture);
    for (unsigned i = 2; i < 4; ++i) {
        floors[i].component = 1;
        floors[i].lighting_labels = std::vector<unsigned>{1};
    }
    EnvironmentNeighbourhoods separate(floors, 24, &scene, &settings);
    report.owners = {0, 1};
    const auto isolated = select_environment_captures(
        scene, settings, {}, floors, regions, origins, separate, report, influences
    );
    CHECK(isolated.size() == 2 && influences[0].capture != influences[1].capture);
}

void test_enclosed_space_capture_quality() {
    using namespace bake_detail;
    BakeScene geometry;
    Scene scene(geometry);
    BakeSettings settings;
    std::vector<EnvironmentFloor> floors(31);
    for (unsigned i = 0; i < floors.size(); ++i) {
        auto& floor = floors[i];
        floor.position = {float(i * 2), 0, 0};
        floor.capture = {float(i * 2), 0, 3};
        floor.component = 0;
        floor.room = i < 13 ? 0 : i <= 17 ? 1 : 2;
        floor.island = floor.room;
        floor.lighting_labels = std::vector<unsigned>{0};
        floor.area = 16;
        floor.clearance = floor.room == 1 ? 2.f : 8.f;
    }
    const std::vector<EnvironmentProbe> regions{
        EnvironmentProbe{floors[10].capture, {-1, -1, -1}, {41, 1, 4}},
        EnvironmentProbe{floors[20].capture, {19, -1, -1}, {61, 1, 4}}
    };
    const std::array<unsigned, 2> origins{10, 20};
    const auto run = [&] {
        EnvironmentNeighbourhoods neighbourhoods(floors, 24, &scene, &settings);
        EnvironmentPlacementReport report;
        report.owners = {0, 2};
        std::vector<EnvironmentInfluence> influences;
        return select_environment_captures(
            scene, settings, {}, floors, regions, origins, neighbourhoods, report, influences
        );
    };
    const auto enclosed = run();
    CHECK(std::any_of(enclosed.begin(), enclosed.end(), [](const auto& p) {
        return std::abs(p.position[0] - 30) < .01f;
    }));
    // The same footprint in open space can borrow the two existing captures.
    // A watershed label alone must not force a new photograph.
    for (auto& floor : floors) {
        floor.clearance = 8;
    }
    const auto open = run();
    CHECK(open.size() == 2);
    CHECK(std::none_of(open.begin(), open.end(), [](const auto& p) {
        return std::abs(p.position[0] - 30) < .01f;
    }));
}

void test_sliver_reassignment_margin() {
    using namespace bake_detail;
    BakeScene geometry;
    Scene scene(geometry);
    BakeSettings settings;
    std::vector<EnvironmentFloor> floors(3);
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = {i == 0 ? 0.f : i == 1 ? 4.f : 4.5f, 0, 0};
        floors[i].capture = floors[i].position;
        floors[i].capture[2] += 3;
        floors[i].component = i == 2 ? 1 : 0;
        floors[i].lighting_labels = std::vector<unsigned>{i == 2 ? 1u : 0u};
    }
    std::vector<EnvironmentProbe> probes{
        EnvironmentProbe{floors[0].capture, {-1, -1, -1}, {1, 1, 4}},
        EnvironmentProbe{floors[1].capture, {3.75f, -1, -1}, {4.25f, 1, 4}},
        EnvironmentProbe{floors[2].capture, {4.25f, -1, -1}, {4.75f, 1, 4}}
    };
    const auto old = probes;
    std::vector<unsigned> origins{0, 1, 2};
    EnvironmentNeighbourhoods neighbourhoods(floors, 24, &scene, &settings);
    auto assignments = assign_environment_receivers(floors, probes, origins, neighbourhoods);
    // The sliver needs only its existing .25-unit margin. Requiring a fresh .75
    // would inevitably cover the foreign receiver half a unit away.
    CHECK(remove_environment_capture(
        scene, floors, settings, {}, 1, probes, origins, assignments, neighbourhoods
    ));
    CHECK(probes.size() == 2);
    for (unsigned h = 0; h < 3; ++h) {
        for (unsigned f = 0; f < 2; ++f) {
            const auto p = environment_receiver_position(floors[f], h);
            CHECK(
                environment_probe_coverage(probes.front(), p) + .0001f
                >= environment_probe_coverage(old[f], p)
            );
        }
        CHECK(
            environment_probe_coverage(probes.front(), environment_receiver_position(floors[2], h))
            < kEnvironmentUsefulCoverage
        );
    }
}

void test_reassignment_alternative() {
    using namespace bake_detail;
    BakeScene geometry;
    Scene scene(geometry);
    BakeSettings settings;
    std::vector<EnvironmentFloor> floors(5);
    const std::array<Float3, 5> positions{{{0, 0, 0}, {0, 4, 0}, {4, 0, 0}, {8, 0, 0}, {2, 2, 0}}};
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = positions[i];
        floors[i].capture = positions[i];
        floors[i].capture[2] += 3;
        floors[i].component = i == 4 ? 1 : 0;
        floors[i].lighting_labels = std::vector<unsigned>{i == 4 ? 1u : 0u};
    }
    std::vector<EnvironmentProbe> probes{
        EnvironmentProbe{floors[0].capture, {-1, -1, -1}, {1, 5, 4}},
        EnvironmentProbe{floors[2].capture, {3, -1, -1}, {5, 1, 4}},
        EnvironmentProbe{floors[3].capture, {7, -1, -1}, {9, 1, 4}},
        EnvironmentProbe{floors[4].capture, {1, 1, -1}, {3, 3, 4}}
    };
    std::vector<unsigned> origins{0, 2, 3, 4};
    EnvironmentNeighbourhoods neighbourhoods(floors, 24, &scene, &settings);
    auto assignments = assign_environment_receivers(floors, probes, origins, neighbourhoods);
    // The nearest capture's triangular domain would enclose a foreign receiver.
    // Transferring to the other neighbour is safe and should remove the camera.
    CHECK(remove_environment_capture(
        scene, floors, settings, {}, 1, probes, origins, assignments, neighbourhoods
    ));
    CHECK(probes.size() == 3);
    CHECK(
        environment_probe_coverage(probes[1], environment_receiver_position(floors[2], 0))
        >= kEnvironmentUsefulCoverage
    );
    CHECK(
        environment_probe_coverage(probes[1], environment_receiver_position(floors[4], 0))
        < kEnvironmentUsefulCoverage
    );
    // The incompatible pocket cannot be transferred to any remaining capture.
    CHECK(!remove_environment_capture(
        scene, floors, settings, {}, 2, probes, origins, assignments, neighbourhoods
    ));
    CHECK(probes.size() == 3 && origins.back() == 4);
}

void test_sloping_reflection_box() {
    using namespace bake_detail;
    const ProbeFrame frame{{.8f, .6f}, {.96f, .28f}};
    BakeScene geometry;
    const auto a = frame.to_world({-5, -5, 0}), b = frame.to_world({45, -5, 0});
    const auto c = frame.to_world({45, 5, 0}), d = frame.to_world({-5, 5, 0});
    BakeTriangle triangle;
    triangle.vertices = {a, b, c};
    geometry.triangles.push_back(triangle);
    triangle.vertices = {a, c, d};
    geometry.triangles.push_back(triangle);
    Scene scene(geometry);
    ProbeGrid grid;
    grid.origin = {-16, -16, -16};
    grid.spacing = {32, 32, 32};
    grid.dimensions = {3, 3, 3};
    auto volume = ProbeVolume::allocate(grid);
    std::vector<EnvironmentFloorSample> samples(20);
    for (unsigned i = 0; i < samples.size(); ++i) {
        auto& floor = samples[i];
        floor.position = frame.to_world({float(i) * 2, 0, 0});
        floor.anchor = true;
        floor.island = 0;
        floor.island_clearance = 3;
        if (i) {
            floor.neighbours.push_back(i - 1);
        }
        if (i + 1 < samples.size()) {
            floor.neighbours.push_back(i + 1);
        }
    }
    EnvironmentPlacementReport report;
    const auto captures =
        place_environment_probes(scene, std::span(&volume, 1), samples, {}, {}, &report);
    CHECK(captures.size() == 1 && report.uncovered.empty());
    CHECK(std::abs(captures.front().frame.axis[1]) > .3f);
    CHECK(std::abs(captures.front().frame.pitch[1]) > .15f);
}

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
    // A real adaptive sample at the old edge midpoint is already authoritative. Do not
    // duplicate that receiver with inherited ownership from one coarse endpoint.
    samples.resize(3);
    samples[2].position = {0, 0, 0};
    samples[2].anchor = true;
    samples[2].lighting_labels = std::vector<unsigned>{0};
    samples[0].neighbours = {1, 2};
    samples[1].neighbours = {0, 2};
    samples[2].neighbours = {0, 1};
    (void)place_environment_probes(acceleration, std::span(&volume, 1), samples, {}, {}, &report);
    CHECK(report.playable_floors == 5);  // Three originals and the two remaining half-edges.
    CHECK(report.ownership_conflicts == 0 && report.uncovered.empty());
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

void test_flat_fit_obstruction() {
    BakeScene geometry;
    bake_detail::Scene scene(geometry);
    std::array<bake_detail::EnvironmentFloor, 5> floors;
    floors[0].position = {-4, -4, 0};
    floors[1].position = {4, -4, 0};
    floors[2].position = {4, 4, 0};
    floors[3].position = {-4, 4, 0};
    for (unsigned i = 0; i < 4; ++i) {
        floors[i].room = 0;
    }
    floors[4].position = {0, 0, 0};
    floors[4].room = 1;
    floors[0].capture = {-4, -4, 3};
    EnvironmentProbe box{floors[0].capture, {-5, -5, -1}, {5, 5, 4}};
    std::vector<EnvironmentProbe> probes{box};
    const std::array<unsigned, 1> origins{0};
    const bake_detail::EnvironmentAssignments required{{1, 4, 7, 10}};
    CHECK(!bake_detail::fit_environment_capture_boxes(
               scene, floors, {}, {}, probes, origins, &required
    )
               .empty());
    // A point over the patch is not inside its required horizontal plane.
    floors[4].position[2] = 6;
    probes = {box};
    CHECK(
        bake_detail::fit_environment_capture_boxes(
            scene, floors, {}, {}, probes, origins, &required
        )
            .empty()
    );
    // Nor can an unrelated point outside the footprint reject a valid fit.
    floors[4].position = {12, 0, 0};
    probes = {box};
    CHECK(
        bake_detail::fit_environment_capture_boxes(
            scene, floors, {}, {}, probes, origins, &required
        )
            .empty()
    );
}

void test_open_neighbourhood_visibility() {
    BakeScene geometry;
    BakeTriangle wall;
    wall.vertices = {Float3{20, 5, 0}, Float3{20, 25, 0}, Float3{20, 25, 6}};
    geometry.triangles.push_back(wall);
    wall.vertices = {Float3{20, 5, 0}, Float3{20, 25, 6}, Float3{20, 5, 6}};
    geometry.triangles.push_back(wall);
    bake_detail::Scene scene(geometry);
    std::array<bake_detail::EnvironmentFloor, 3> floors;
    floors[0].position = {0, 0, 0};
    floors[1].position = {30, 0, 0};
    floors[2].position = {40, 20, 0};
    for (auto& f : floors) {
        f.room = 0;
    }
    floors[0].locality_radius = 64;
    floors[0].neighbours = {1};
    floors[1].neighbours = {0, 2};
    floors[2].neighbours = {1};
    BakeSettings settings;
    bake_detail::EnvironmentNeighbourhoods local(floors, 24, &scene, &settings);
    CHECK(local.contains(0, 1));   // Farther than nominal spacing, with a clear view.
    CHECK(!local.contains(0, 2));  // The connected path bends behind the wall.
}

int main() {
    test_open_neighbourhood_visibility();
    test_flat_fit_obstruction();
    test_influence_ceiling_coverage();
    test_local_receiver_assignment();
    test_reassignment_alternative();
    test_sliver_reassignment_margin();
    test_shared_capture_plan();
    test_enclosed_space_capture_quality();
    test_constrained_reflection_fit();
    test_collision_supported_capture_centres();
    test_partially_covered_island_centre();
    test_sloping_reflection_box();
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
    // Disconnected watershed islands retain their captures even within the spacing radius.
    CHECK(first.probes.size() == 2);

    const auto spatial = bake_detail::build_spatial_placement(
        scene, seeds, {}, volume.grid.spacing, 4096, 1024, options
    );
    CHECK(first.probes.size() == spatial.field.basins.size());
    CHECK(first.influences.size() == spatial.reflection.volumes.size());
    for (unsigned i = 0; i < first.influences.size(); ++i) {
        const auto& region = first.influences[i];
        const auto& fitted = spatial.reflection.volumes[i];
        CHECK(region.capture == fitted.basin);
        CHECK(region.frame.axis == fitted.frame.axis);
        CHECK(region.frame.pitch == fitted.frame.pitch);
        const auto minimum = fitted.frame.to_local(fitted.origin);
        for (unsigned k = 0; k < 3; ++k) {
            CHECK(std::abs(region.minimum[k] - minimum[k]) < 1e-5f);
            CHECK(std::abs(region.maximum[k] - minimum[k] - fitted.extent[k]) < 1e-5f);
        }
    }
    EnvironmentPlacementReport production_report;
    const auto shared = bake_environment(
        scene, {}, std::span(&volume, 1), seeds, options, {1}, &production_report, nullptr, &spatial
    );
    CHECK(production_report.layered_regions && production_report.uncovered.empty());
    CHECK(production_report.surface_coverage.added_regions == 0);
    CHECK(production_report.watershed.size() == spatial.field.cells.size());
    CHECK(encode_environment(first) == encode_environment(shared));
    // A visible-mesh seam can obstruct every sampled camera while collision
    // still permits a bounded subcell relocation; it must not create more regions.
    auto seam_scene = scene;
    BakeTriangle seam;
    seam.vertices = {Float3{.1f, -1, 0}, Float3{.1f, 1, 0}, Float3{.1f, 1, 20}};
    seam_scene.triangles.push_back(seam);
    seam.vertices = {Float3{.1f, -1, 0}, Float3{.1f, 1, 20}, Float3{.1f, -1, 20}};
    seam_scene.triangles.push_back(seam);
    EnvironmentPlacementReport seam_report;
    std::vector<EnvironmentInfluence> seam_regions;
    const auto moved = bake_detail::place_spatial_environment_probes(
        seam_scene,
        scene,
        std::span(&volume, 1),
        seeds,
        {},
        options,
        spatial,
        seam_report,
        seam_regions
    );
    CHECK(moved.size() == first.probes.size() && seam_regions.size() == first.influences.size());
    CHECK(std::abs(moved[0].position[0]) == .5f);
    CHECK(seam_report.uncovered.empty());

    for (unsigned basin = 0; basin < first.probes.size(); ++basin) {
        bool on_cell = false;
        for (const auto& cell : spatial.field.cells) {
            on_cell |= cell.basin == basin && cell.position == first.probes[basin].position;
        }
        CHECK(on_cell);
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
    // A covered space retains nominal locality even when its box can reach much farther.
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
        auto covered_scene = scene;
        BakeTriangle ceiling;
        ceiling.vertices = {Float3{-20, -20, 8}, Float3{20, -20, 8}, Float3{20, 20, 8}};
        covered_scene.triangles.push_back(ceiling);
        ceiling.vertices = {Float3{-20, -20, 8}, Float3{20, 20, 8}, Float3{-20, 20, 8}};
        covered_scene.triangles.push_back(ceiling);
        bake_detail::Scene acceleration(covered_scene);
        EnvironmentPlacementReport report;
        const auto pruned = bake_detail::place_environment_probes(
            acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(pruned.size() == 5);
        CHECK(report.unpruned_probes == 5 && report.pruned_probes == 0);
        CHECK(report.validation_samples == 15 && report.minimum_coverage >= .2f);
        CHECK(report.coverage_hash == environment_capture_hash(pruned));
        CHECK(report.receivers.size() == 15);

        for (const auto& receiver : report.receivers) {
            CHECK(receiver.probes.size() == 1);
            if (!receiver.probes.empty()) {
                const auto capture = pruned[receiver.probes.front()].position;
                CHECK(
                    std::hypot(capture[0] - receiver.position[0], capture[1] - receiver.position[1])
                    <= prune_options.spacing + .001f
                );
            }
        }

        CHECK(report.uncovered.empty() && report.ownership_conflicts == 0);
        const auto serial =
            bake_environment(covered_scene, {}, std::span(&wide, 1), hall, prune_options, {1});
        const auto threaded =
            bake_environment(covered_scene, {}, std::span(&wide, 1), hall, prune_options, {2});
        CHECK(encode_environment(serial) == encode_environment(threaded));
        // Removing the roof permits a shared capture over unobstructed open space.
        bake_detail::Scene open_acceleration(scene);
        const auto open = bake_detail::place_environment_probes(
            open_acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(open.size() < pruned.size());
        CHECK(report.uncovered.empty() && report.locality_conflicts == 0);
        const std::array<EnvironmentFloorSample, 2> compact_patch{
            {{{-1, 0, 0}, 0, 1, true, {1}}, {{1, 0, 0}, 0, 1, false, {0}}}
        };
        const auto compact_open = bake_detail::place_environment_probes(
            open_acceleration, std::span(&wide, 1), compact_patch, {}, prune_options, &report
        );
        CHECK(compact_open.size() == 1 && compact_open[0].position == Float3({0, 0, 3}));
        CHECK(
            std::all_of(
                report.floor_locality_radii.begin(),
                report.floor_locality_radii.end(),
                [&](float r) { return r == prune_options.spacing; }
            )
        );
        prune_options.box_distance = 6;
        const auto limited = bake_detail::place_environment_probes(
            acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(limited.size() == 5);
        CHECK(report.uncovered.empty() && report.minimum_coverage >= .2f);
        prune_options.spacing = 24;
        prune_options.box_distance = 64;
        const auto compact = bake_detail::place_environment_probes(
            acceleration, std::span(&wide, 1), hall, {}, prune_options, &report
        );
        CHECK(compact.size() == 1 && compact.front().position == Float3({0, 0, 3}));
        CHECK(report.uncovered.empty() && report.locality_conflicts == 0);
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
    // Nearby islands keep stable independent ownership while preserving receiver coverage.
    const std::array<EnvironmentFloorSample, 2> adjacent{
        {{{-.5f, 0, 0}, 0, 0, true, {}}, {{.5f, 0, 0}, 1, 1, true, {}}}
    };
    {
        bake_detail::Scene acceleration(scene);
        EnvironmentPlacementReport report;
        const auto placed = bake_detail::place_environment_probes(
            acceleration, std::span(&volume, 1), adjacent, {}, options, &report
        );
        CHECK(placed.size() == 2 && report.uncovered.empty() && report.ownership_conflicts == 0);
        CHECK(report.owners.size() == 2 && report.owners[0] != report.owners[1]);
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
    separate[0].lighting_labels = std::vector<unsigned>{0};
    separate[1].lighting_labels = std::vector<unsigned>{1};
    placed = bake_detail::place_environment_probes(
        acceleration, std::span(&volume, 1), separate, {}, options, &report
    );
    CHECK(placed.size() == 2 && report.lighting_spaces == 1 && report.uncovered.empty());
    CHECK(report.ownership_conflicts == 0);
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
