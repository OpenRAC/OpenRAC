// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Synthetic platforms and a four-unit-deep basin test traversal and reflection-medium boundaries.
#include <algorithm>

#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_placement.h"
#include "enhanced/lighting/environment_placement_internal.h"
#include "enhanced/lighting/room_distance_field.h"
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

// Undirected medial edges must keep the same body clearance in either traversal direction.
void test_body_path_is_bidirectional() {
    BakeSettings settings;
    BakeScene source;
    quad(source, {-3, -3, 0}, {3, -3, 0}, {3, 3, 0}, {-3, 3, 0});
    const Float3 a{0, 0, 0}, b{1, 0, 0};

    {
        Scene clear(source);

        for (bool swimming : {false, true}) {
            CHECK(room_body_path_clear(clear, a, b, settings, swimming));
            CHECK(room_body_path_clear(clear, b, a, settings, swimming));
        }
    }

    // A wall in the endpoint overlap is closer than ray_bias to the reverse ray's origin.
    // It must block the undirected field edge, whichever end the skeleton traverses first.
    const float wall = b[0] + .05f - settings.ray_bias * .5f;
    quad(source, {wall, -2, 0}, {wall, 2, 0}, {wall, 2, 4}, {wall, -2, 4});
    Scene blocked(source);

    for (bool swimming : {false, true}) {
        CHECK(!room_body_path_clear(blocked, a, b, settings, swimming));
        CHECK(!room_body_path_clear(blocked, b, a, settings, swimming));
    }
}

// A sloping support surface must not lose alternate field columns to the narrow smoothing window.
void test_medial_field_follows_slope() {
    BakeScene source;
    quad(source, {-8, -3, -7.2f}, {8, -3, 7.2f}, {8, 3, 7.2f}, {-8, 3, -7.2f});
    std::vector<EnvironmentFloorSample> floors(7);
    RoomLayout layout;
    layout.rooms.resize(1);
    for (unsigned i = 0; i < floors.size(); ++i) {
        const float x = -6.0625f + float(i) * 2;
        floors[i].position = {x, 0, .9f * x};
        if (i) {
            floors[i].neighbours.push_back(i - 1);
            floors[i - 1].neighbours.push_back(i);
        }
        layout.rooms[0].push_back(i);
        layout.owners.push_back(0);
        layout.height_bounds.push_back({.9f * x - .8f, .9f * x + 3.8f});
    }
    Scene scene(source);
    const auto field = room_distance_field(scene, floors, layout, {});
    CHECK(!field.nodes.empty());
    if (field.nodes.empty()) {
        return;
    }
    std::vector<bool> seen(field.nodes.size());
    std::vector<unsigned> pending{0};
    seen[0] = true;
    for (unsigned i = 0; i < pending.size(); ++i) {
        for (auto b : field.nodes[pending[i]].neighbours) {
            if (!seen[b]) {
                seen[b] = true;
                pending.push_back(b);
            }
        }
    }
    CHECK(std::all_of(seen.begin(), seen.end(), [](bool value) { return value; }));
    CHECK(std::any_of(field.nodes.begin(), field.nodes.end(), [](const auto& node) {
        return node.position[0] == -7;
    }));
}

// Player-radius checks must follow rounded/sloping floor contact rather than starting
// inside the side of a pipe. Real low ceilings and side walls still reject the route.
void test_medial_field_follows_pipe_floor() {
    for (unsigned variant = 0; variant < 4; ++variant) {
        BakeScene source;
        if (variant == 3) {
            // A steeply rising tube still has a walkable centre. Radius contact can
            // meet steeper curved side triangles within the bounded floor-rise window.
            const float y[]{-2, -.6f, -.4f, .4f, .6f, 2};
            const float z[]{2, .4f, 0, 0, .4f, 2};
            for (unsigned i = 1; i < 6; ++i) {
                quad(
                    source,
                    {-8, y[i - 1], z[i - 1] - 6.4f},
                    {8, y[i - 1], z[i - 1] + 6.4f},
                    {8, y[i], z[i] + 6.4f},
                    {-8, y[i], z[i] - 6.4f}
                );
            }
        } else {
            quad(source, {-8, -2, 2}, {8, -2, 2}, {8, 0, 0}, {-8, 0, 0});
            quad(source, {-8, 0, 0}, {8, 0, 0}, {8, 2, 2}, {-8, 2, 2});
        }
        if (variant == 1) {
            quad(source, {-8, -2, 2}, {-8, 2, 2}, {8, 2, 2}, {8, -2, 2});
        }
        if (variant == 2) {
            quad(source, {-8, .3f, 0}, {8, .3f, 0}, {8, .3f, 4}, {-8, .3f, 4});
        }
        Scene scene(source);
        RoomLayout layout;
        layout.medial_water_footprint = true;
        layout.medial_roots.emplace();
        layout.rooms.resize(1);
        std::vector<EnvironmentFloorSample> floors(7);
        for (unsigned i = 0; i < floors.size(); ++i) {
            floors[i].position = {-6.f + float(i) * 2, 0, 0};
            if (variant == 3) {
                floors[i].position[2] = floors[i].position[0] * .8f;
            }
            layout.rooms[0].push_back(i);
            layout.owners.push_back(0);
            layout.medial_roots->push_back(i);
            if (i) {
                floors[i].neighbours.push_back(i - 1);
                floors[i - 1].neighbours.push_back(i);
            }
        }
        const auto field = room_distance_field(scene, floors, layout, {});
        CHECK(field.nodes.empty() == (variant == 1 || variant == 2));
        if (variant == 0 || variant == 3) {
            CHECK(std::any_of(field.nodes.begin(), field.nodes.end(), [](const auto& n) {
                return n.position[0] < -5 && !n.neighbours.empty();
            }));
            CHECK(std::any_of(field.nodes.begin(), field.nodes.end(), [](const auto& n) {
                return n.position[0] > 5 && !n.neighbours.empty();
            }));
            if (!field.nodes.empty()) {
                std::vector<bool> seen(field.nodes.size());
                std::vector<unsigned> pending{0};
                seen[0] = true;
                for (unsigned i = 0; i < pending.size(); ++i) {
                    for (auto next : field.nodes[pending[i]].neighbours) {
                        if (!seen[next]) {
                            seen[next] = true;
                            pending.push_back(next);
                        }
                    }
                }
                CHECK(std::all_of(seen.begin(), seen.end(), [](bool v) { return v; }));
            }
        }
    }
}

// The water axis follows its authored footprint below overhead obstacles, but actual
// waterline walls still block it. Empty/dry cells also keep separate pools disconnected.
void test_water_footprint_axis() {
    BakeScene source;
    quad(source, {0, -4, 2}, {0, 4, 2}, {0, 4, 4}, {0, -4, 4});
    Scene scene(source);
    for (bool gap : {false, true}) {
        std::vector<EnvironmentFloorSample> floors;
        RoomLayout layout;
        layout.medial_water_footprint = true;
        layout.rooms.resize(1);
        for (int x = -8; x <= 8; x += 2) {
            if (gap && std::abs(x) < 4) {
                continue;
            }
            for (int y = -2; y <= 2; y += 2) {
                EnvironmentFloorSample floor;
                floor.position = {float(x), float(y), float(x) * .001f};
                floor.swimming_surface = true;
                floor.water_height = floor.position[2];
                layout.rooms[0].push_back(unsigned(floors.size()));
                layout.owners.push_back(0);
                floors.push_back(floor);
            }
        }
        const auto components = [](const RoomDistanceField& field) {
            std::vector<bool> seen(field.nodes.size());
            unsigned count = 0;
            for (unsigned root = 0; root < field.nodes.size(); ++root) {
                if (seen[root]) {
                    continue;
                }
                ++count;
                seen[root] = true;
                std::vector<unsigned> pending{root};
                for (unsigned i = 0; i < pending.size(); ++i) {
                    for (auto b : field.nodes[pending[i]].neighbours) {
                        if (!seen[b]) {
                            seen[b] = true;
                            pending.push_back(b);
                        }
                    }
                }
            }
            return count;
        };
        CHECK(components(room_distance_field(scene, floors, layout, {})) == (gap ? 2u : 1u));
        auto blocked_source = source;
        quad(blocked_source, {0, -4, -1}, {0, 4, -1}, {0, 4, .5f}, {0, -4, .5f});
        Scene blocked(blocked_source);
        CHECK(components(room_distance_field(blocked, floors, layout, {})) >= 2);
        auto low_source = source;
        quad(low_source, {0, -4, .5f}, {0, 4, .5f}, {0, 4, 2}, {0, -4, 2});
        Scene low(low_source);
        CHECK(components(room_distance_field(low, floors, layout, {})) >= 2);
        // Generated water anchors cannot authorize the other side of a collision barrier.
        for (auto& floor : floors) {
            floor.anchor = true;
        }
        layout.medial_roots = std::vector<unsigned>{0};
        const auto reachable = room_distance_field(blocked, floors, layout, {});
        CHECK(!reachable.nodes.empty());
        CHECK(std::all_of(reachable.nodes.begin(), reachable.nodes.end(), [](const auto& node) {
            return node.position[0] < 0;
        }));
        layout.medial_roots = std::vector<unsigned>{};
        CHECK(room_distance_field(scene, floors, layout, {}).nodes.empty());
        layout.medial_roots.reset();
        layout.medial_water_footprint = false;
        CHECK(components(room_distance_field(low, floors, layout, {})) >= 2);
    }
}

// Short holes between coalesced basin-bed lattices are swimming space, provided body clearance
// passes. Wider missing water evidence and actual collision barriers must still split the field.
void test_water_axis_requires_entry() {
    for (bool blocked : {false, true}) {
        BakeScene source;
        quad(source, {-10, -4, 0}, {0, -4, 0}, {0, 4, 0}, {-10, 4, 0});
        if (blocked) {
            quad(source, {.5f, -6, -1}, {.5f, 6, -1}, {.5f, 6, 4}, {.5f, -6, 4});
        }
        Scene scene(source);
        RoomLayout layout;
        layout.medial_water_footprint = true;
        layout.medial_roots.emplace();
        layout.rooms.resize(2);
        std::vector<EnvironmentFloorSample> floors;
        for (int x = -8; x <= 8; x += 2) {
            for (int y = -2; y <= 2; y += 2) {
                EnvironmentFloorSample floor;
                floor.position = {float(x), float(y), 0};
                floor.swimming_surface = x >= 2;
                floor.anchor = true;  // Generated water anchors must not seed the flood.
                if (floor.swimming_surface) {
                    floor.water_height = 0;
                } else {
                    layout.medial_roots->push_back(unsigned(floors.size()));
                }
                const unsigned room = floor.swimming_surface ? 1 : 0;
                layout.rooms[room].push_back(unsigned(floors.size()));
                layout.owners.push_back(room);
                floors.push_back(floor);
            }
        }
        const auto field = room_distance_field(scene, floors, layout, {});
        CHECK(!field.nodes.empty());
        const bool has_water =
            std::any_of(field.nodes.begin(), field.nodes.end(), [](const auto& n) {
                return n.room == 1;
            });
        CHECK(has_water == !blocked);
    }
}

void test_submerged_lattice_gap() {
    for (unsigned variant = 0; variant < 3; ++variant) {
        BakeScene source;
        quad(source, {-2, -3, 0}, {8, -3, 0}, {8, 3, 0}, {-2, 3, 0});
        if (variant == 2) {
            quad(source, {2, -3, 0}, {2, 3, 0}, {2, 3, 4}, {2, -3, 4});
        }
        Scene scene(source);
        std::vector<EnvironmentFloorSample> floors(2);
        floors[1].position[0] = variant == 1 ? 6.f : 4.f;
        for (auto& floor : floors) {
            floor.underwater = true;
            floor.swimming_surface = true;
            floor.water_height = 4;
        }
        RoomLayout layout;
        layout.medial_water_footprint = true;
        layout.owners = {0, 0};
        layout.rooms = {{0, 1}};
        const auto field = room_distance_field(scene, floors, layout, {});
        CHECK(!field.nodes.empty());
        if (field.nodes.empty()) {
            continue;
        }
        std::vector<bool> seen(field.nodes.size());
        std::vector<unsigned> pending{0};
        seen[0] = true;
        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto b : field.nodes[pending[i]].neighbours) {
                if (!seen[b]) {
                    seen[b] = true;
                    pending.push_back(b);
                }
            }
        }
        const bool connected = std::all_of(seen.begin(), seen.end(), [](bool v) { return v; });
        CHECK(connected == (variant == 0));
        layout.medial_water_footprint = false;
        const auto walking_domain = room_distance_field(scene, floors, layout, {});
        CHECK(!walking_domain.connects(floors[0].position, floors[1].position, 0));
    }
}

void test_water_axis_requires_clear_ascent() {
    for (unsigned variant = 0; variant < 3; ++variant) {
        BakeScene source;
        if (variant == 1) {
            quad(source, {-4, -4, 2}, {4, -4, 2}, {4, 4, 2}, {-4, 4, 2});
        }
        Scene scene(source);
        std::vector<EnvironmentFloorSample> floors(2);
        for (auto& floor : floors) {
            floor.anchor = true;
            floor.swimming_surface = true;
            floor.water_height = 4;
        }
        floors[0].underwater = true;
        floors[1].position[2] = 4;
        if (variant == 2) {
            floors[0].water_height.reset();  // Proximity alone cannot prove a water column.
        }
        RoomLayout layout;
        layout.medial_water_footprint = true;
        layout.medial_roots = std::vector<unsigned>{0};
        layout.rooms = {{0}, {1}};
        layout.owners = {0, 1};
        const auto field = room_distance_field(scene, floors, layout, {});
        const bool surface = std::any_of(field.nodes.begin(), field.nodes.end(), [](const auto& n) {
            return n.room == 1;
        });
        CHECK(surface == (variant == 0));
    }
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
        == 20
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
    CHECK(std::any_of(report.receivers.begin(), report.receivers.end(), [](const auto& r) {
        return r.water_surface;
    }));
    for (const auto& r : report.receivers) {
        CHECK(r.water_surface == (r.position[2] > 0 && r.position[0] >= 0));
    }
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

// Swimming can authorize a supported basin floor without authorizing sealed rooms or distant pools.
void test_swimming_reaches_supported_basin() {
    for (bool blocked : {false, true}) {
        BakeScene source;
        auto triangles = basin(source);
        if (blocked) {
            const auto first = source.triangles.size();
            quad(source, {1, -2, -5}, {1, 10, -5}, {1, 10, 3}, {1, -2, 3});
            for (unsigned i = unsigned(first); i < source.triangles.size(); ++i) {
                triangles.push_back({source.triangles[i].vertices, false});
            }
        }
        // This large triangle's 0.4-unit rise is a gentle slope, not a waterfall.
        for (auto& triangle : triangles) {
            if (triangle.swimmable) {
                for (auto& p : triangle.vertices) {
                    p[2] += p[0] * .05f;
                }
            }
        }
        const auto pool = triangles;
        for (auto triangle : pool) {
            if (!triangle.swimmable
                && std::any_of(triangle.vertices.begin(), triangle.vertices.end(), [](auto p) {
                       return p[2] == 0;
                   })) {
                continue;  // Leave a real dry gap between pools.
            }
            if (triangle.swimmable) {
                for (auto& p : triangle.vertices) {
                    p[2] += .4f;
                }
            }
            for (auto& p : triangle.vertices) {
                p[0] += 12;
            }
            triangles.push_back(triangle);  // No reachable entry to this separate pool.
        }
        EnvironmentFloorSample bank, bed, cellar;
        bank.position = {-2, 4, 0};
        bank.anchor = true;
        bed.position = {4, 4, -4};
        bed.region = 1;
        cellar.position = {4, 4, -8};
        cellar.region = 2;
        std::vector<EnvironmentFloorSample> floors{bank, bed, cellar};
        append_swimming_surfaces(triangles, floors);
        CHECK(std::none_of(floors.begin(), floors.end(), [](const auto& floor) {
            return floor.position[0] > 10 || floor.position[2] < -4.1f;
        }));
        CHECK(
            std::any_of(
                floors.begin(),
                floors.end(),
                [](const auto& floor) { return floor.position == Float3{4, 4, -4}; }
            )
            == !blocked
        );
        CHECK(
            std::any_of(
                floors.begin(),
                floors.end(),
                [](const auto& floor) {
                    return floor.swimming_surface && !floor.underwater && !floor.anchor;
                }
            )
            == !blocked
        );
    }
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
    // Four clear bed columns plus the bank; the shared boundary intersects solid shoreline.
    CHECK(floors.size() == 21);

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
        == 20
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
        == 10
    );  // Two clear wet columns; the shoreline column intersects the solid bank.
}

// Underwater floors can extend just beyond the visible surface outline under a bank.
void test_underwater_floor_mask_gap() {
    for (unsigned variant = 0; variant < 4; ++variant) {
        BakeScene source;
        auto triangles = basin(source);
        quad(source, {8, 0, -4}, {12, 0, -4}, {12, 8, -4}, {8, 8, -4});
        const auto t = source.triangles.size();
        triangles.push_back({source.triangles[t - 2].vertices, false});
        triangles.push_back({source.triangles[t - 1].vertices, false});
        if (variant == 1) {
            quad(source, {8.5f, 0, -4}, {8.5f, 8, -4}, {8.5f, 8, 1}, {8.5f, 0, 1});
            triangles.push_back({source.triangles[source.triangles.size() - 2].vertices, false});
            triangles.push_back({source.triangles.back().vertices, false});
        }
        std::vector<EnvironmentFloorSample> floors(2);
        floors[0].position = {6, 4, -4};
        floors[1].position = {9, 4, variant == 2 ? -.5f : -4};
        for (unsigned a = 0; a < 2; ++a) {
            floors[a].anchor = true;
            if (variant != 3) {
                floors[a].neighbours = {1 - a};
            }
        }
        append_swimming_surfaces(triangles, floors);
        CHECK(floors[0].underwater);
        CHECK(floors[1].underwater == (variant == 0));
        if (variant == 0) {
            CHECK(floors[1].water_height && std::abs(*floors[1].water_height) < .001f);
            CHECK(floors[1].water_bottom && *floors[1].water_bottom == -4);
        }
    }
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

void test_existing_bed_is_not_duplicated() {
    BakeScene source;
    const auto triangles = basin(source);
    EnvironmentFloorSample bank;
    bank.position = {-2, 4, 0};
    bank.anchor = true;
    EnvironmentFloorSample bed;
    bed.position = {4, 4, -4};
    bed.anchor = true;
    std::vector<EnvironmentFloorSample> floors{bank, bed};
    append_swimming_surfaces(triangles, floors);
    CHECK(floors[1].underwater);
    CHECK(floors[1].height_bounds && (*floors[1].height_bounds)[1] == -.25f);
    CHECK(
        std::count_if(
            floors.begin(),
            floors.end(),
            [](const auto& floor) {
                return floor.underwater && floor.position == Float3({4, 4, -4});
            }
        )
        == 1
    );
}

int main() {
    test_existing_bed_is_not_duplicated();
    test_jumpable_platforms();
    test_body_path_is_bidirectional();
    test_medial_field_follows_slope();
    test_medial_field_follows_pipe_floor();
    test_water_footprint_axis();
    test_water_axis_requires_entry();
    test_water_axis_requires_clear_ascent();
    test_submerged_lattice_gap();
    test_swimming_reflection_layers();
    test_only_authored_water_is_gameplay();
    test_swimming_reaches_supported_basin();
    test_wading_uses_the_basin_floor();
    test_water_coastline_and_sealed_room();
    test_underwater_floor_mask_gap();
    test_waterline_is_not_walking_ground();
    test_refined_waterline();
    return openrac::test::result();
}
