// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>
#include <functional>
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
void quad(BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle t;
    t.vertices = {a, b, c};
    scene.triangles.push_back(t);
    t.vertices = {a, c, d};
    scene.triangles.push_back(t);
}

void doorway() {
    BakeScene source;
    quad(source, {-6, -4, 0}, {6, -4, 0}, {6, 4, 0}, {-6, 4, 0});
    quad(source, {-6, -4, 0}, {6, -4, 0}, {6, -4, 4}, {-6, -4, 4});
    quad(source, {-6, 4, 0}, {6, 4, 0}, {6, 4, 4}, {-6, 4, 4});
    quad(source, {-6, -4, 0}, {-6, 4, 0}, {-6, 4, 4}, {-6, -4, 4});
    quad(source, {6, -4, 0}, {6, 4, 0}, {6, 4, 4}, {6, -4, 4});
    quad(source, {0, -4, 0}, {0, -.5f, 0}, {0, -.5f, 4}, {0, -4, 4});
    quad(source, {0, .5f, 0}, {0, 4, 0}, {0, 4, 4}, {0, .5f, 4});
    std::vector<EnvironmentFloorSample> floors;

    for (int y = -3; y <= 3; ++y) {
        for (int x = -5; x <= 5; ++x) {
            if (x == 0 && y != 0) {
                continue;
            }

            floors.push_back({{float(x), float(y), 0}, 7, float((x + y) & 1), false, {}});
        }
    }

    for (unsigned a = 0; a < floors.size(); ++a) {
        for (unsigned b = 0; b < floors.size(); ++b) {
            const auto& p = floors[a].position;
            const auto& q = floors[b].position;

            if (std::abs(p[0] - q[0]) + std::abs(p[1] - q[1]) == 1) {
                floors[a].neighbours.push_back(b);
            }
        }
    }

    Scene scene(source);
    const auto layout = room_layout(scene, floors, {});
    CHECK(layout.rooms.size() == 2);
    CHECK(layout.owners.front() != layout.owners.back());

    for (unsigned room = 0; room < layout.rooms.size(); ++room) {
        const auto centre = layout.centre(floors, room);
        CHECK(layout.clearance[centre] >= 1.5f);
    }

    // Sky changes must have no effect on geometric room identities.
    for (auto& floor : floors) {
        floor.sky = 0;
    }

    CHECK(room_layout(scene, floors, {}).owners == layout.owners);

    // With authored mood evidence, a same-lit doorway remains part of the continuous graph.
    for (auto& floor : floors) {
        floor.lighting_labels = std::vector<unsigned>{7};
    }

    CHECK(room_layout(scene, floors, {}).rooms.size() == 1);

    for (auto& floor : floors) {
        if (floor.position[0] > 0) {
            floor.region = 8;
            floor.lighting_labels = std::vector<unsigned>{8};
        }
    }

    CHECK(room_layout(scene, floors, {}).rooms.size() == 2);
}

std::vector<EnvironmentFloorSample> cardinal_floors(
    const std::function<bool(float, float)>& inside
) {
    std::vector<EnvironmentFloorSample> floors;
    std::map<std::pair<int, int>, unsigned> ids;

    for (int x = -26; x <= 26; ++x) {
        for (int y = -26; y <= 26; ++y) {
            if (inside(float(x), float(y))) {
                ids[{x, y}] = static_cast<unsigned>(floors.size());
                floors.push_back({{float(x), float(y), 0}, 0, 0, false, {}});
            }
        }
    }

    for (const auto& [p, a] : ids) {
        for (auto delta : {std::pair{1, 0}, std::pair{0, 1}}) {
            const auto next = ids.find({p.first + delta.first, p.second + delta.second});

            if (next != ids.end()) {
                floors[a].neighbours.push_back(next->second);
                floors[next->second].neighbours.push_back(a);
            }
        }
    }

    return floors;
}

void bent_medial_paths() {
    for (bool junction : {false, true}) {
        BakeScene source;
        quad(source, {-30, -30, 0}, {30, -30, 0}, {30, 30, 0}, {-30, 30, 0});
        const std::vector<Float2> outline =
            junction
                ? std::vector<
                      Float2>{{-24, -3}, {24, -3}, {24, 3}, {3, 3}, {3, 24}, {-3, 24}, {-3, 3}, {-24, 3}}
                : std::vector<Float2>{{-24, -3}, {3, -3}, {3, 24}, {-3, 24}, {-3, 3}, {-24, 3}};

        for (unsigned a = 0; a < outline.size(); ++a) {
            const auto p = outline[a], q = outline[(a + 1) % outline.size()];
            quad(source, {p[0], p[1], 0}, {q[0], q[1], 0}, {q[0], q[1], 8}, {p[0], p[1], 8});
        }

        const auto floors = cardinal_floors([&](float x, float y) {
            return (x > -24 && x < (junction ? 24 : 3) && std::abs(y) < 3)
                   || (std::abs(x) < 3 && y >= 3 && y < 24);
        });
        Scene scene(source);
        const auto layout = room_layout(scene, floors, {});
        CHECK(layout.rooms.size() == 1 && !layout.medial_edges.empty());
        CHECK(std::abs(layout.frame(floors, 0, {-18, 0, 0}).axis[0]) > .95f);
        CHECK(std::abs(layout.frame(floors, 0, {0, 18, 0}).axis[1]) > .95f);
        std::vector<unsigned> degree(layout.medial_points.size());

        for (auto edge : layout.medial_edges) {
            CHECK(room_path_clear(
                scene, layout.medial_points[edge[0]], layout.medial_points[edge[1]], {}
            ));
            ++degree[edge[0]];
            ++degree[edge[1]];
        }

        if (junction) {
            CHECK(std::any_of(degree.begin(), degree.end(), [](auto d) { return d >= 3; }));
        } else {
            CHECK(!room_path_clear(scene, {-10, 0, 0}, {0, 10, 0}, {}));
        }
    }
}

void diagonal_distance_field() {
    BakeScene source;
    ProbeFrame orientation{{.6f, .8f}};
    auto p = [&](float x, float y, float z) {
        return orientation.to_world({x, y, z});
    };
    quad(source, p(-25, -3, 0), p(25, -3, 0), p(25, 3, 0), p(-25, 3, 0));
    quad(source, p(-25, -3, 0), p(25, -3, 0), p(25, -3, 8), p(-25, -3, 8));
    quad(source, p(-25, 3, 0), p(25, 3, 0), p(25, 3, 8), p(-25, 3, 8));
    Scene scene(source);
    const auto distance = room_wall_distance(scene, p(0, 1, 1.25f));
    CHECK(distance.found && std::abs(distance.distance - 2) < .001f);
    // The input is deliberately world-axis cardinal. The previous implementation's diagonal
    // fixture supplied a diagonal graph and therefore never tested removal of the grid bias.
    auto floors = cardinal_floors([&](float x, float y) {
        const auto q = orientation.to_local({x, y, 0});
        return std::abs(q[0]) < 22 && std::abs(q[1]) < 2.5f;
    });

    for (auto& f : floors) {
        f.height_bounds = Float2{-1, 4};
    }

    const auto layout = room_layout(scene, floors, {});
    CHECK(layout.rooms.size() == 1 && !layout.medial_edges.empty());
    const auto axis = layout.frame(floors, 0, {}).axis;
    CHECK(std::abs(axis[0] * orientation.axis[0] + axis[1] * orientation.axis[1]) > .98f);
    double total = 0, following = 0;

    for (auto edge : layout.medial_edges) {
        const auto a = layout.medial_points[edge[0]], b = layout.medial_points[edge[1]];
        const auto d = add(b, mul(a, -1));
        const float length = std::hypot(d[0], d[1]);

        if (length < .01f) {
            continue;
        }

        total += length;

        if (std::abs((d[0] * .6f + d[1] * .8f) / length) > .95f) {
            following += length;
        }

        CHECK(room_path_clear(scene, a, b, {}));
    }

    CHECK(total > 20 && following / total > .8);

    for (auto& f : floors) {
        f.height_bounds = Float2{-1, 16};
    }

    const auto higher = room_layout(scene, floors, {});
    CHECK(
        higher.medial_points == layout.medial_points && higher.medial_edges == layout.medial_edges
    );

    for (auto& triangle : source.triangles) {
        std::swap(triangle.vertices[0], triangle.vertices[2]);
    }

    Scene reversed(source);
    CHECK(std::abs(room_wall_distance(reversed, p(0, 1, 1.25f)).distance - 2) < .001f);
}

void curved_distance_field() {
    BakeScene source;
    quad(source, {-25, -25, 0}, {25, -25, 0}, {25, 25, 0}, {-25, 25, 0});
    constexpr unsigned segments = 32;

    for (float radius : {12.f, 18.f}) {
        for (unsigned i = 0; i < segments; ++i) {
            const float a = float(i) * kPi / (2 * segments),
                        b = float(i + 1) * kPi / (2 * segments);
            quad(
                source,
                {radius * std::cos(a), radius * std::sin(a), 0},
                {radius * std::cos(b), radius * std::sin(b), 0},
                {radius * std::cos(b), radius * std::sin(b), 8},
                {radius * std::cos(a), radius * std::sin(a), 8}
            );
        }
    }

    const auto floors = cardinal_floors([](float x, float y) {
        return x > 0 && y > 0 && std::abs(std::hypot(x, y) - 15) < 2.2f;
    });
    Scene scene(source);
    const auto layout = room_layout(scene, floors, {});
    CHECK(layout.rooms.size() == 1 && layout.medial_ridge_samples > 0);

    for (float angle : {20.f, 70.f}) {
        const float a = angle * kPi / 180;
        const Float3 position{15 * std::cos(a), 15 * std::sin(a), 0};
        const auto frame = layout.frame(floors, 0, position);
        CHECK(std::abs(-frame.axis[0] * std::sin(a) + frame.axis[1] * std::cos(a)) > .94f);
    }

    CHECK(!room_path_clear(scene, {15, 0, 0}, {0, 15, 0}, {}));

    for (auto edge : layout.medial_edges) {
        CHECK(
            room_path_clear(scene, layout.medial_points[edge[0]], layout.medial_points[edge[1]], {})
        );
    }
}

void field_domain() {
    RoomDistanceField field;
    field.domain[{0, 0, 0}] = {0};
    field.domain[{0, 2, 0}] = {0};
    CHECK(field.contains({0, 0, 0}, 0));
    CHECK(!field.contains({0, 0, 0}, 1));
    CHECK(!field.contains({0, 0, 4}, 0));
    CHECK(
        field.contains({.5f, .5f, 0}, 0)
    );  // Closed cell boundaries cannot depend on rounding direction.
    CHECK(!field.contains({.5002f, .5002f, 0}, 0));
    CHECK(!field.connects({0, 0, 0}, {2, 0, 0}, 0));
    field.domain[{0, 1, 0}] = {0};
    CHECK(field.connects({0, 0, 0}, {2, 0, 0}, 0));
}

void ceiling_enclosure() {
    auto scene = [](bool enclosed) {
        BakeScene source;
        quad(source, {-10, -10, 0}, {10, -10, 0}, {10, 10, 0}, {-10, 10, 0});
        quad(source, {-10, -10, 8}, {10, -10, 8}, {10, 10, 8}, {-10, 10, 8});

        if (enclosed) {
            quad(source, {-10, -10, 0}, {10, -10, 0}, {10, -10, 8}, {-10, -10, 8});
            quad(source, {-10, 10, 0}, {10, 10, 0}, {10, 10, 8}, {-10, 10, 8});
            quad(source, {-10, -10, 0}, {-10, 10, 0}, {-10, 10, 8}, {-10, -10, 8});
            quad(source, {10, -10, 0}, {10, 10, 0}, {10, 10, 8}, {10, -10, 8});
        }

        return source;
    };
    const std::vector<EnvironmentFloorSample> floors{{{0, 0, 0}, 0, 0, false, {}}};
    Scene interior(scene(true)), overhang(scene(false));
    CHECK(std::abs(room_layout(interior, floors, {}).ceilings[0] - 8) < .001f);
    CHECK(!std::isfinite(room_layout(overhang, floors, {}).ceilings[0]));
    auto bounded = floors;
    bounded[0].height_bounds = Float2{-2, 5};
    CHECK(room_layout(overhang, bounded, {}).height_bounds[0] == Float2({-2, 5}));
    CHECK(room_layout(interior, bounded, {}).height_bounds[0] == Float2({-2, 8}));
}

void rotated_corridor() {
    BakeScene source;
    ProbeFrame frame{{.6f, .8f}};
    auto p = [&](float x, float y, float z) {
        return frame.to_world({x, y, z});
    };
    quad(source, p(-12, -3, 0), p(12, -3, 0), p(12, 3, 0), p(-12, 3, 0));
    quad(source, p(-12, -3, 0), p(12, -3, 0), p(12, -3, 5), p(-12, -3, 5));
    quad(source, p(-12, 3, 0), p(12, 3, 0), p(12, 3, 5), p(-12, 3, 5));
    std::vector<EnvironmentFloorSample> floors;

    for (int x = -10; x <= 10; ++x) {
        floors.push_back({p(float(x), 0, 0), 4, 1, false, {}});
    }

    for (unsigned a = 0; a < floors.size(); ++a) {
        if (a) {
            floors[a].neighbours.push_back(a - 1);
        }

        if (a + 1 < floors.size()) {
            floors[a].neighbours.push_back(a + 1);
        }
    }

    Scene scene(source);
    const auto layout = room_layout(scene, floors, {});
    CHECK(layout.rooms.size() == 1);
    const auto fitted = layout.frame(floors, 0, {}, 24);
    CHECK(std::abs(fitted.axis[0] * frame.axis[0] + fitted.axis[1] * frame.axis[1]) > .999f);
    CHECK(std::abs(floors[layout.centre(floors, 0)].position[0]) < 1);
    ProbeGrid broad;
    broad.origin = {-15, -15, -2};
    broad.spacing = {1, 1, 2};
    broad.dimensions = {31, 31, 5};
    const auto fitted_grid = fit_room_grid(broad, source, floors, layout);
    auto wide_spacing = broad;
    wide_spacing.spacing = {4, 4, 2};
    wide_spacing.dimensions = {9, 9, 5};
    const auto adaptive_grid = fit_room_grid(wide_spacing, source, floors, layout, true);
    CHECK(adaptive_grid.spacing[1] == 2);
    CHECK(adaptive_grid.probe_count() <= wide_spacing.probe_count());
    CHECK(adaptive_grid.spacing[2] == broad.spacing[2]);
    CHECK(adaptive_grid.dimensions[2] == broad.dimensions[2]);
    CHECK(fitted_grid.probe_count() < broad.probe_count());
    CHECK(
        std::abs(
            fitted_grid.frame.axis[0] * frame.axis[0] + fitted_grid.frame.axis[1] * frame.axis[1]
        )
        > .999f
    );

    for (const auto& floor : floors) {
        auto q = floor.position;

        for (unsigned a = 0; a < 3; ++a) {
            q[a] -= fitted_grid.origin[a];
        }

        q = fitted_grid.frame.to_local(q);

        for (unsigned a = 0; a < 3; ++a) {
            CHECK(
                q[a] >= -.001f
                && q[a] <= fitted_grid.spacing[a] * float(fitted_grid.dimensions[a] - 1) + .001f
            );
        }
    }

    // A nearby stacked pipe remains a separate space when no walkable edge connects it.
    auto stacked = floors;

    for (auto floor : floors) {
        floor.position[2] += 5;

        for (auto& neighbour : floor.neighbours) {
            neighbour += static_cast<unsigned>(floors.size());
        }

        stacked.push_back(std::move(floor));
    }

    CHECK(room_layout(scene, stacked, {}).rooms.size() == 2);
}
}  // namespace

int main() {
    doorway();
    rotated_corridor();
    bent_medial_paths();
    diagonal_distance_field();
    curved_distance_field();
    ceiling_enclosure();
    field_domain();
    return openrac::test::result();
}
