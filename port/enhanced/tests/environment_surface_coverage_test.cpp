// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>

#include "enhanced/lighting/environment_bake.h"
#include "tests/check.h"
using namespace openrac::lighting;

namespace {
void quad(BakeScene& s, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle t;
    t.vertices = {a, b, c};
    s.triangles.push_back(t);
    t.vertices = {a, c, d};
    s.triangles.push_back(t);
}

void room() {
    BakeScene s;
    quad(s, {-8, -8, 0}, {8, -8, 0}, {8, 8, 0}, {-8, 8, 0});
    quad(s, {-8, -8, 12}, {8, -8, 12}, {8, 8, 12}, {-8, 8, 12});
    quad(s, {-8, -8, 0}, {8, -8, 0}, {8, -8, 12}, {-8, -8, 12});
    quad(s, {-8, 8, 0}, {8, 8, 0}, {8, 8, 12}, {-8, 8, 12});
    quad(s, {-8, -8, 0}, {-8, 8, 0}, {-8, 8, 12}, {-8, -8, 12});
    quad(s, {8, -8, 0}, {8, 8, 0}, {8, 8, 12}, {8, -8, 12});
    EnvironmentFloorSample floor;
    floor.anchor = true;
    EnvironmentInfluence initial;
    initial.capture = 3;
    initial.minimum = {-2, -2, -1};
    initial.maximum = {2, 2, 4};
    initial.projection = EnvironmentProjection{{}, {-8, -8, 0}, {8, 8, 12}};
    std::vector regions{initial};
    const auto r = measure_environment_surface_coverage(s, s, std::span(&floor, 1), {}, regions);
    CHECK(r.relevant_area > 200 && !r.samples.empty());
    CHECK(r.uncovered_area > 0);
    CHECK(r.added_regions == 0 && regions.size() == 1);
    CHECK(regions[0].minimum == initial.minimum && regions[0].maximum == initial.maximum);
    CHECK(regions.size() <= kEnvironmentProbeLimit && r.parents.size() == regions.size());
    for (const auto& region : regions) {
        CHECK(region.capture == 3);
        CHECK(region.projection->minimum == initial.projection->minimum);
    }
    for (unsigned k = 0; k < 3; ++k) {
        CHECK(regions[0].minimum[k] <= initial.minimum[k]);
        CHECK(regions[0].maximum[k] >= initial.maximum[k]);
    }
    // A sealed cavity behind the wall has no reachable witness and is excluded.
    auto hidden = s;
    quad(hidden, {11, -2, 2}, {11, 2, 2}, {11, 2, 6}, {11, -2, 6});
    regions = {initial};
    const auto h =
        measure_environment_surface_coverage(hidden, s, std::span(&floor, 1), {}, regions);
    for (const auto& sample : h.samples) {
        CHECK(sample.position[0] <= 8.01f);
    }
    // Budget failures remain visible rather than being silently called covered.
    regions.assign(kEnvironmentProbeLimit, initial);
    const auto limited =
        measure_environment_surface_coverage(s, s, std::span(&floor, 1), {}, regions);
    CHECK(limited.uncovered_area > 0 && limited.added_regions == 0);
    CHECK(regions.size() == kEnvironmentProbeLimit);
}

void neighbouring_rooms() {
    BakeScene s;
    quad(s, {-8, -8, 0}, {8, -8, 0}, {8, 8, 0}, {-8, 8, 0});
    quad(s, {-8, -8, 12}, {8, -8, 12}, {8, 8, 12}, {-8, 8, 12});
    quad(s, {0, -8, 0}, {0, 8, 0}, {0, 8, 12}, {0, -8, 12});
    EnvironmentFloorSample a, b;
    a.position = {-4, 0, 0};
    b.position = {4, 0, 0};
    a.anchor = b.anchor = true;
    EnvironmentInfluence left, right;
    left.minimum = {-7, -3, -1};
    left.maximum = {-1, 3, 4};
    right.minimum = {1, -3, -1};
    right.maximum = {7, 3, 4};
    right.capture = 1;
    std::array floors{a, b};
    std::vector regions{left, right};
    const auto r = measure_environment_surface_coverage(s, s, floors, {}, regions);
    CHECK(r.uncovered_area > 0 && regions.size() == 2 && r.added_regions == 0);
    unsigned sides = 0;
    for (const auto& sample : r.samples) {
        if (std::abs(sample.position[0]) < .01f && std::abs(sample.normal[0]) > .9f
            && sample.position[2] > 1 && sample.position[2] < 11) {
            CHECK(sample.owner == (sample.normal[0] > 0 ? 1u : 0u));
            ++sides;
        }
    }
    CHECK(sides > 0);
    for (const auto& region : regions) {
        auto p =
            region.frame.to_local(region.capture == 0 ? Float3{4, 0, 1.25f} : Float3{-4, 0, 1.25f});
        bool inside = true;
        for (unsigned k = 0; k < 3; ++k) {
            inside &= p[k] > region.minimum[k] && p[k] < region.maximum[k];
        }
        CHECK(!inside);
    }
}

void elbow() {
    BakeScene s;
    for (float z : {0.f, 20.f}) {
        quad(s, {0, 0, z}, {32, 0, z}, {32, 4, z}, {0, 4, z});
        quad(s, {0, 4, z}, {4, 4, z}, {4, 32, z}, {0, 32, z});
    }
    std::vector<EnvironmentFloorSample> floors;
    for (int i = 2; i < 32; i += 4) {
        EnvironmentFloorSample f;
        f.position = {float(i), 2, 0};
        f.anchor = true;
        floors.push_back(f);
        f.position = {2, float(i), 0};
        floors.push_back(f);
    }
    EnvironmentInfluence initial;
    initial.minimum = {-1, -1, -1};
    initial.maximum = {33, 33, 4};
    std::vector regions{initial};
    const auto r = measure_environment_surface_coverage(s, s, floors, {}, regions);
    CHECK(r.covered_area > 0 && r.uncovered_area > 0);
    CHECK(regions.size() == 1 && r.added_regions == 0);
    CHECK(regions[0].minimum == initial.minimum && regions[0].maximum == initial.maximum);
    CHECK(r.initial_covered_area == r.covered_area);
    CHECK(r.initial_weighted_overlaps == r.weighted_overlaps);
    CHECK(regions[0].frame.axis == initial.frame.axis);
}
}  // namespace

int main() {
    room();
    elbow();
    neighbouring_rooms();
    return openrac::test::result();
}
