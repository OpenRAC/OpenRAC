// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
void quad(BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle triangle;
    triangle.vertices = {a, b, c};
    scene.triangles.push_back(triangle);
    triangle.vertices = {a, c, d};
    scene.triangles.push_back(triangle);
}

BakeScene ground() {
    BakeScene scene;
    quad(scene, {-20, -10, 0}, {20, -10, 0}, {20, 10, 0}, {-20, 10, 0});
    return scene;
}

struct Fixture {
    std::vector<EnvironmentFloorSample> samples;
    std::vector<EnvironmentFloor> floors;
    EnvironmentPlacementReport report;

    Fixture() {
        for (float x : {-4.f, -2.f, 2.f, 4.f}) {
            EnvironmentFloorSample sample;
            sample.position = {x, 0, 0};
            sample.region = x < 0 ? 10u : 20u;
            sample.lighting_labels = std::vector<unsigned>{0};
            samples.push_back(sample);
            EnvironmentFloor floor;
            floor.position = sample.position;
            floor.capture = {x, 0, 3};
            floor.clearance = 8;
            floor.light = {.5f, .4f, .3f};
            floors.push_back(floor);
        }

        samples[0].neighbours = {1};
        samples[1].neighbours = {0};
        samples[2].neighbours = {3};
        samples[3].neighbours = {2};
    }

    void connect(const BakeScene& source) {
        Scene scene(source);
        connect_environment_floor_fragments(scene, samples, floors, {}, {}, report);
    }

    bool combined() const { return samples.front().region == samples.back().region; }
};

void compatible_fragments() {
    auto source = ground();

    for (bool reverse : {false, true}) {
        if (reverse) {
            for (auto& triangle : source.triangles) {
                std::swap(triangle.vertices[0], triangle.vertices[2]);
            }
        }

        Fixture fixture;
        fixture.connect(source);
        CHECK(fixture.combined());
        CHECK(fixture.report.merged_floor_components == 1);
        CHECK(fixture.report.bridged_floor_edges >= 1);
        Scene scene(source);
        CHECK(room_layout(scene, fixture.samples, {}, false).rooms.size() == 1);
        const auto connected = connected_room_floors(scene, Fixture{}.samples, {});
        CHECK(connected.size() > fixture.samples.size());
        const auto medial = room_layout(scene, connected, {});
        CHECK(medial.rooms.size() == 1);
        const auto field = room_distance_field(scene, connected, medial, {});
        CHECK(field.contains({0, 0, 0}, 0));
    }
}

// A long inference gap can connect only when every intermediate sample has floor and player clearance.
void test_long_supported_gap() {
    Fixture fixture;
    fixture.samples = {fixture.samples.front(), fixture.samples.back()};
    fixture.samples[0].position = {-5, 0, 0};
    fixture.samples[1].position = {5, 0, 0};

    for (auto& sample : fixture.samples) {
        sample.neighbours.clear();
    }

    auto source = ground();
    Scene scene(source);
    const auto connected = connected_room_floors(scene, fixture.samples, {});
    CHECK(connected.front().region == connected[1].region);
    CHECK(connected.size() > fixture.samples.size());
    const auto layout = room_layout(scene, connected, {});
    CHECK(layout.rooms.size() == 1);
    CHECK(room_distance_field(scene, connected, layout, {}).contains({0, 0, 0}, 0));
    quad(source, {0, -10, 0}, {0, 10, 0}, {0, 10, 4}, {0, -10, 4});
    Scene blocked(source);
    const auto separate = connected_room_floors(blocked, fixture.samples, {});
    CHECK(separate.front().region != separate[1].region);
}

// Floor following retains a shallow valley that lies below the straight chord between its ends.
void test_supported_floor_dip() {
    auto source = ground();
    quad(source, {-6, -2, 1.5f}, {-4, -2, 1.5f}, {-4, 2, 1.5f}, {-6, 2, 1.5f});
    quad(source, {-4, -2, 1.5f}, {-2, -2, 0}, {-2, 2, 0}, {-4, 2, 1.5f});
    quad(source, {2, -2, 0}, {4, -2, 1.5f}, {4, 2, 1.5f}, {2, 2, 0});
    quad(source, {4, -2, 1.5f}, {6, -2, 1.5f}, {6, 2, 1.5f}, {4, 2, 1.5f});
    Scene scene(source);
    const Float3 a{-5, 0, 1.5f}, b{5, 0, 1.5f};
    CHECK(!room_path_clear(scene, a, b, {}));
    const auto path = room_floor_path(scene, a, b, {});
    CHECK(!path.empty());
    CHECK(std::any_of(path.begin(), path.end(), [](auto p) { return std::abs(p[2]) < .01f; }));
    // A genuine missing floor is still a barrier, even when there is support at both ends.
    source.triangles.erase(source.triangles.begin(), source.triangles.begin() + 2);
    Scene gap(source);
    CHECK(room_floor_path(gap, a, b, {}).empty());
}

void retain_lighting_boundaries() {
    for (unsigned variant = 0; variant < 4; ++variant) {
        Fixture fixture;

        for (unsigned i : {2u, 3u}) {
            if (variant == 0) {
                fixture.samples[i].lighting_labels = std::vector<unsigned>{1};
            } else if (variant == 1) {
                fixture.samples[i].lighting_labels.reset();
            } else if (variant == 2) {
                fixture.floors[i].light = {0, 0, 8};
            } else {
                fixture.floors[i].light = {8, 8, 8};
            }
        }

        fixture.connect(ground());
        CHECK(!fixture.combined());
    }
}

void retain_geometry_boundaries() {
    for (unsigned variant = 0; variant < 4; ++variant) {
        auto source = ground();
        Fixture fixture;

        if (variant == 0) {
            quad(source, {0, -10, 0}, {0, 10, 0}, {0, 10, 6}, {0, -10, 6});
        } else if (variant == 1) {
            quad(source, {0, -10, 0}, {0, -.5f, 0}, {0, -.5f, 6}, {0, -10, 6});
            quad(source, {0, .5f, 0}, {0, 10, 0}, {0, 10, 6}, {0, .5f, 6});
        } else if (variant == 2) {
            source.triangles.clear();
            quad(source, {-20, -10, 0}, {-1, -10, 0}, {-1, 10, 0}, {-20, 10, 0});
            quad(source, {1, -10, 0}, {20, -10, 0}, {20, 10, 0}, {1, 10, 0});
        } else {
            quad(source, {-20, -10, 4}, {20, -10, 4}, {20, 10, 4}, {-20, 10, 4});

            for (unsigned i : {2u, 3u}) {
                fixture.samples[i].position[2] = fixture.floors[i].position[2] = 4;
                fixture.floors[i].capture[2] = 7;
            }
        }

        fixture.connect(source);
        CHECK(!fixture.combined());
    }
}

void unlabelled_patch_cannot_bridge_authored_moods() {
    Fixture fixture;
    fixture.samples.resize(3);
    fixture.floors.resize(3);

    for (unsigned i = 0; i < 3; ++i) {
        const Float3 p{float(i) * 4 - 4, 0, 0};
        fixture.samples[i].position = fixture.floors[i].position = p;
        fixture.samples[i].region = i;
        fixture.samples[i].neighbours.clear();
        fixture.samples[i].lighting_labels =
            i == 1 ? std::vector<unsigned>{} : std::vector<unsigned>{i / 2};
    }

    fixture.connect(ground());
    CHECK(fixture.samples[0].region == fixture.samples[1].region);
    CHECK(fixture.samples[0].region != fixture.samples[2].region);
}
}  // namespace

int main() {
    compatible_fragments();
    test_long_supported_gap();
    test_supported_floor_dip();
    retain_lighting_boundaries();
    retain_geometry_boundaries();
    unlabelled_patch_cannot_bridge_authored_moods();
    return openrac::test::result();
}
