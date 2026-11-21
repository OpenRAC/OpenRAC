// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>
#include <functional>
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
// A connected corridor must remain one skeleton after ridge projection and smoothing.
void check_connected_skeleton(const RoomLayout& layout) {
    CHECK(!layout.medial_edges.empty());
    if (layout.medial_edges.empty()) {
        return;
    }
    std::vector<std::vector<unsigned>> neighbours(layout.medial_points.size());
    for (auto edge : layout.medial_edges) {
        neighbours[edge[0]].push_back(edge[1]);
        neighbours[edge[1]].push_back(edge[0]);
    }
    const auto root = layout.medial_edges.front()[0];
    std::vector<bool> seen(neighbours.size());
    std::vector<unsigned> pending{root};
    seen[root] = true;
    for (unsigned i = 0; i < pending.size(); ++i) {
        for (auto b : neighbours[pending[i]]) {
            if (!seen[b]) {
                seen[b] = true;
                pending.push_back(b);
            }
        }
    }
    for (unsigned i = 0; i < neighbours.size(); ++i) {
        CHECK(neighbours[i].empty() || seen[i]);
    }
}

void quad(BakeScene& scene, Float3 a, Float3 b, Float3 c, Float3 d) {
    BakeTriangle t;
    t.vertices = {a, b, c};
    scene.triangles.push_back(t);
    t.vertices = {a, c, d};
    scene.triangles.push_back(t);
}

void doorway(bool wide = false) {
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

    if (wide) {
        for (auto& triangle : source.triangles) {
            for (auto& point : triangle.vertices) {
                point[0] *= 2;
                point[1] *= std::abs(point[1]) == .5f ? 4 : 2;
            }
        }
        for (auto& floor : floors) {
            floor.position[0] *= 2;
            floor.position[1] *= 2;
        }

        Scene scene(source);
        CHECK(room_layout(scene, floors, {}, false).rooms.size() == 1);
        const auto layout = room_layout(scene, floors, {}, false, true);
        CHECK(layout.rooms.size() == 2);
        CHECK(layout.owners.front() != layout.owners.back());
        CHECK(room_layout(scene, floors, {}, false, true).owners == layout.owners);
        for (auto& floor : floors) {
            floor.anchor = true;
            floor.region = floor.position[0] < 0 ? 7 : 8;
            floor.lighting_labels = std::vector<unsigned>{floor.region};
        }
        // Removing lighting cuts must still preserve the physical doorway.
        const auto shared = prepare_room_layout(source, floors, {}, 1);
        CHECK(shared.owners == layout.owners);
        for (unsigned i = 0; i < floors.size(); ++i) {
            CHECK(floors[i].island == shared.owners[i]);
        }
        ProbeGrid prototype;
        const auto grids = coverage_probe_grids(prototype, floors, shared, {2, 2, 2}, 128);
        CHECK(grids.size() > shared.rooms.size());
        for (const auto& allocation : grids) {
            CHECK(allocation.grid.probe_count() <= 128);
            CHECK(!allocation.islands.empty());
        }
        for (unsigned i = 0; i < floors.size(); ++i) {
            for (float height : {.15f, 1.25f, 2.5f}) {
                auto p = floors[i].position;
                p[2] += height;
                CHECK(std::any_of(grids.begin(), grids.end(), [&](const auto& allocation) {
                    return std::find(
                               allocation.islands.begin(),
                               allocation.islands.end(),
                               *floors[i].island
                           ) != allocation.islands.end()
                           && probe_boundary_weight(allocation.grid, p) > 0;
                }));
            }
        }
        const auto gi = ProbeVolume::allocate(prototype);
        EnvironmentPlacementReport report;
        const auto map = environment_floor_map(scene, std::span(&gi, 1), floors, {}, {}, report);
        CHECK(map.rooms.size() == shared.rooms.size());
        CHECK(report.spaces_before_merge == report.lighting_spaces);
        for (const auto& floor : map.floors) {
            CHECK(floor.island == floor.room);
        }
        for (const auto& mark : report.watershed) {
            CHECK(mark.basin < shared.rooms.size());
        }
        return;
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
        check_connected_skeleton(layout);
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
    const auto query = p(0, 1, 1.25f);
    const auto distance = room_wall_distance(scene, query);
    CHECK(distance.found && std::abs(distance.distance - 2) < .001f);
    const auto capped = room_wall_distance(scene, query, 1.f);
    CHECK(!capped.found && capped.distance == 1.f);
    const auto within_range = room_wall_distance(scene, query, 2.5f);
    CHECK(within_range.found && within_range.distance == distance.distance);
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
    check_connected_skeleton(layout);
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
    check_connected_skeleton(layout);

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

// A wall within an endpoint margin must block both directions of a swimming connection.
void test_swimming_endpoint_clearance() {
    BakeScene source;
    constexpr float kWallX = -.049f;  // Between the overlap margin and the forward ray's near clip.
    quad(source, {kWallX, -1, 0}, {kWallX, 1, 0}, {kWallX, 1, 2}, {kWallX, -1, 2});
    Scene scene(source);
    const Float3 a{0, 0, 0}, b{1, 0, 0}, away{2, 0, 0};
    CHECK(!room_body_path_clear(scene, a, b, {}, true));
    CHECK(!room_body_path_clear(scene, b, a, {}, true));
    CHECK(room_body_path_clear(scene, b, away, {}, true));
    CHECK(room_body_path_clear(scene, away, b, {}, true));
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
    const auto interior_source = scene(true), overhang_source = scene(false);
    Scene interior(interior_source), overhang(overhang_source);
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

void island_column_budget() {
    std::vector<EnvironmentFloorSample> floors(1);
    floors[0].island = 0;
    RoomLayout layout;
    layout.owners = {0};
    layout.rooms = {{0}};
    layout.height_bounds = {{-1, 120}};
    const auto grids = coverage_probe_grids({}, floors, layout, {1, 1, 1}, 64);
    CHECK(grids.size() > 1);
    for (const auto& allocation : grids) {
        CHECK(
            allocation.islands == std::vector<unsigned>{0} && allocation.grid.probe_count() <= 64
        );
    }
    for (float height = 0; height <= 120; height += .5f) {
        CHECK(std::any_of(grids.begin(), grids.end(), [&](const auto& allocation) {
            return probe_boundary_weight(allocation.grid, {0, 0, height}) > 0;
        }));
    }
    bool rejected = false;
    try {
        coverage_probe_grids({}, floors, layout, {1, 1, 1}, 64, 1);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
}

void neighbouring_islands_share_storage() {
    std::vector<EnvironmentFloorSample> floors(2);
    floors[0].position = {1, 1, 0};
    floors[1].position = {2, 1, 0};
    RoomLayout layout;
    layout.owners = {0, 1};
    layout.rooms = {{0}, {1}};
    layout.height_bounds = {{-.8f, 3.8f}, {-.8f, 3.8f}};
    const auto grids = coverage_probe_grids({}, floors, layout, {1, 1, 1});
    CHECK(grids.size() == 1);
    CHECK(grids[0].islands == std::vector<unsigned>({0, 1}));
    CHECK(grids[0].grid.frame.axis == Float2({1, 0}));
}

void small_watershed_cleanup() {
    // Absorb a shallow side patch across lighting labels. Preserve doorways, water,
    // isolated patches, stacked floors, and two separate shared boundary arcs.
    for (unsigned variant = 0; variant < 18; ++variant) {
        std::vector<EnvironmentFloorSample> floors(10);
        RoomLayout layout;
        layout.rooms = {{0, 1, 2, 3, 4, 5, 6, 7}, {8, 9}};
        layout.owners = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1};
        layout.clearance.resize(10, 3);
        for (unsigned a = 0; a < 10; ++a) {
            floors[a].position = {float(a), 0, 0};
            if (a) {
                floors[a].neighbours.push_back(a - 1);
            }
            if (a + 1 < 10) {
                floors[a].neighbours.push_back(a + 1);
            }
        }
        if (variant == 1 || variant == 6 || variant == 15) {
            if (variant != 6) {
                layout.clearance[7] = layout.clearance[8] = variant == 1 ? .2f : 1.6f;
            }
            // Give doorway and multiple-boundary negatives a genuine interior.
            for (unsigned n = 0; n < 3; ++n) {
                const unsigned id = unsigned(floors.size());
                auto floor = floors.back();
                floor.position[0] += 1;
                floor.neighbours = {id - 1};
                floors.back().neighbours.push_back(id);
                floors.push_back(floor);
                layout.owners.push_back(1);
                layout.rooms[1].push_back(id);
                layout.clearance.push_back(3);
            }
        }
        if (variant == 2) {
            floors[8].region = floors[9].region = 1;
        }
        if (variant == 3) {
            floors[8].underwater = floors[9].underwater = true;
        }
        if (variant == 4) {
            floors[7].neighbours = {6};
            floors[8].neighbours = {9};
        }
        if (variant == 5) {
            floors[9].position = {0, 0, 3};
        }
        if (variant == 6 || variant == 17) {
            floors[0].neighbours.push_back(9);
            floors[9].neighbours.push_back(0);
        }
        if (variant >= 7 && variant <= 12) {
            for (unsigned i = 0; i < floors.size(); ++i) {
                auto& floor = floors[i];
                floor.underwater = true;
                floor.water_height = 10;
                floor.lighting_labels = std::vector<unsigned>{0};
                floor.region = i < 8 ? 1 : 2;
                floor.swimming_surface = i >= 8 && variant != 10;
            }
            if (variant == 8) {
                floors[8].lighting_labels = floors[9].lighting_labels = std::vector<unsigned>{1};
            }
            if (variant == 9) {
                floors[8].water_height = floors[9].water_height = 12;
            }
            if (variant >= 11) {
                for (unsigned i = 0; i < 8; ++i) {
                    floors[i].lighting_labels = std::vector<unsigned>{};
                }
                if (variant == 12) {
                    floors[0].lighting_labels = std::vector<unsigned>{1};
                }
            }
        }
        if (variant == 13 || variant == 14) {
            layout.clearance[7] = layout.clearance[8] = .6f;
            layout.clearance[9] = 1.7f;
            if (variant == 14) {
                for (auto& floor : floors) {
                    floor.position[0] *= 2;
                }
            }
        }
        if (variant == 16) {
            for (auto& floor : floors) {
                floor.position[0] *= 2;
            }
            std::fill(layout.clearance.begin(), layout.clearance.end(), 6.f);
            layout.clearance[7] = .6f;
        }
        merge_small_watershed_regions(floors, layout);
        CHECK(
            layout.rooms.size()
            == (variant == 0 || variant == 2 || variant == 7 || variant == 8 || variant == 10
                        || variant == 11 || variant == 12 || variant == 13 || variant == 14
                        || variant == 15 || variant == 16 || variant == 17
                    ? 1u
                    : 2u)
        );
        unsigned count = 0;
        for (unsigned room = 0; room < layout.rooms.size(); ++room) {
            for (auto a : layout.rooms[room]) {
                ++count;
                CHECK(layout.owners[a] == room);
            }
        }
        CHECK(count == floors.size());
        const auto owners = layout.owners;
        merge_small_watershed_regions(floors, layout);
        CHECK(layout.owners == owners);
    }
}

void underwater_edge_receiver_ownership() {
    for (unsigned variant = 0; variant < 5; ++variant) {
        BakeScene geometry;
        if (variant == 2) {
            quad(geometry, {-1, -2, 0}, {.4f, -2, 0}, {.4f, 2, 0}, {-1, 2, 0});
            quad(geometry, {1.6f, -2, 0}, {6, -2, 0}, {6, 2, 0}, {1.6f, 2, 0});
        } else {
            quad(geometry, {-1, -2, 0}, {6, -2, 0}, {6, 2, 0}, {-1, 2, 0});
        }
        quad(geometry, {-.14f, -2, 0}, {-.14f, 2, 0}, {-.14f, 2, 4}, {-.14f, -2, 4});
        if (variant == 1) {
            quad(geometry, {1, -2, 0}, {1, 2, 0}, {1, 2, 4}, {1, -2, 4});
        }
        std::vector<EnvironmentFloorSample> floors(5);
        RoomLayout layout;
        layout.rooms = {{0}, {1, 2, 3, 4}};
        layout.owners = {0, 1, 1, 1, 1};
        layout.clearance = {.14f, 3, 3, 3, 3};
        for (unsigned a = 0; a < floors.size(); ++a) {
            auto& floor = floors[a];
            floor.position = {a ? float(a + 1) : 0, 0, 0};
            floor.underwater = true;
            floor.water_height = 5;
            floor.region = a ? 1 : 2;
            floor.lighting_labels = std::vector<unsigned>{0};
            floor.swimming_surface = a == 0;
            if (a > 1) {
                floor.neighbours.push_back(a - 1);
            }
            if (a && a + 1 < floors.size()) {
                floor.neighbours.push_back(a + 1);
            }
        }
        if (variant == 3) {
            floors[0].water_height = 7;
        }
        if (variant == 4) {
            floors[0].underwater = false;
        }
        const auto original = floors;
        Scene scene(geometry);
        merge_small_watershed_regions(floors, layout, &scene, {});
        CHECK(layout.rooms.size() == (variant == 0 ? 1u : 2u));
        for (unsigned a = 0; a < floors.size(); ++a) {
            CHECK(floors[a].neighbours == original[a].neighbours);
        }
        if (variant == 0) {
            CHECK(!room_supported_path_clear(scene, floors[0].position, floors[0].position, {}));
        }
    }
}

void steep_underwater_receiver_ownership() {
    // A non-walkable submerged face can share basin ownership, but not through a wall
    // or without a common collision-checked water-surface connection.
    for (unsigned variant = 0; variant < 3; ++variant) {
        BakeScene geometry;
        quad(geometry, {-.1f, -2, 1}, {.1f, -2, 0}, {.1f, 2, 0}, {-.1f, 2, 1});
        quad(geometry, {.1f, -2, 0}, {6, -2, 0}, {6, 2, 0}, {.1f, 2, 0});
        if (variant == 1) {
            quad(geometry, {1, -2, 0}, {1, 2, 0}, {1, 2, 4}, {1, -2, 4});
        }
        std::vector<EnvironmentFloorSample> floors(6);
        RoomLayout layout;
        layout.rooms = {{0}, {1, 2, 3, 4}, {5}};
        layout.owners = {0, 1, 1, 1, 1, 2};
        layout.clearance = {.2f, 3, 3, 3, 3, 3};
        for (unsigned a = 0; a < 5; ++a) {
            auto& floor = floors[a];
            floor.position = {a ? float(a + 1) : 0, 0, a ? 0 : .5f};
            floor.underwater = true;
            floor.water_height = 5;
            floor.region = a ? 1 : 2;
            floor.lighting_labels = std::vector<unsigned>{0};
            floor.swimming_surface = a == 0;
            if (a > 1) {
                floor.neighbours.push_back(a - 1);
            }
            if (a && a + 1 < 5) {
                floor.neighbours.push_back(a + 1);
            }
        }
        floors[5].position = {0, 0, 5};
        floors[5].swimming_surface = true;
        floors[5].water_height = 5;
        floors[0].neighbours = {5};
        floors[5].neighbours = {0};
        if (variant != 2) {
            floors[1].neighbours.push_back(5);
            floors[5].neighbours.push_back(1);
        }
        const auto original = floors;
        Scene scene(geometry);
        CHECK(!room_floor_position(scene, floors[0].position, {}));
        merge_small_watershed_regions(floors, layout, &scene, {});
        CHECK((layout.owners[0] == layout.owners[1]) == (variant == 0));
        for (unsigned a = 0; a < floors.size(); ++a) {
            CHECK(floors[a].neighbours == original[a].neighbours);
        }
        if (variant == 0) {
            auto render = geometry;
            quad(render, {1, -2, 0}, {1, 2, 0}, {1, 2, 4}, {1, -2, 4});
            auto visual_floors = original, collision_floors = original;
            const auto visual = prepare_room_layout(render, visual_floors, {}, 1);
            const auto physical = prepare_room_layout(render, collision_floors, {}, 1, &geometry);
            CHECK(visual.owners[0] != visual.owners[1]);
            CHECK(physical.owners[0] == physical.owners[1]);
        }
    }
}

void open_water_basin_regions() {
    // Broad swimming space above low basin clutter should not inherit a floor bottleneck.
    for (unsigned variant = 0; variant < 7; ++variant) {
        const float scale = variant >= 5 ? 2.f : 1.f;
        std::vector<EnvironmentFloorSample> floors;
        RoomLayout layout;
        layout.rooms.resize(variant == 2 ? 4 : 3);
        for (unsigned room = 0; room < 2; ++room) {
            for (int x = 0; x < 5; ++x) {
                for (int y = -2; y <= 2; ++y) {
                    EnvironmentFloorSample floor;
                    floor.position = {float(room ? 1 + x : -5 + x) * scale, float(y) * scale, 0};
                    floor.underwater = true;
                    floor.water_height = 8;
                    floor.region = 1;
                    floor.lighting_labels = std::vector<unsigned>{0};
                    layout.owners.push_back(room);
                    layout.rooms[room].push_back(unsigned(floors.size()));
                    layout.clearance.push_back(4);
                    floors.push_back(floor);
                }
            }
        }
        for (unsigned a = 0; a < 50; ++a) {
            for (unsigned b = 0; b < 50; ++b) {
                const auto p = floors[a].position, q = floors[b].position;
                if (layout.owners[a] == layout.owners[b]
                    && std::abs(p[0] - q[0]) + std::abs(p[1] - q[1]) == scale) {
                    floors[a].neighbours.push_back(b);
                }
            }
        }
        floors[22].neighbours.push_back(27);
        floors[27].neighbours.push_back(22);
        layout.clearance[22] = layout.clearance[27] = .5f;
        if (variant == 4) {
            floors[0].neighbours.push_back(49);
            floors[49].neighbours.push_back(0);
            layout.clearance[0] = layout.clearance[49] = .5f;
        }
        std::map<std::pair<int, int>, unsigned> surface;
        for (int x = -10; x <= 10; x += 2) {
            for (int y = -4; y <= 4; y += 2) {
                if (variant == 3 && x == 0) {
                    continue;
                }
                const unsigned owner = variant == 2 && x >= 0 ? 3 : 2;
                EnvironmentFloorSample floor;
                floor.position = {float(x), float(y), 8};
                floor.swimming_surface = true;
                floor.water_height = 8;
                floor.region = 77;
                floor.lighting_labels = std::vector<unsigned>{0};
                surface[{x, y}] = unsigned(floors.size());
                layout.rooms[owner].push_back(unsigned(floors.size()));
                layout.owners.push_back(owner);
                layout.clearance.push_back(4);
                floors.push_back(floor);
            }
        }
        for (auto pair : {std::pair{22u, surface[{-2, 0}]}, std::pair{27u, surface[{2, 0}]}}) {
            floors[pair.first].neighbours.push_back(pair.second);
            floors[pair.second].neighbours.push_back(pair.first);
        }
        BakeScene geometry;
        quad(geometry, {-14, -8, 0}, {14, -8, 0}, {14, 8, 0}, {-14, 8, 0});
        const float height = variant == 1 || variant == 6 ? 9.f : 2.f;
        quad(geometry, {0, -8, 0}, {0, 8, 0}, {0, 8, height}, {0, -8, height});
        Scene scene(geometry);
        const auto original = floors;
        merge_small_watershed_regions(floors, layout, &scene, {});
        CHECK(
            (layout.owners[22] == layout.owners[27])
            == (variant == 0 || variant == 4 || variant == 5)
        );
        for (unsigned a = 0; a < floors.size(); ++a) {
            CHECK(floors[a].neighbours == original[a].neighbours);
        }
    }
}

void geometric_islands_preserve_lighting_labels() {
    BakeScene geometry;
    quad(geometry, {-8, -4, 0}, {8, -4, 0}, {8, 4, 0}, {-8, 4, 0});
    quad(geometry, {-8, -4, 0}, {8, -4, 0}, {8, -4, 5}, {-8, -4, 5});
    quad(geometry, {-8, 4, 0}, {8, 4, 0}, {8, 4, 5}, {-8, 4, 5});
    Scene scene(geometry);
    for (unsigned variant = 0; variant < 3; ++variant) {
        std::vector<EnvironmentFloorSample> floors;
        for (int x = -7; x <= 7; ++x) {
            for (int y = -3; y <= 3; ++y) {
                EnvironmentFloorSample sample;
                sample.position = {float(x), float(y), 0};
                sample.region = x < 0 ? 0 : 1;
                sample.lighting_labels = std::vector<unsigned>{sample.region};
                sample.underwater = variant == 1 && x >= 0;
                floors.push_back(sample);
            }
        }
        for (unsigned a = 0; a < floors.size(); ++a) {
            for (unsigned b = 0; b < floors.size(); ++b) {
                const auto d = add(floors[a].position, mul(floors[b].position, -1));
                if (std::abs(d[0]) + std::abs(d[1]) == 1
                    && (variant != 2 || floors[a].region == floors[b].region)) {
                    floors[a].neighbours.push_back(b);
                }
            }
        }
        const auto layout = room_layout(scene, floors, {}, false, true);
        CHECK(layout.rooms.size() == (variant == 0 ? 1u : 2u));
        for (const auto& floor : floors) {
            CHECK(
                floor.lighting_labels
                && *floor.lighting_labels == std::vector<unsigned>{floor.region}
            );
        }
    }
}

void pitched_island_grids() {
    const ProbeFrame expected{{.8f, .6f}, {.96f, .28f}};
    std::vector<EnvironmentFloorSample> floors;
    RoomLayout layout;
    for (unsigned x = 0; x < 20; ++x) {
        for (unsigned y = 0; y < 3; ++y) {
            EnvironmentFloorSample floor;
            floor.position = expected.to_world({float(x) * 2, float(y) * 2, 10});
            floors.push_back(floor);
            layout.owners.push_back(0);
            layout.height_bounds.push_back({floor.position[2] + .15f, floor.position[2] + 4});
        }
    }
    const auto grids = coverage_probe_grids({}, floors, layout, {2, 2, 2});
    CHECK(!grids.empty());
    CHECK(grids.size() == 1 && grids.front().grid.probe_count() < 800);
    for (const auto& allocation : grids) {
        CHECK(std::abs(allocation.grid.frame.axis[1]) > .3f);
        CHECK(std::abs(allocation.grid.frame.pitch[1]) > .15f);
        CHECK(allocation.grid.probe_count() <= 4096);
        const auto p = allocation.grid.frame.to_world({3, 4, 5});
        const auto back = allocation.grid.frame.to_local(p);
        CHECK(std::abs(back[0] - 3) < .001f && std::abs(back[2] - 5) < .001f);
    }
    for (unsigned i = 0; i < floors.size(); ++i) {
        for (float z = layout.height_bounds[i][0]; z <= layout.height_bounds[i][1]; z += .25f) {
            auto p = floors[i].position;
            p[2] = z;
            CHECK(std::any_of(grids.begin(), grids.end(), [&](const auto& allocation) {
                return probe_boundary_weight(allocation.grid, p) > 0;
            }));
        }
    }
    const Float3 translation{1000.125f, -500.375f, 37.25f};
    for (unsigned i = 0; i < floors.size(); ++i) {
        floors[i].position = add(floors[i].position, translation);
        for (auto& height : layout.height_bounds[i]) {
            height += translation[2];
        }
    }
    const auto moved = coverage_probe_grids({}, floors, layout, {2, 2, 2});
    CHECK(moved.size() == grids.size());
    CHECK(moved.front().grid.probe_count() == grids.front().grid.probe_count());
}

int main() {
    geometric_islands_preserve_lighting_labels();
    pitched_island_grids();
    open_water_basin_regions();
    steep_underwater_receiver_ownership();
    underwater_edge_receiver_ownership();
    small_watershed_cleanup();
    neighbouring_islands_share_storage();
    island_column_budget();
    doorway();
    doorway(true);
    rotated_corridor();
    bent_medial_paths();
    diagonal_distance_field();
    curved_distance_field();
    ceiling_enclosure();
    field_domain();
    test_swimming_endpoint_clearance();
    return openrac::test::result();
}
