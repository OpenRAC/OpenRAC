// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Synthetic platforms and a four-unit-deep basin test traversal and reflection-medium boundaries.
#include <algorithm>

#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_placement.h"
#include "enhanced/lighting/environment_placement_internal.h"
#include "enhanced/lighting/room_layout.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
// A valid black GI field isolates placement from transport and visibility variance.
ProbeVolume uniform_gi() {
    ProbeGrid grid;
    grid.origin = {-16, -16, -16};
    grid.spacing = {64, 64, 64};
    auto result = ProbeVolume::allocate(grid);

    for (auto& probe : result.probes) {
        probe.valid = true;
    }

    for (auto& moments : result.distance_moments) {
        moments = {100, 10000};
    }

    return result;
}

// Test geometry keeps authored normals absent; only two-sided triangle geometry participates.
void quad(BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle t;
    t.vertices = {a, b, c};
    scene.triangles.push_back(t);
    t.vertices = {a, c, d};
    scene.triangles.push_back(t);
}

// A supported three-unit rise requires an airborne link; no floor is invented over the gap.
void test_jumpable_platforms() {
    BakeScene source;
    quad(source, {-6, -3, 0}, {-1, -3, 0}, {-1, 3, 0}, {-6, 3, 0});
    quad(source, {1, -3, 3}, {6, -3, 3}, {6, 3, 3}, {1, 3, 3});
    std::vector<EnvironmentFloorSample> floors(2);

    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = i ? Float3{3, 0, 3} : Float3{-3, 0, 0};
        floors[i].region = i;
        floors[i].anchor = true;
        floors[i].lighting_labels = std::vector<unsigned>{0};
    }

    Scene scene(source);
    const auto connected = connected_room_floors(scene, floors, {});
    CHECK(connected.size() == 2 && connected[0].region != connected[1].region);
    const auto layout = room_layout(scene, connected, {});
    CHECK(layout.rooms.size() == 2);
    CHECK(std::any_of(layout.links.begin(), layout.links.end(), [](const auto& link) {
        return link.kind == RoomLinkKind::Jump;
    }));
    BakeSettings limited;
    limited.traversal.jump_rise = 0;
    CHECK(room_layout(scene, connected, limited).links.empty());
    quad(source, {0, -10, 0}, {0, 10, 0}, {0, 10, 10}, {0, -10, 10});
    Scene blocked(source);
    CHECK(room_layout(blocked, connected, {}).links.empty());
}

// Water collision is gameplay support but remains absent from solid transport geometry.
std::vector<GameplayTriangle> basin(BakeScene& source) {
    quad(source, {-4, -2, 0}, {0, -2, 0}, {0, 10, 0}, {-4, 10, 0});
    quad(source, {0, 0, -4}, {8, 0, -4}, {8, 8, -4}, {0, 8, -4});
    std::vector<GameplayTriangle> triangles;

    for (const auto& t : source.triangles) {
        triangles.push_back({t.vertices, false});
    }

    triangles.push_back({{Float3{0, 0, 0}, Float3{8, 0, 0}, Float3{8, 8, 0}}, true});
    triangles.push_back({{Float3{0, 0, 0}, Float3{8, 8, 0}, Float3{0, 8, 0}}, true});
    return triangles;
}

// Above-water and submerged reflection captures remain separate even with identical GI signatures.
void test_swimming_reflection_layers() {
    BakeScene source;
    const auto triangles = basin(source);
    EnvironmentFloorSample bank;
    bank.position = {-2, 4, 0};
    bank.anchor = true;
    bank.lighting_labels = std::vector<unsigned>{0};
    std::vector<EnvironmentFloorSample> floors{bank};
    append_swimming_surfaces(triangles, floors);
    CHECK(
        std::count_if(
            floors.begin(),
            floors.end(),
            [](const auto& floor) { return floor.swimming_surface && !floor.underwater; }
        )
        == 25
    );
    CHECK(
        std::count_if(
            floors.begin(), floors.end(), [](const auto& floor) { return floor.underwater; }
        )
        == 25
    );
    Scene scene(source);
    EnvironmentPlacementReport report;
    auto gi = uniform_gi();
    const auto probes = place_environment_probes(scene, std::span(&gi, 1), floors, {}, {}, &report);
    CHECK(report.uncovered.empty() && report.ownership_conflicts == 0);
    CHECK(std::any_of(probes.begin(), probes.end(), [](const auto& p) {
        return p.position[2] < 0;
    }));
    CHECK(std::any_of(probes.begin(), probes.end(), [](const auto& p) {
        return p.position[2] > 0;
    }));

    for (const auto& p : probes) {
        if (p.position[2] < 0) {
            CHECK(p.maximum[2] == 0);
        } else {
            CHECK(environment_probe_coverage(p, {4, 4, -1}) == 0);
        }
    }

    EnvironmentFloor air, submerged;
    air.room = submerged.room = 0;
    submerged.underwater = true;
    CHECK(!environment_spaces_compatible(air, submerged));
    CHECK(!environment_capture_context_clear(scene, air, {0, 0, 3}, submerged, {1, 0, 3}, {}));
}

// Ordinary decorative geometry cannot become a swimming surface without the authored water tag.
void test_only_authored_water_is_gameplay() {
    BakeScene source;
    auto triangles = basin(source);

    for (auto& t : triangles) {
        t.swimmable = false;
    }

    EnvironmentFloorSample bank;
    bank.position = {-2, 4, 0};
    bank.anchor = true;
    std::vector<EnvironmentFloorSample> floors{bank};
    append_swimming_surfaces(triangles, floors);
    CHECK(floors.size() == 1 && !floors.front().water_height);
}

// Raises the synthetic four-unit basin to a half-unit depth without moving its bank or waterline.
std::vector<GameplayTriangle> shallow_basin(BakeScene& source) {
    auto triangles = basin(source);

    for (auto& triangle : source.triangles) {
        for (auto& vertex : triangle.vertices) {
            if (vertex[2] == -4) {
                vertex[2] = -.5f;
            }
        }
    }

    for (auto& triangle : triangles) {
        for (auto& vertex : triangle.vertices) {
            if (vertex[2] == -4) {
                vertex[2] = -.5f;
            }
        }
    }

    return triangles;
}

// A half-unit-deep pool keeps supported walking receivers and needs no underwater capture layer.
void test_wading_uses_the_basin_floor() {
    BakeScene source;
    const auto triangles = shallow_basin(source);

    EnvironmentFloorSample bank;
    bank.position = {-2, 4, 0};
    bank.anchor = true;
    bank.lighting_labels = std::vector<unsigned>{0};
    std::vector<EnvironmentFloorSample> floors{bank};
    append_swimming_surfaces(triangles, floors, 2, {}, .85f);
    // Five columns by five rows at two-unit spacing use the real bed, not the waterline at zero.
    CHECK(floors.size() == 26);

    for (unsigned i = 1; i < floors.size(); ++i) {
        CHECK(floors[i].position[2] == -.5f);
        CHECK(!floors[i].underwater && !floors[i].swimming_surface && !floors[i].water_height);
    }

    Scene scene(source);
    const auto connected = connected_room_floors(scene, floors, {});
    CHECK(std::all_of(connected.begin(), connected.end(), [&](const auto& floor) {
        return floor.region == connected.front().region;
    }));
    auto gi = uniform_gi();
    EnvironmentPlacementReport report;
    const auto probes = place_environment_probes(scene, std::span(&gi, 1), floors, {}, {}, &report);
    CHECK(report.uncovered.empty() && report.ownership_conflicts == 0);
    CHECK(report.submerged_captures.empty());
    CHECK(probes.size() == 1);

    std::vector<EnvironmentFloorSample> swimming{bank};
    append_swimming_surfaces(triangles, swimming, 2, {}, .5f);
    // At the configured depth boundary the same pool gains surface and submerged domains.
    CHECK(
        std::count_if(
            swimming.begin(), swimming.end(), [](const auto& floor) { return floor.underwater; }
        )
        == 25
    );
}

// An optional coastline mask excludes dry banks without disabling collision-only level support.
void test_water_coastline_and_sealed_room() {
    BakeScene source;
    const auto triangles = basin(source);
    EnvironmentFloorSample bank;
    bank.position = {-2, 4, 0};
    bank.anchor = true;
    EnvironmentFloorSample cellar;
    cellar.position = {4, 4, -8};
    cellar.anchor = true;
    std::vector<EnvironmentFloorSample> floors{bank, cellar};
    append_swimming_surfaces(triangles, floors, 2, [](auto p) { return p[0] <= 4; });
    CHECK(!floors[1].underwater);  // The basin floor at -4 seals this dry room from water at zero.
    CHECK(
        std::count_if(
            floors.begin(),
            floors.end(),
            [](const auto& floor) { return floor.swimming_surface && !floor.underwater; }
        )
        == 15
    );  // Three wet columns, five rows, at two-unit spacing.
}

// A shallow solid bed must not pull a virtual swimming surface down during walking-gap repair.
void test_waterline_is_not_walking_ground() {
    BakeScene source;
    quad(source, {-8, -4, -.25f}, {8, -4, -.25f}, {8, 4, -.25f}, {-8, 4, -.25f});
    std::vector<EnvironmentFloorSample> floors(2);

    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = {i ? 3.f : -3.f, 0, 0};
        floors[i].region = i;
        floors[i].water_height = 0;
        floors[i].swimming_surface = true;
        floors[i].lighting_labels = std::vector<unsigned>{0};
    }

    Scene scene(source);
    const auto connected = connected_room_floors(scene, floors, {});
    CHECK(connected.size() == floors.size());
    CHECK(connected[0].position[2] == 0 && connected[1].position[2] == 0);
    CHECK(connected[0].region != connected[1].region);
    const auto layout = room_layout(scene, connected, {});
    CHECK(std::any_of(layout.links.begin(), layout.links.end(), [](const auto& link) {
        return link.kind == RoomLinkKind::Swim;
    }));
}

// Refined boundary samples interpolate their waterline together with their swimming position.
void test_refined_waterline() {
    BakeScene source;
    quad(source, {-8, -8, -4}, {8, -8, -4}, {8, 8, -4}, {-8, 8, -4});
    std::vector<EnvironmentFloorSample> floors(2);

    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = {float(i) * 4, 0, float(i) * .5f};
        floors[i].anchor = true;
        floors[i].neighbours = {1 - i};
        floors[i].water_height = floors[i].position[2];
        floors[i].swimming_surface = true;
        floors[i].lighting_labels = std::vector<unsigned>{0};
    }

    auto gi = uniform_gi();
    Scene scene(source);
    EnvironmentPlacementReport report;
    const auto map = environment_floor_map(scene, std::span(&gi, 1), floors, {}, {}, report);
    const auto middle = std::find_if(map.floors.begin(), map.floors.end(), [](const auto& floor) {
        return std::abs(floor.position[0] - 2) < .01f;
    });
    CHECK(middle != map.floors.end());

    if (middle != map.floors.end()) {
        CHECK(middle->water_height && std::abs(*middle->water_height - .25f) < .01f);
    }
}
}  // namespace

int main() {
    test_jumpable_platforms();
    test_swimming_reflection_layers();
    test_only_authored_water_is_gameplay();
    test_wading_uses_the_basin_floor();
    test_water_coastline_and_sealed_room();
    test_waterline_is_not_walking_ground();
    test_refined_waterline();
    return openrac::test::result();
}
