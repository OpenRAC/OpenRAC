// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// High overlap is useful only when a replacement preserves meaningful receiver coverage.
#include <limits>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_probe_merge.h"
#include "tests/check.h"
using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
void perceptual_similarity() {
    EnvironmentBakeSettings options;
    auto similar = [&](Float3 a, Float3 b) {
        return similar_environment_capture_light(a, b, options);
    };
    CHECK(similar({.1f, .2f, .3f}, {.2f, .4f, .6f}));
    CHECK(!similar({.1f, .2f, .3f}, {.3f, .6f, .9f}));
    CHECK(similar({.2f, .2f, .2f}, {.2f, .21f, .2f}));
    CHECK(!similar({1, 0, 0}, {0, .29726f, 0}));  // Equal luminance, distinct perceived colour.
    CHECK(!similar({.2f, .2f, .2f}, {.94073f, 0, 0}));  // Equal luminance, different chroma.
    CHECK(similar({}, {.001f, 0, 0}));
    CHECK(!similar({}, {.1f, .1f, .1f}));
    CHECK(!similar({-1, 0, 0}, {0, 0, 0}));
    CHECK(!similar({std::numeric_limits<float>::infinity(), 0, 0}, {0, 0, 0}));
    options.merge_brightness_ratio = 4;
    CHECK(similar({.1f, .2f, .3f}, {.3f, .6f, .9f}));
    options.merge_colour_distance = 0;
    CHECK(!similar({.2f, .2f, .2f}, {.2f, .21f, .2f}));
    CHECK(similar({.2f, .2f, .2f}, {.2f, .2f, .2f}));
}

void loss_policy() {
    std::vector<EnvironmentFloor> floors(1000);
    std::vector<EnvironmentCoverageReceiver> receivers(3000);

    for (unsigned f = 0; f < floors.size(); ++f) {
        for (unsigned h = 0; h < 3; ++h) {
            auto& r = receivers[f * 3 + h];
            r.position = {float(f) * 4, 0, kEnvironmentReceiverHeights[h]};
            r.height = h;
        }
    }

    const EnvironmentBakeSettings options;
    auto allowed = [&](std::vector<unsigned> all,
                       std::vector<unsigned> added,
                       float target = 1000,
                       float total = 3000) {
        return environment_merge_loss_allowed(
            all, added, {receivers, floors, target, total, options}
        );
    };
    CHECK(allowed({}, {}));
    CHECK(allowed({0}, {0}));
    CHECK(allowed({0, 3}, {0, 3}));
    CHECK(!allowed({1}, {1}));              // Body samples are never expendable.
    CHECK(!allowed({2}, {2}));              // Nor are head samples.
    CHECK(!allowed({0, 3, 6}, {0, 3, 6}));  // Per-merge absolute bound.
    CHECK(environment_merge_loss_allowed(
        std::array<unsigned, 1>{0},
        std::array<unsigned, 1>{0},
        {receivers, floors, 100, 3000, options, true}
    ));
    CHECK(!environment_merge_loss_allowed(
        std::array<unsigned, 1>{1},
        std::array<unsigned, 1>{1},
        {receivers, floors, 100, 3000, options, true}
    ));
    CHECK(!allowed({0}, {0}, 100));        // Receiver-area loss exceeds the pair budget.
    CHECK(!allowed({0}, {0}, 1000, 500));  // Whole-level area budget.
    receivers[3].position[0] = 1;
    CHECK(!allowed({0, 3}, {0, 3}));  // Two nearby misses form a hole, not isolated outliers.
    receivers[3].position[0] = 4;
    std::vector<unsigned> many;

    for (unsigned i = 0; i < 17; ++i) {
        many.push_back(i * 3);
    }

    CHECK(!allowed(many, {48}, 10000, 100000));  // Whole-level absolute bound.
    floors[0].area = 10;
    CHECK(!allowed({0}, {0}));  // Large footprints count more than densely sampled small ones.
}

struct Fixture {
    BakeScene source;
    std::vector<EnvironmentFloor> floors;
    std::vector<std::vector<unsigned>> rooms{1};
    std::vector<EnvironmentProbe> probes;
    std::vector<unsigned> origins{0, 1};

    Fixture() {
        auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle t;
            t.vertices = {a, b, c};
            source.triangles.push_back(t);
            t.vertices = {a, c, d};
            source.triangles.push_back(t);
        };
        quad({-20, -20, 0}, {20, -20, 0}, {20, 20, 0}, {-20, 20, 0});
        quad(
            {0, -.3f, 0}, {0, .3f, 0}, {0, .3f, 1}, {0, -.3f, 1}
        );  // Low finite obstacle, not a room wall.
        auto floor = [&](Float3 p) {
            EnvironmentFloor f;
            f.position = p;
            f.capture = p;
            f.capture[2] += 3;
            f.room = 0;
            f.sky_band = 1;
            f.light = {2, 2, 2};
            f.clearance = 8;
            rooms[0].push_back(static_cast<unsigned>(floors.size()));
            floors.push_back(f);
        };
        floor({-2, 0, 0});
        floor({2, 0, 0});
        floor({0, 1, 0});
        floor({-.5f, 0, 0});
        floor({.5f, 0, 0});

        for (int y = -9; y <= 9; ++y) {
            for (int x = -9; x <= 9; ++x) {
                if (x == 0 && y == 0) {
                    continue;  // Receivers cannot be inside the obstacle.
                }

                floor({float(x), float(y), 0});
            }
        }

        probes =
            {EnvironmentProbe{floors[0].capture, {-20, -20, -.8f}, {20, 20, 4}},
             EnvironmentProbe{floors[1].capture, {-20, -20, -.8f}, {20, 20, 4}}};
    }

    EnvironmentPlacementReport coverage() const {
        EnvironmentPlacementReport report;
        report.owners = {0, 0};
        report.lighting_spaces = 1;

        for (unsigned f = 0; f < floors.size(); ++f) {
            for (unsigned h = 0; h < 3; ++h) {
                EnvironmentCoverageReceiver r;
                r.position = floors[f].position;
                r.position[2] += kEnvironmentReceiverHeights[h];
                r.height = h;

                for (unsigned p = 0; p < probes.size(); ++p) {
                    if (environment_receiver_supported(
                            probes[p], floors[origins[p]], floors[f], r.position
                        )) {
                        r.probes.push_back(p);
                    }
                }

                report.receivers.push_back(std::move(r));
            }
        }

        return report;
    }
};

// Minimum spacing applies across visible fragments even when their local GI signatures differ.
void test_close_fragments_share_one_capture() {
    Fixture fixture;
    fixture.rooms = {{}, {}};

    for (unsigned i = 0; i < fixture.floors.size(); ++i) {
        auto& floor = fixture.floors[i];
        floor.room = floor.position[0] > 0 ? 1u : 0u;
        floor.light = floor.room == 0 ? Float3{.1f, .1f, .1f} : Float3{2, 2, 2};
        floor.lighting_labels = std::vector<unsigned>{floor.room};
        fixture.rooms[floor.room].push_back(i);
    }

    CHECK(!similar_environment_capture_light(fixture.floors[0].light, fixture.floors[1].light, {}));
    Scene scene(fixture.source);
    auto report = fixture.coverage();
    report.owners = {0, 1};
    report.lighting_spaces = 2;
    report.spaces.resize(2);

    for (unsigned room = 0; room < fixture.rooms.size(); ++room) {
        report.spaces[room].cells = static_cast<unsigned>(fixture.rooms[room].size());
        report.spaces[room].neighbours = {1 - room};
    }

    merge_environment_probes(
        {scene, fixture.floors, fixture.rooms, {}, {}}, fixture.probes, fixture.origins, report
    );
    CHECK(fixture.probes.size() == 1 && fixture.rooms.size() == 1);
    CHECK(report.lighting_spaces == 1 && report.tolerated_receivers.empty());
    CHECK(report.spaces.size() == 1 && report.spaces[0].cells == fixture.floors.size());
    CHECK(report.spaces[0].neighbours.empty() && report.owners == std::vector<unsigned>{0});

    for (const auto& receiver : report.receivers) {
        CHECK(receiver.probes == std::vector<unsigned>{0});
    }
}

// A room wall blocks a proximity union even though the capture centres are only four units apart.
void test_close_fragments_keep_wall_separation() {
    Fixture fixture;
    fixture.rooms = {{}, {}};

    for (unsigned i = 0; i < fixture.floors.size(); ++i) {
        auto& floor = fixture.floors[i];
        floor.room = floor.position[0] > 0 ? 1u : 0u;
        fixture.rooms[floor.room].push_back(i);
    }

    BakeTriangle wall;
    wall.vertices = {Float3{0, -20, 0}, Float3{0, 20, 0}, Float3{0, 20, 8}};
    fixture.source.triangles.push_back(wall);
    wall.vertices = {Float3{0, -20, 0}, Float3{0, 20, 8}, Float3{0, -20, 8}};
    fixture.source.triangles.push_back(wall);
    Scene scene(fixture.source);
    auto report = fixture.coverage();
    report.owners = {0, 1};
    merge_environment_probes(
        {scene, fixture.floors, fixture.rooms, {}, {}}, fixture.probes, fixture.origins, report
    );
    CHECK(fixture.probes.size() == 2 && fixture.rooms.size() == 2);
}

// A farther pair with greater overlap cannot take precedence over the closest conflicting pair.
void test_nearest_spacing_pair_is_repaired_first() {
    Fixture fixture;
    auto floor = fixture.floors[1];
    floor.position[0] = floor.capture[0] = 6.5f;
    const unsigned origin = static_cast<unsigned>(fixture.floors.size());
    fixture.floors.push_back(floor);
    fixture.rooms[0].push_back(origin);
    fixture.origins.push_back(origin);
    fixture.probes.emplace_back(floor.capture, Float3{-20, -20, -.8f}, Float3{20, 20, 4});
    // The farther pair shares its complete footprint; the nearest pair shares only part of it.
    fixture.probes[0].maximum[0] = 3;
    auto report = fixture.coverage();
    report.owners = {0, 0, 0};
    Scene scene(fixture.source);
    merge_environment_probes(
        {scene, fixture.floors, fixture.rooms, {}, {}}, fixture.probes, fixture.origins, report
    );
    CHECK(!report.probe_merges.empty());

    if (!report.probe_merges.empty()) {
        CHECK(report.probe_merges[0].a == Float3({-2, 0, 3}));
        CHECK(report.probe_merges[0].b == Float3({2, 0, 3}));
    }
}

// Occluders inside an already connected lighting space do not reserve additional captures.
void test_shared_space_influence(bool low_roofs = false) {
    Fixture fixture;
    fixture.source.triangles.resize(2);
    BakeTriangle wall;
    wall.vertices = {Float3{0, 1, 0}, Float3{0, 10, 0}, Float3{0, 10, 4}};
    fixture.source.triangles.push_back(wall);

    if (low_roofs) {
        for (float x : {-1.f, 1.f}) {
            BakeTriangle roof;
            roof.vertices =
                {Float3{x - .05f, 3.95f, 2.6f},
                 Float3{x + .05f, 3.95f, 2.6f},
                 Float3{x + .05f, 4.05f, 2.6f}};
            fixture.source.triangles.push_back(roof);
            roof.vertices =
                {Float3{x - .05f, 3.95f, 2.6f},
                 Float3{x + .05f, 4.05f, 2.6f},
                 Float3{x - .05f, 4.05f, 2.6f}};
            fixture.source.triangles.push_back(roof);
        }
    }

    wall.vertices = {Float3{0, 1, 0}, Float3{0, 10, 4}, Float3{0, 1, 4}};
    fixture.source.triangles.push_back(wall);
    fixture.floors.clear();
    fixture.rooms = {{}};

    for (const Float3 p :
         {Float3{-1, 0, 0},
          Float3{1, 0, 0},
          Float3{low_roofs ? -3.f : -5.f, 0, 0},
          Float3{low_roofs ? 3.f : 5.f, 0, 0},
          Float3{-1, 4, 0},
          Float3{1, 4, 0}}) {
        EnvironmentFloor f;
        f.position = p;
        f.capture = p;
        f.capture[2] += 3;
        f.room = 0;
        f.light = {2, 2, 2};
        f.clearance = 8;
        fixture.rooms[0].push_back(static_cast<unsigned>(fixture.floors.size()));
        fixture.floors.push_back(f);
    }

    fixture.probes =
        {EnvironmentProbe{fixture.floors[0].capture, {-20, -20, -.8f}, {20, 20, 5}},
         EnvironmentProbe{fixture.floors[1].capture, {-20, -20, -.8f}, {20, 20, 5}}};
    Scene scene(fixture.source);
    auto report = fixture.coverage();
    merge_environment_probes(
        {scene, fixture.floors, fixture.rooms, {}, {}}, fixture.probes, fixture.origins, report
    );
    CHECK(fixture.probes.size() == 1);
    CHECK(report.covered_fraction == 1 && report.tolerated_receivers.empty());

    for (const auto& receiver : report.receivers) {
        CHECK(receiver.probes == std::vector<unsigned>{0});
    }
}

void replacement_search() {
    Fixture fixture;
    Scene scene(fixture.source);
    CHECK(environment_capture_context_clear(
        scene,
        fixture.floors[0],
        fixture.probes[0].position,
        fixture.floors[1],
        fixture.probes[1].position,
        {}
    ));
    auto blocked = fixture.source;

    for (auto& t : blocked.triangles) {
        for (auto& p : t.vertices) {
            if (p[2] == 1) {
                p[2] = 2;
            }
        }
    }

    Scene body_blocked(blocked);
    CHECK(environment_segment_clear(
        body_blocked, fixture.probes[0].position, fixture.probes[1].position, {}
    ));
    CHECK(!environment_capture_context_clear(
        body_blocked,
        fixture.floors[0],
        fixture.probes[0].position,
        fixture.floors[1],
        fixture.probes[1].position,
        {}
    ));
    auto report = fixture.coverage();
    unsigned shared = 0;

    for (const auto& r : report.receivers) {
        CHECK(!r.probes.empty());
        shared += r.probes.size() == 2;
    }

    CHECK(float(shared) / float(report.receivers.size()) > .95f);
    merge_environment_probes(
        {scene, fixture.floors, fixture.rooms, {}, {}}, fixture.probes, fixture.origins, report
    );
    CHECK(fixture.probes.size() == 1 && report.probe_merges.size() == 1);
    CHECK(report.tolerated_receivers.empty() && report.covered_fraction == 1);
    CHECK(
        fixture.probes[0].position != Float3({-2, 0, 3})
        && fixture.probes[0].position != Float3({2, 0, 3})
    );

    for (const auto& r : report.receivers) {
        CHECK(r.probes == std::vector<unsigned>{0});
    }

    Fixture split;
    Scene split_scene(split.source);
    auto split_report = split.coverage();
    split.floors[1].room = 1;
    std::erase(split.rooms[0], 1u);
    split.rooms.push_back({1});
    split_report.owners[1] = 1;
    merge_environment_probes(
        {split_scene, split.floors, split.rooms, {}, {}}, split.probes, split.origins, split_report
    );
    CHECK(
        split.probes.size() == 1 && split_report.probe_merges.size() == 1
    );  // Visible fragment IDs do not exempt close captures from minimum spacing.
    Fixture apart;
    apart.probes[0].maximum[0] = 2;
    apart.probes[1].minimum[0] = -2;
    Scene apart_scene(apart.source);
    auto apart_report = apart.coverage();
    EnvironmentBakeSettings overlap_only;
    overlap_only.minimum_capture_spacing = 0;
    merge_environment_probes(
        {apart_scene, apart.floors, apart.rooms, {}, overlap_only},
        apart.probes,
        apart.origins,
        apart_report
    );
    CHECK(
        apart.probes.size() == 2 && apart_report.merge_pair_attempts == 0
    );  // Low overlap is not a merge candidate.
    Fixture partial;
    partial.probes[0].maximum[0] = 8;
    Scene partial_scene(partial.source);
    auto partial_report = partial.coverage();
    unsigned common = 0;

    for (const auto& receiver : partial_report.receivers) {
        common += receiver.probes.size() == 2;
    }

    const float overlap = float(common) / float(partial_report.receivers.size());
    CHECK(overlap >= .85f && overlap < .95f);
    EnvironmentBakeSettings strict;
    strict.minimum_capture_spacing = 0;
    strict.merge_overlap = .95f;
    merge_environment_probes(
        {partial_scene, partial.floors, partial.rooms, {}, strict},
        partial.probes,
        partial.origins,
        partial_report
    );
    CHECK(partial.probes.size() == 2 && partial_report.merge_pair_attempts == 0);
    merge_environment_probes(
        {partial_scene, partial.floors, partial.rooms, {}, {}},
        partial.probes,
        partial.origins,
        partial_report
    );
    CHECK(partial.probes.size() == 1 && partial_report.covered_fraction == 1);
    Fixture disabled;
    Scene disabled_scene(disabled.source);
    auto disabled_report = disabled.coverage();
    EnvironmentBakeSettings options;
    options.merge_enabled = false;
    merge_environment_probes(
        {disabled_scene, disabled.floors, disabled.rooms, {}, options},
        disabled.probes,
        disabled.origins,
        disabled_report
    );
    CHECK(disabled.probes.size() == 2 && disabled_report.probe_merges.empty());
}
}  // namespace

int main() {
    perceptual_similarity();
    loss_policy();
    replacement_search();
    test_shared_space_influence();
    test_shared_space_influence(true);
    test_close_fragments_share_one_capture();
    test_close_fragments_keep_wall_separation();
    test_nearest_spacing_pair_is_repaired_first();
    return openrac::test::result();
}
