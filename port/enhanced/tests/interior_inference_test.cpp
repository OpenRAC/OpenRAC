// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "enhanced/lighting/interiors.h"
#include "tests/check.h"
using namespace openrac::lighting::interiors;

namespace {
void quad(std::vector<Triangle>& t, Point a, Point b, Point c, Point d) {
    t.push_back({{a, b, c}});
    t.push_back({{a, c, d}});
}

std::vector<Triangle> room() {
    std::vector<Triangle> t;
    quad(t, {-12, -8, 0}, {12, -8, 0}, {12, 8, 0}, {-12, 8, 0});
    quad(t, {0, -8, 5}, {12, -8, 5}, {12, 8, 5}, {0, 8, 5});
    quad(t, {12, -8, 0}, {12, 8, 0}, {12, 8, 5}, {12, -8, 5});
    quad(t, {0, -8, 0}, {12, -8, 0}, {12, -8, 5}, {0, -8, 5});
    quad(t, {0, 8, 0}, {12, 8, 0}, {12, 8, 5}, {0, 8, 5});
    quad(t, {0, -8, 0}, {0, -2, 0}, {0, -2, 5}, {0, -8, 5});
    quad(t, {0, 2, 0}, {0, 8, 0}, {0, 8, 5}, {0, 2, 5});
    return t;
}

Transition portal() {
    Transition t{};
    t.inverse = {1, 0, 0, 0, 0, .5f, 0, 0, 0, 0, .4f, 0, 0, 0, -1, 1};
    t.light = {0, 1};
    t.ambient = {{{40, 40, 40}, {20, 20, 30}}};
    return t;
}

// Splits the indoor floor with an unsupported gap that does not obstruct endpoint visibility.
std::vector<Triangle> fragmented_room() {
    auto triangles = room();
    triangles.erase(triangles.begin(), triangles.begin() + 2);
    quad(triangles, {-12, -8, 0}, {6, -8, 0}, {6, 8, 0}, {-12, 8, 0});
    quad(triangles, {10, -8, 0}, {12, -8, 0}, {12, 8, 0}, {10, 8, 0});
    return triangles;
}

// Labels both disconnected floor patches when the same transition endpoint can see them.
void test_visible_floor_components_share_the_endpoint_label() {
    auto transition = portal();
    transition.inverse[0] = .25f;  // The endpoint search covers both sides of the floor gap.
    const auto result = infer(fragmented_room(), {transition});
    const auto label = result.seeds[1].label;
    unsigned patches = 0;
    bool all_labelled = true;

    for (const auto& region : result.regions) {
        const bool indoor_floor =
            std::any_of(region.cells.begin(), region.cells.end(), [&](unsigned cell) {
                const auto position = result.cells[cell].position;
                return position[0] > 2 && position[0] < 12 && std::abs(position[1]) < 8
                       && position[2] < 1;
            });

        // Roof and outdoor samples are outside the behaviour under test.
        if (indoor_floor) {
            ++patches;
            all_labelled = all_labelled && region.labels == std::vector<unsigned>{label};
        }
    }

    CHECK(patches == 2);
    CHECK(all_labelled);
}

// Leaves the distant component unlabelled when a real wall blocks the endpoint.
void test_endpoint_labels_do_not_cross_walls() {
    auto triangles = fragmented_room();
    quad(triangles, {7, -8, 0}, {7, 8, 0}, {7, 8, 5}, {7, -8, 5});
    auto transition = portal();
    transition.inverse[0] = .25f;
    const auto result = infer(triangles, {transition});
    unsigned checked = 0;
    bool labels_match = true;

    for (const auto& cell : result.cells) {
        // Only the supported floor behind the wall must remain unresolved.
        if (cell.position[0] > 8 && cell.position[2] < 1) {
            labels_match = labels_match && result.regions[cell.component].labels.empty();
            ++checked;
        }
    }

    CHECK(checked > 0);
    CHECK(labels_match);
}

// Respects another authored transition even when the endpoint has an unobstructed view through it.
void test_endpoint_labels_stop_at_other_transitions() {
    auto entrance = portal();
    entrance.inverse[0] = .25f;
    auto boundary = entrance;
    boundary.id = 1;
    boundary.inverse[0] = 1;
    boundary.inverse[12] = -8;  // Boundary centred in the floor gap.
    boundary.light = {1, 2};
    const auto result = infer(fragmented_room(), {entrance, boundary});
    const auto expected = result.seeds[3].label;
    unsigned checked = 0;
    bool labels_match = true;

    for (const auto& cell : result.cells) {
        // The distant floor belongs to the second transition's positive endpoint alone.
        if (cell.position[0] > 8 && cell.position[0] < 12 && std::abs(cell.position[1]) < 8
            && cell.position[2] < 1) {
            labels_match =
                labels_match
                && result.regions[cell.component].labels == std::vector<unsigned>{expected};
            ++checked;
        }
    }

    CHECK(checked > 0);
    CHECK(labels_match);
}

// Checks an open floor strip against one mood without relying on component numbering.
void check_floor_strip_label(
    const Result& result, unsigned expected, float minimum, float maximum
) {
    unsigned checked = 0;
    bool all_labelled = true;

    for (const auto& cell : result.cells) {
        // Roof samples and other floor patches are outside this assertion.
        if (cell.position[0] > minimum && cell.position[0] < maximum && cell.position[2] < 1) {
            all_labelled =
                all_labelled
                && result.regions[cell.component].labels == std::vector<unsigned>{expected};
            ++checked;
        }
    }

    CHECK(checked > 0);
    CHECK(all_labelled);
}

// Carries an entrance label beyond its finite endpoint search into a visible disconnected patch.
void test_labels_propagate_between_floor_components() {
    const auto result = infer(fragmented_room(), {portal()});
    const unsigned expected = result.seeds[1].label;
    check_floor_strip_label(result, expected, 8, 12);
}

// Air routes can turn around a floor obstruction without inventing continuous walkable support.
void test_air_labels_route_around_floor_obstructions() {
    auto triangles = fragmented_room();
    quad(triangles, {7, -8, 0}, {7, 4, 0}, {7, 4, 5}, {7, -8, 5});
    const auto result = infer(triangles, {portal()});
    const unsigned expected = result.seeds[1].label;
    check_floor_strip_label(result, expected, 8, 12);
}

// A shorter enclosed-air route wins over a distant mood that can reach the same room indirectly.
void test_air_labels_prefer_the_shorter_route() {
    auto triangles = room();

    // Extend the enclosed room to 24 units, retaining the entrance at X=0.
    for (auto& triangle : triangles) {
        for (auto& point : triangle.vertices) {
            if (point[0] > 0) {
                point[0] *= 2;
            }
        }
    }

    triangles.erase(triangles.begin(), triangles.begin() + 2);
    quad(triangles, {-12, -8, 0}, {6, -8, 0}, {6, 8, 0}, {-12, 8, 0});
    quad(triangles, {10, -8, 0}, {12, -8, 0}, {12, 8, 0}, {10, 8, 0});
    quad(triangles, {18, -8, 0}, {24, -8, 0}, {24, 8, 0}, {18, 8, 0});
    auto entrance = portal();
    entrance.inverse[0] = .25f;
    auto boundary = portal();
    boundary.id = 1;
    boundary.inverse[12] = -8;
    boundary.light = {1, 2};
    const auto result = infer(triangles, {entrance, boundary});
    const unsigned expected = result.seeds[3].label;
    check_floor_strip_label(result, expected, 18, 24);
}

// NaNs and infinities must not bypass range checks and reach Embree's ray inputs.
void test_nonfinite_inference_settings() {
    const auto triangles = room();
    for (const float value :
         {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        for (const bool slope : {false, true}) {
            Settings settings;
            // Exercise both angular and distance settings that previously used ordered comparisons alone.
            if (slope) {
                settings.slope_cosine = value;
            } else {
                settings.sky_distance = value;
            }
            bool rejected = false;
            try {
                (void)infer(triangles, {}, settings);
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            CHECK(rejected);
        }
    }
}

}  // namespace

int main() {
    test_nonfinite_inference_settings();
    test_labels_propagate_between_floor_components();
    test_air_labels_route_around_floor_obstructions();
    test_air_labels_prefer_the_shorter_route();
    test_visible_floor_components_share_the_endpoint_label();
    test_endpoint_labels_do_not_cross_walls();
    test_endpoint_labels_stop_at_other_transitions();
    const auto t = portal();

    for (Point p : {Point{1, 2, 3}, Point{-5, 4, -2}}) {
        const auto q = world(t, local(t, p));

        for (unsigned i = 0; i < 3; ++i) {
            CHECK(std::abs(q[i] - p[i]) < .001f);
        }
    }

    CHECK(crosses_transition(t, {-2, 0, 2}, {2, 0, 2}));
    CHECK(!crosses_transition(t, {-2, 4, 2}, {2, 4, 2}));
    auto rotated = t;
    rotated.inverse = {0, .5f, 0, 0, -1, 0, 0, 0, 0, 0, .4f, 0, 0, 0, -1, 1};
    const auto sample = world(rotated, {.5f, -.7f, .3f});
    const auto roundtrip = local(rotated, sample);
    CHECK(std::abs(roundtrip[0] - .5f) < .001f && std::abs(roundtrip[1] + .7f) < .001f);
    CHECK(crosses_transition(rotated, world(rotated, {-2, 0, 0}), world(rotated, {2, 0, 0})));
    auto triangles = room();
    auto result = infer(triangles, {t});
    CHECK(result.seeds.size() == 2);
    CHECK(result.blocked_portal_edges > 0);
    CHECK(
        std::any_of(
            result.walkable_edges.begin(), result.walkable_edges.end(), [&](const auto& edge) {
                return result.cells[edge[0]].component != result.cells[edge[1]].component;
            }
        )
    );  // Lighting transitions cut labels, not physical reachability.

    CHECK(result.seeds[0].cell >= 0 && result.seeds[1].cell >= 0);

    if (result.seeds[0].cell >= 0 && result.seeds[1].cell >= 0) {
        const auto a = result.cells[result.seeds[0].cell].component,
                   b = result.cells[result.seeds[1].cell].component;
        CHECK(a != b);
        CHECK(result.regions[a].labels.size() == 1 && result.regions[b].labels.size() == 1);
        CHECK(result.regions[a].sky > result.regions[b].sky);
        CHECK(result.regions[b].sky < .35f);
    }

    for (auto& tri : triangles) {
        std::swap(tri.vertices[0], tri.vertices[2]);
    }

    const auto reversed = infer(triangles, {t});
    CHECK(reversed.cells.size() == result.cells.size());
    CHECK(reversed.regions.size() == result.regions.size());
    // A second entrance without an authored boundary must expose conflicting
    // labels, not silently choose the nearer light set.
    auto open = room();
    open.resize(8);
    const auto ambiguous = infer(open, {t});
    CHECK(std::any_of(ambiguous.regions.begin(), ambiguous.regions.end(), [](const Region& r) {
        return r.labels.size() > 1;
    }));
    // Stacked roof/floor samples survive rather than being flattened to XY.
    CHECK(std::any_of(result.cells.begin(), result.cells.end(), [](const Cell& c) {
        return c.position[2] > 4;
    }));
    auto water = room();
    water[0].support = false;
    water[1].support = false;
    const auto no_ground = infer(water, {t});
    CHECK(std::none_of(no_ground.cells.begin(), no_ground.cells.end(), [](const Cell& c) {
        return c.position[2] < 1;
    }));
    auto singular = t;
    singular.inverse.fill(0);
    bool rejected = false;

    try {
        (void)infer(triangles, {singular});
    } catch (...) {
        rejected = true;
    }

    CHECK(rejected);
    return openrac::test::result();
}
