// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Open areas have hubs, passages retain routes, and disconnected entrance cuts retain loops.
#include <algorithm>
#include <functional>
#include <map>
#include <queue>
#include <set>

#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_backbone.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
RoomDistanceField field(const std::function<bool(int, int)>& inside) {
    RoomDistanceField result;
    std::map<std::pair<int, int>, unsigned> ids;
    for (int x = -26; x <= 26; ++x) {
        for (int y = -26; y <= 26; ++y) {
            if (inside(x, y)) {
                ids[{x, y}] = unsigned(result.nodes.size());
                RoomFieldNode node;
                node.position = {float(x), float(y), 0};
                node.wall.distance = INFINITY;
                result.nodes.push_back(node);
            }
        }
    }
    std::queue<unsigned> pending;
    for (const auto& [p, a] : ids) {
        for (auto d : {std::pair{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
            const auto found = ids.find({p.first + d.first, p.second + d.second});
            if (found != ids.end()) {
                result.nodes[a].neighbours.push_back(found->second);
            }
        }
        if (result.nodes[a].neighbours.size() < 4) {
            result.nodes[a].wall.distance = .5f;
            pending.push(a);
        }
    }
    while (!pending.empty()) {
        auto a = pending.front();
        pending.pop();
        for (auto b : result.nodes[a].neighbours) {
            if (result.nodes[b].wall.distance > result.nodes[a].wall.distance + 1) {
                result.nodes[b].wall.distance = result.nodes[a].wall.distance + 1;
                pending.push(b);
            }
        }
    }
    return result;
}

void meaningful_leaves(const RoomBackbone& graph) {
    const std::set<unsigned> hubs(graph.centres.begin(), graph.centres.end());
    for (unsigned a = 0; a < graph.skeleton.size(); ++a) {
        if (graph.skeleton[a].size() == 1) {
            CHECK(hubs.contains(a));
        }
    }
}

void open_area(const Scene& scene) {
    auto source = field([](int x, int y) {
        return (std::abs(x) <= 12 && std::abs(y) <= 10) || (x > 12 && x <= 15 && std::abs(y) <= 1);
    });
    const auto graph = room_backbone(source, scene, {});
    CHECK(graph.centres.size() == 1);
    CHECK(graph.entrances.empty());
    CHECK(std::all_of(graph.skeleton.begin(), graph.skeleton.end(), [](const auto& row) {
        return row.empty();
    }));
    const auto p = source.nodes[graph.centres.front()].position;
    CHECK(std::abs(p[0]) <= 3 && std::abs(p[1]) <= 1);
}

void short_tunnel(const Scene& scene) {
    auto source = field([](int x, int y) { return std::abs(x) <= 5 && std::abs(y) <= 1; });
    const auto graph = room_backbone(source, scene, {});
    meaningful_leaves(graph);
    CHECK(std::any_of(graph.skeleton.begin(), graph.skeleton.end(), [](const auto& row) {
        return !row.empty();
    }));
    for (float direction : {-1.f, 1.f}) {
        CHECK(std::any_of(graph.centres.begin(), graph.centres.end(), [&](auto a) {
            return source.nodes[a].position[0] * direction >= 3;
        }));
    }
}

void entrances_and_dead_ends(const Scene& scene) {
    auto source = field([](int x, int y) {
        return (std::abs(x) <= 8 && std::abs(y) <= 8) || (std::abs(x) <= 24 && std::abs(y) <= 2)
               || (std::abs(x) <= 2 && y > 8 && y <= 24);
    });
    const auto graph = room_backbone(source, scene, {});
    meaningful_leaves(graph);
    for (auto direction : {Float3{-1, 0, 0}, Float3{1, 0, 0}, Float3{0, 1, 0}}) {
        CHECK(std::any_of(graph.centres.begin(), graph.centres.end(), [&](auto a) {
            const auto p = source.nodes[a].position;
            return p[0] * direction[0] + p[1] * direction[1] > 18;
        }));
    }
    CHECK(
        std::count_if(
            graph.centres.begin(),
            graph.centres.end(),
            [&](auto a) {
                const auto p = source.nodes[a].position;
                return std::abs(p[0]) <= 8 && std::abs(p[1]) <= 8;
            }
        )
        == 1
    );
}

void separate_entrances_preserve_loop(const Scene& scene) {
    auto source = field([](int x, int y) {
        return ((x <= -5 || x >= 5) && std::abs(x) <= 24 && std::abs(y) <= 12)
               || (std::abs(x) < 5 && std::abs(std::abs(y) - 6) <= 1);
    });
    const auto graph = room_backbone(source, scene, {});
    meaningful_leaves(graph);
    unsigned vertices = 0, edges = 0;
    for (const auto& row : graph.skeleton) {
        if (!row.empty()) {
            ++vertices;
            edges += unsigned(row.size());
        }
    }
    CHECK(vertices > 0 && edges / 2 >= vertices);  // At least one retained cycle.
    CHECK(graph.entrances.size() >= 4);
}

void classification_is_not_a_room_boundary() {
    for (bool blocked : {false, true}) {
        auto source =
            field([](int x, int y) { return std::abs(x) <= 12 && std::abs(y) <= 10 && x != 0; });
        source.water_footprint = true;
        source.swimming_rooms = {false, true};
        source.submerged_rooms = {false, true};
        for (auto& node : source.nodes) {
            node.room = node.position[0] < 0 ? 1 : 0;
            node.wall.distance =
                std::min(12.5f - std::abs(node.position[0]), 10.5f - std::abs(node.position[1]));
            if (blocked) {
                node.wall.distance = std::min(node.wall.distance, std::abs(node.position[0]));
            }
            source.domain[{node.room, int(node.position[0]), int(node.position[1])}] = {0};
        }
        BakeScene geometry;
        const auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle first, second;
            first.vertices = {a, b, c};
            second.vertices = {a, c, d};
            geometry.triangles.push_back(first);
            geometry.triangles.push_back(second);
        };
        quad({-20, -20, 0}, {20, -20, 0}, {20, 20, 0}, {-20, 20, 0});
        if (blocked) {
            quad({0, -20, 0}, {0, 20, 0}, {0, 20, 4}, {0, -20, 4});
        }
        Scene scene(geometry);
        const auto graph = room_backbone(source, scene, {});
        CHECK(graph.centres.size() == (blocked ? 2u : 1u));
        CHECK(graph.entrances.empty());
    }
}

void water_area_has_a_shore_entrance() {
    // Level shore, raised bank, separating wall, excessive bank height, low ceiling.
    for (unsigned variant = 0; variant < 5; ++variant) {
        auto source =
            field([](int x, int y) { return std::abs(x) <= 24 && std::abs(y) <= 16 && x != 0; });
        source.water_footprint = true;
        source.swimming_rooms = {false, true};
        source.submerged_rooms = {false, false};
        const float bank = variant == 0 ? 0 : variant == 3 ? 2.5f : .9f;
        for (auto& node : source.nodes) {
            node.room = node.position[0] > 0 ? 1 : 0;
            node.position[2] = node.room == 0 ? bank : 0;
            source.domain[{node.room, int(node.position[0]), int(node.position[1])}] = {
                node.position[2]
            };
        }
        BakeScene geometry;
        const auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle first, second;
            first.vertices = {a, b, c};
            second.vertices = {a, c, d};
            geometry.triangles.push_back(first);
            geometry.triangles.push_back(second);
        };
        quad({-30, -20, bank}, {.1f, -20, bank}, {.1f, 20, bank}, {-30, 20, bank});
        quad({.1f, -20, -4}, {30, -20, -4}, {30, 20, -4}, {.1f, 20, -4});
        quad({.1f, -20, -4}, {.1f, 20, -4}, {.1f, 20, bank}, {.1f, -20, bank});
        if (variant == 2) {
            quad({.1f, -20, bank}, {.1f, 20, bank}, {.1f, 20, 5}, {.1f, -20, 5});
        }
        if (variant == 4) {
            quad({.1f, -20, 2.5f}, {30, -20, 2.5f}, {30, 20, 2.5f}, {.1f, 20, 2.5f});
        }
        Scene scene(geometry);
        const auto graph = room_backbone(source, scene, {});
        CHECK(graph.centres.size() == 2);
        bool entrance = false;
        for (unsigned a = 0; a < graph.skeleton.size(); ++a) {
            for (auto b : graph.skeleton[a]) {
                entrance |= source.nodes[a].room != source.nodes[b].room;
            }
        }
        CHECK(entrance == (variant <= 1));
        CHECK(graph.entrances.size() == (variant <= 1 ? 2 : 0));
        meaningful_leaves(graph);
    }
}

void submerged_tunnel_entrance() {
    // A submerged basin can lie more than a walking step below a tunnel mouth.
    // Reject the same link through a wall, a dry mask gap, or insufficient water depth.
    for (unsigned variant = 0; variant < 4; ++variant) {
        auto source =
            field([](int x, int y) { return std::abs(x) <= 4 && std::abs(y) <= 4 && x != 0; });
        source.water_footprint = true;
        source.swimming_rooms = {false, true, true};
        source.submerged_rooms = {false, true, false};
        for (auto& node : source.nodes) {
            node.room = node.position[0] < 0 ? 0 : 1;
            node.position[2] = node.room == 0 ? 2.7f : 0;
            source.domain[{node.room, int(node.position[0]), int(node.position[1])}] = {
                node.position[2]
            };
        }
        for (int x = -4; x <= 4; ++x) {
            for (int y = -4; y <= 4; ++y) {
                if (variant != 2 || x != 0) {
                    source.domain[{2, x, y}] = {variant == 3 ? 4.f : 9.f};
                }
            }
        }
        BakeScene geometry;
        const auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle first, second;
            first.vertices = {a, b, c};
            second.vertices = {a, c, d};
            geometry.triangles.push_back(first);
            geometry.triangles.push_back(second);
        };
        quad({-6, -6, 2.7f}, {-.4f, -6, 2.7f}, {-.4f, 6, 2.7f}, {-6, 6, 2.7f});
        quad({-.4f, -6, 0}, {6, -6, 0}, {6, 6, 0}, {-.4f, 6, 0});
        quad({-.4f, -6, 0}, {-.4f, 6, 0}, {-.4f, 6, 2.7f}, {-.4f, -6, 2.7f});
        if (variant == 1) {
            quad({0, -6, 0}, {0, 6, 0}, {0, 6, 10}, {0, -6, 10});
        }
        Scene scene(geometry);
        const auto graph = room_backbone(source, scene, {});
        CHECK(graph.centres.size() == (variant == 0 ? 1u : 2u));
        meaningful_leaves(graph);
    }
}

void sampled_floor_gap() {
    // Flat gap, separating wall, missing floor, legal step, excessive step, wall detour.
    // Supported links may cross underwater tags; a floating water surface is not floor.
    for (unsigned variant = 0; variant < 8; ++variant) {
        auto source =
            field([](int x, int y) { return y == 0 && (x == -3 || x == -2 || x == 2 || x == 3); });
        source.water_footprint = true;
        source.swimming_rooms = {false, variant >= 6};
        source.submerged_rooms = {false, variant == 6};
        const float rise = variant == 3 ? 1.5f : variant == 4 ? 3.f : 0.f;
        for (auto& node : source.nodes) {
            node.wall.distance = 2;
            node.room = variant >= 6 && node.position[0] > 0 ? 1 : 0;
            node.position[2] = node.position[0] > 0 ? rise : 0;
            source.domain[{node.room, int(node.position[0]), 0}] = {node.position[2]};
        }
        const auto initial = source.nodes.size();
        BakeScene geometry;
        const auto quad = [&](Float3 a, Float3 b, Float3 c, Float3 d) {
            BakeTriangle first, second;
            first.vertices = {a, b, c};
            second.vertices = {a, c, d};
            geometry.triangles.push_back(first);
            geometry.triangles.push_back(second);
        };
        if (variant == 2) {
            quad({-6, -3, 0}, {-.5f, -3, 0}, {-.5f, 3, 0}, {-6, 3, 0});
            quad({.5f, -3, 0}, {6, -3, 0}, {6, 3, 0}, {.5f, 3, 0});
        } else if (rise > 0) {
            quad({-6, -3, 0}, {0, -3, 0}, {0, 3, 0}, {-6, 3, 0});
            quad({0, -3, rise}, {6, -3, rise}, {6, 3, rise}, {0, 3, rise});
            quad({0, -3, 0}, {0, 3, 0}, {0, 3, rise}, {0, -3, rise});
        } else {
            quad({-6, -3, 0}, {6, -3, 0}, {6, 3, 0}, {-6, 3, 0});
        }
        if (variant == 1) {
            quad({0, -3, -1}, {0, 3, -1}, {0, 3, 4}, {0, -3, 4});
        }
        if (variant == 5) {
            quad({0, -.5f, -1}, {0, .5f, -1}, {0, .5f, 4}, {0, -.5f, 4});
        }
        Scene scene(geometry);
        repair_backbone_floor_gaps(source, scene, {});
        const bool connected = variant == 0 || variant == 3 || variant == 5 || variant == 6;
        CHECK((source.nodes.size() > initial) == connected);
        std::vector<bool> seen(source.nodes.size());
        std::vector<unsigned> pending{0};
        seen[0] = true;
        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto b : source.nodes[pending[i]].neighbours) {
                if (!seen[b]) {
                    seen[b] = true;
                    pending.push_back(b);
                }
            }
        }
        CHECK(std::all_of(seen.begin(), seen.end(), [](bool v) { return v; }) == connected);
    }
}
}  // namespace

int main() {
    BakeScene geometry;
    Scene scene(geometry);
    open_area(scene);
    short_tunnel(scene);
    entrances_and_dead_ends(scene);
    separate_entrances_preserve_loop(scene);
    classification_is_not_a_room_boundary();
    water_area_has_a_shore_entrance();
    submerged_tunnel_entrance();
    sampled_floor_gap();
    return openrac::test::result();
}
