// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Differential floor matching tests against the original exhaustive loops, including ties and storeys.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/baker/planning/floor_matching.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
// Parses deterministic fixtures without allowing a malformed test input to pass as an empty object.
openrac::json::Value document(const std::string& text) {
    openrac::json::Value value;
    std::string error;

    if (!openrac::json::parse(text, value, error)) {
        throw std::runtime_error(error);
    }

    return value;
}

// Retains the original exhaustive anchor selection as an independent differential reference.
void reference_anchors(
    std::vector<EnvironmentFloorSample>& floors, const openrac::json::Value& seeds
) {
    const auto& anchors =
        seeds["floor_anchors"].is_array() ? seeds["floor_anchors"] : seeds["moby_seeds"];
    for (const auto& seed : anchors.items()) {
        // Malformed optional seeds never established floor reachability.
        if (!seed["position"].is_array() || seed["position"].items().size() != 3
            || !seed["ground_z"].is_number()) {
            continue;
        }

        float best = 16;  // Original weighted squared-distance threshold.
        unsigned selected = ~0u;
        for (unsigned id = 0; id < floors.size(); ++id) {
            const auto p = floors[id].position;
            const float dx = p[0] - float(seed["position"][0].number());
            const float dy = p[1] - float(seed["position"][1].number());
            const float dz = p[2] - float(seed["ground_z"].number());
            const float score = dx * dx + dy * dy + 4 * dz * dz;  // Original vertical weighting.

            // Strict comparison preserves the earliest original floor when scores tie.
            if (std::abs(dz) <= 1.25f && score < best) {
                best = score;
                selected = id;
            }
        }

        if (selected != ~0u) {
            floors[selected].anchor = true;
        }
    }
}

// Retains original containment precision and region order as the height-bound reference.
void reference_bounds(
    std::vector<EnvironmentFloorSample>& floors, const openrac::json::Value& seeds
) {
    for (auto& floor : floors) {
        for (const auto& region : seeds["regions"].items()) {
            const auto& bounds = region["bounds"];
            // Malformed optional boxes did not contribute height bounds.
            if (!bounds.is_array() || bounds.items().size() != 6) {
                continue;
            }

            bool inside = true;
            for (unsigned axis = 0; axis < 3; ++axis) {
                inside = inside && floor.position[axis] >= bounds[axis].number() - .001
                         && floor.position[axis] <= bounds[axis + 3].number() + .001;
            }

            if (!inside) {
                continue;
            }

            const Float2 height{float(bounds[2].number()), float(bounds[5].number())};
            if (!floor.height_bounds) {
                floor.height_bounds = height;
            } else {
                (*floor.height_bounds)[0] = std::min((*floor.height_bounds)[0], height[0]);
                (*floor.height_bounds)[1] = std::max((*floor.height_bounds)[1], height[1]);
            }
        }
    }
}

// Compares decisions and retained graph order, rather than only total anchor or region counts.
void compare(std::vector<EnvironmentFloorSample> floors, const openrac::json::Value& seeds) {
    auto expected = floors;
    reference_anchors(expected, seeds);
    reference_bounds(expected, seeds);
    CHECK(bool(match_floor_evidence(floors, seeds)));

    for (std::size_t id = 0; id < floors.size(); ++id) {
        CHECK(floors[id].position == expected[id].position);
        CHECK(floors[id].anchor == expected[id].anchor);
        CHECK(floors[id].height_bounds == expected[id].height_bounds);
    }
}

// Floor index order opposes original tie order; stacked floors and strict radius edges remain distinct.
void test_boundaries() {
    std::vector<EnvironmentFloorSample> floors(7);
    const Float3 positions[] =
        {{1, 0, 0}, {-1, 0, 0}, {0, 0, 4}, {4, 0, 0}, {0, 0, 1.25f}, {0, 0, -2}, {-.0009f, 0, 0}};
    for (unsigned id = 0; id < floors.size(); ++id) {
        floors[id].position = positions[id];
    }

    const auto seeds = document(R"({"floor_anchors":[{"position":[0,0,0],"ground_z":0},
        {"position":[0,0,4],"ground_z":4},{"position":[],"ground_z":0}],
        "regions":[{"bounds":[0,-2,-1,2,2,5]},{"bounds":[-2,-1,-3,1,1,1]},
        {"bounds":[2,0,0,-2,1,1]},{"bounds":[1,2]}]})");
    compare(floors, seeds);
    // Isolate a pure equal-distance tie and a strict four-unit rejection from nearer candidates.
    floors.resize(2);
    CHECK(bool(match_floor_evidence(floors, seeds)));
    CHECK(floors[0].anchor);
    CHECK(!floors[1].anchor);
    floors.resize(1);
    floors[0] = {};
    floors[0].position = {4, 0, 0};
    CHECK(bool(match_floor_evidence(floors, seeds)));
    CHECK(!floors[0].anchor);
    CHECK(!match_floor_evidence(floors, document("{}")));
}

// A regular multi-storey field and off-grid seeds cover negative coordinates and overlapping bounds.
void test_many_candidates() {
    std::vector<EnvironmentFloorSample> floors;
    for (int x = -12; x <= 12; ++x) {
        for (int y = -8; y <= 8; ++y) {
            for (int z : {-2, 0, 2}) {
                EnvironmentFloorSample floor;
                floor.position = {float(x), float(y), float(z)};
                floors.push_back(floor);
            }
        }
    }

    std::ostringstream text;
    text << "{\"moby_seeds\":[";
    for (int id = 0; id < 60; ++id) {
        if (id) {
            text << ',';
        }
        text << "{\"position\":[" << id % 29 - 14.5 << ',' << id % 19 - 9.5
             << ",0],\"ground_z\":" << id % 5 - 2 << '}';
    }
    text << "],\"regions\":[";
    for (int id = 0; id < 40; ++id) {
        if (id) {
            text << ',';
        }
        const int x = id % 23 - 12, y = id % 17 - 8;
        text << "{\"bounds\":[" << x << ',' << y << ",-3," << x + 5 << ',' << y + 4 << ",5]}";
    }
    text << "]}";
    compare(floors, document(text.str()));
}
}  // namespace

// Tests need no level files, graphics context or ray tracing.
int main() {
    try {
        test_boundaries();
        test_many_candidates();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
