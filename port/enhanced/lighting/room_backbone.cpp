// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Condense reachable clearance space into area centres and distinct entrance routes.
#include "enhanced/lighting/room_backbone.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <queue>
#include <set>

#include "enhanced/lighting/bake_math.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned missing = ~0u;
using Edge = std::array<unsigned, 2>;
using Graph = std::vector<std::vector<unsigned>>;
using Entry = std::pair<float, unsigned>;
using Queue = std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>>;

float distance(Float3 a, Float3 b) {
    const auto d = add(a, mul(b, -1));
    return std::sqrt(dot(d, d));
}

float width(const RoomDistanceField& field, unsigned a) {
    return std::clamp(field.nodes[a].wall.distance, .25f, 32.f);
}

float weight(const RoomDistanceField& field, unsigned a, unsigned b) {
    const float w = std::min(width(field, a), width(field, b));
    return std::max(.001f, distance(field.nodes[a].position, field.nodes[b].position))
           * (1 + 4 / (w * w)) * (field.nodes[a].ridge && field.nodes[b].ridge ? .65f : 1.f);
}

struct Components {
    std::vector<unsigned> parent;

    explicit Components(unsigned n) : parent(n) { std::iota(parent.begin(), parent.end(), 0u); }

    unsigned root(unsigned a) {
        while (parent[a] != a) {
            parent[a] = parent[parent[a]];
            a = parent[a];
        }
        return a;
    }

    void join(unsigned a, unsigned b) { parent[root(b)] = root(a); }
};

// Leave the water vertically before crossing the bank. Sweeping the submerged feet
// diagonally to land strikes the riser even when the bank is a legal, clear step.
bool shore_step_clear(
    const Scene& scene, Float3 land, Float3 water, Float3 immersed, const BakeSettings& settings
) {
    if (land[2] <= water[2] || land[2] - water[2] > 2) {
        return false;
    }
    auto raised = water;
    raised[2] = land[2];
    return room_body_path_clear(scene, immersed, raised, settings, true)
           && room_body_path_clear(scene, raised, immersed, settings, true)
           && room_body_path_clear(scene, raised, land, settings)
           && room_body_path_clear(scene, land, raised, settings);
}

// A dry-tagged tunnel can open below a pool's surface. Its basin floor need not be
// within walking-step height; require a clear swimmer and a continuous wet approach.
bool submerged_entry_clear(
    const RoomDistanceField& field,
    const Scene& scene,
    Float3 dry,
    Float3 submerged,
    const BakeSettings& settings
) {
    if (!room_body_path_clear(scene, dry, submerged, settings)
        || !room_body_path_clear(scene, submerged, dry, settings)) {
        return false;
    }
    for (unsigned room = 0; room < field.swimming_rooms.size(); ++room) {
        if (!field.swimming_rooms[room] || field.submerged_rooms[room]) {
            continue;
        }
        const auto cell = field.domain.find(
            {room,
             int(std::round(submerged[0] / field.step)),
             int(std::round(submerged[1] / field.step))}
        );
        if (cell == field.domain.end()) {
            continue;
        }
        for (float surface : cell->second) {
            if (surface
                < std::max(dry[2], submerged[2]) + settings.traversal.player_height + .06f) {
                continue;
            }
            const auto delta = add(submerged, mul(dry, -1));
            const unsigned steps =
                std::max(1u, unsigned(std::ceil(distance(dry, submerged) / .25f)));
            bool continuous = true, entered = false;
            for (unsigned i = 0; i <= steps; ++i) {
                const auto p = add(dry, mul(delta, float(i) / float(steps)));
                if (field.contains({p[0], p[1], surface}, room)) {
                    entered = true;
                    continue;
                }
                // Only the supported tunnel approach can precede the water. Do not
                // bridge a dry hole between disconnected pools or between storeys.
                const auto floor = room_floor_position(scene, p, settings);
                if (entered || !floor || std::abs((*floor)[2] - p[2]) > .75f) {
                    continuous = false;
                    break;
                }
            }
            if (continuous && entered) {
                return true;
            }
        }
    }
    return false;
}

Graph routing_graph(
    const RoomDistanceField& field, const Scene& scene, const BakeSettings& settings
) {
    Graph graph;
    for (const auto& node : field.nodes) {
        graph.push_back(node.neighbours);
    }
    if (!field.water_footprint) {
        return graph;
    }
    Components components(unsigned(graph.size()));
    for (unsigned a = 0; a < graph.size(); ++a) {
        for (auto b : graph[a]) {
            components.join(a, b);
        }
    }
    const auto top_water = [&](unsigned room) {
        return field.swimming_rooms[room] && !field.submerged_rooms[room];
    };
    // The reflection medium is not a physical room boundary. Rejoin supported floor at
    // shorelines and short basin sampling seams before choosing hubs or entrances.
    constexpr float range = 4;
    std::map<std::array<int, 2>, std::vector<unsigned>> bins;
    for (unsigned a = 0; a < graph.size(); ++a) {
        const auto& node = field.nodes[a];
        const auto p = node.position;
        const std::array<int, 2> bin{int(std::floor(p[0] / range)), int(std::floor(p[1] / range))};
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                const auto found = bins.find({bin[0] + x, bin[1] + y});
                if (found == bins.end()) {
                    continue;
                }
                for (auto b : found->second) {
                    const auto& other = field.nodes[b];
                    if (top_water(node.room) && top_water(other.room)) {
                        continue;
                    }
                    if (node.room == other.room && components.root(a) == components.root(b)) {
                        continue;
                    }
                    const auto q = other.position;
                    if (!top_water(node.room) && !top_water(other.room)
                        && field.submerged_rooms[node.room] != field.submerged_rooms[other.room]) {
                        const auto dry = field.submerged_rooms[node.room] ? q : p;
                        const auto wet = field.submerged_rooms[node.room] ? p : q;
                        if (distance(p, q) <= range
                            && submerged_entry_clear(field, scene, dry, wet, settings)) {
                            graph[a].push_back(b);
                            graph[b].push_back(a);
                            continue;
                        }
                    }
                    if (std::abs(p[2] - q[2]) > 2 || distance(p, q) > 2.5f) {
                        continue;
                    }
                    if (top_water(node.room) || top_water(other.room)) {
                        const auto& water = top_water(node.room) ? node : other;
                        const auto& land = top_water(node.room) ? other : node;
                        if (field.submerged_rooms[land.room]
                            || land.position[2] < water.position[2] - .5f
                            || land.position[2] > water.position[2] + 2) {
                            continue;
                        }
                        const auto d = add(water.position, mul(land.position, -1));
                        const unsigned steps =
                            std::max(1u, unsigned(std::ceil(std::hypot(d[0], d[1]) / .5f)));
                        bool supported = true;
                        for (unsigned i = 0; i <= steps; ++i) {
                            const auto point = add(land.position, mul(d, float(i) / float(steps)));
                            if (field.contains(
                                    {point[0], point[1], water.position[2]}, water.room
                                )) {
                                continue;
                            }
                            const auto floor = room_floor_position(scene, point, settings);
                            if (!floor || std::abs((*floor)[2] - point[2]) > .75f) {
                                supported = false;
                                break;
                            }
                        }
                        auto immersed = water.position;
                        immersed[2] -= std::min(
                            settings.traversal.player_height * .25f,
                            settings.traversal.minimum_swim_depth - .06f
                        );
                        if (!supported
                            || !room_supported_path_clear(
                                scene, land.position, land.position, settings
                            )
                            || !room_body_path_clear(
                                scene, land.position, water.position, settings, true
                            )
                            || (!room_body_path_clear(
                                    scene, land.position, immersed, settings, true
                                )
                                && !shore_step_clear(
                                    scene, land.position, water.position, immersed, settings
                                ))) {
                            continue;
                        }
                        graph[a].push_back(b);
                        graph[b].push_back(a);
                        continue;
                    }
                    if (node.room == other.room
                        && (!field.connects(p, q, node.room) || !field.connects(q, p, node.room))) {
                        continue;
                    }
                    if (!room_supported_path_clear(scene, p, q, settings)
                        || !room_supported_path_clear(scene, q, p, settings)) {
                        continue;
                    }
                    graph[a].push_back(b);
                    graph[b].push_back(a);
                }
            }
        }
        bins[bin].push_back(a);
    }
    for (auto& row : graph) {
        std::sort(row.begin(), row.end());
        row.erase(std::unique(row.begin(), row.end()), row.end());
    }
    return graph;
}

// A broad two-dimensional core is a room. A thin straight, curved or ring-shaped core
// remains a passage: covariance trace measures its spread without assuming a straight axis.
float compactness(const RoomDistanceField& field, std::span<const unsigned> nodes) {
    if (nodes.empty()) {
        return 0;
    }
    Float3 mean{};
    for (auto a : nodes) {
        mean = add(mean, field.nodes[a].position);
    }
    mean = mul(mean, 1.f / float(nodes.size()));
    float spread = 0;
    for (auto a : nodes) {
        const auto d = add(field.nodes[a].position, mul(mean, -1));
        spread += dot(d, d);
    }
    spread /= float(nodes.size());
    return float(nodes.size()) * field.step * field.step
           / std::max(field.step * field.step, 3.14159265f * spread);
}

unsigned centre(const RoomDistanceField& field, std::span<const unsigned> nodes) {
    Float3 mean{};
    for (auto a : nodes) {
        mean = add(mean, field.nodes[a].position);
    }
    mean = mul(mean, 1.f / float(nodes.size()));
    unsigned selected = nodes.front();
    float best = -INFINITY;
    for (auto a : nodes) {
        const float score = width(field, a) - .1f * distance(field.nodes[a].position, mean);
        if (score > best) {
            best = score;
            selected = a;
        }
    }
    return selected;
}

void expand_distance(
    const RoomDistanceField& field, const Graph& graph, Queue& pending, std::vector<float>& costs
) {
    while (!pending.empty()) {
        const auto [value, a] = pending.top();
        pending.pop();
        if (value != costs[a]) {
            continue;
        }
        for (auto b : graph[a]) {
            const float next = value + distance(field.nodes[a].position, field.nodes[b].position);
            if (next < costs[b]) {
                costs[b] = next;
                pending.emplace(next, b);
            }
        }
    }
}

std::vector<unsigned> area_centres(const RoomDistanceField& field, const Graph& graph) {
    const unsigned n = unsigned(graph.size());
    std::vector<bool> completed(n), core_seen(n);
    std::vector<float> reach(n, INFINITY);
    std::vector<unsigned> hubs;
    for (unsigned first = 0; first < n; ++first) {
        if (completed[first]) {
            continue;
        }
        std::vector<unsigned> component{first};
        const auto first_hub = hubs.size();
        completed[first] = true;
        float peak = 0;
        for (unsigned i = 0; i < component.size(); ++i) {
            const auto a = component[i];
            peak = std::max(peak, width(field, a));
            for (auto b : graph[a]) {
                if (!completed[b]) {
                    completed[b] = true;
                    component.push_back(b);
                }
            }
        }
        const float threshold = std::min(4.f, std::max(.75f, .6f * peak));
        Queue pending;
        bool broad = false;
        for (auto root : component) {
            if (core_seen[root] || width(field, root) < threshold) {
                continue;
            }
            std::vector<unsigned> core{root};
            core_seen[root] = true;
            for (unsigned i = 0; i < core.size(); ++i) {
                for (auto b : graph[core[i]]) {
                    if (!core_seen[b] && width(field, b) >= threshold) {
                        core_seen[b] = true;
                        core.push_back(b);
                    }
                }
            }
            if (float(core.size()) * field.step * field.step
                    < std::max(36.f, 6 * threshold * threshold)
                || compactness(field, core) < .65f) {
                continue;
            }
            broad = true;
            hubs.push_back(centre(field, core));
            // The whole open core is served by its one centre. Boundary margins are
            // not uncovered destinations and must never grow spokes towards the coast.
            for (auto a : core) {
                reach[a] = 0;
                pending.emplace(0, a);
            }
        }
        if (!broad) {
            const unsigned hub = centre(field, component);
            hubs.push_back(hub);
            if (compactness(field, component) >= 1.3f) {
                continue;  // One isolated open area.
            }
            reach[hub] = 0;
            pending.emplace(0, hub);
        }
        expand_distance(field, graph, pending, reach);
        const auto central_candidate = [&](unsigned a) {
            const float w = width(field, a);
            return w >= std::max(.5f, threshold * .4f)
                   && (field.nodes[a].ridge
                       || std::none_of(graph[a].begin(), graph[a].end(), [&](auto b) {
                              return width(field, b) > w + .35f * field.step;
                          }));
        };
        for (;;) {
            unsigned tip = missing;
            float best = -INFINITY;
            for (auto a : component) {
                const float w = width(field, a);
                if (reach[a] <= std::max(8.f, 2 * w + 2) || !central_candidate(a)) {
                    continue;
                }
                const float score = reach[a] + 2 * w;
                if (score > best) {
                    best = score;
                    tip = a;
                }
            }
            if (tip == missing) {
                break;
            }
            hubs.push_back(tip);
            reach[tip] = 0;
            pending.emplace(0, tip);
            expand_distance(field, graph, pending, reach);
        }
        // Short tunnels can be narrower than an open area yet shorter than the normal
        // destination spacing. Preserve their centred geodesic diameter, not wall spokes.
        if (!broad && hubs.size() == first_hub + 1) {
            const auto furthest = [&](const std::vector<float>& distances) {
                unsigned tip = hubs[first_hub];
                float score = -INFINITY;
                for (auto a : component) {
                    if (central_candidate(a)) {
                        const float value = distances[a] + 2 * width(field, a);
                        if (value > score) {
                            tip = a;
                            score = value;
                        }
                    }
                }
                return tip;
            };
            const unsigned from = furthest(reach);
            std::vector<float> across(n, INFINITY);
            across[from] = 0;
            pending.emplace(0, from);
            expand_distance(field, graph, pending, across);
            const unsigned to = furthest(across);
            if (across[to]
                > std::max(4.f, 2 * std::min(width(field, from), width(field, to)) + 1)) {
                for (auto tip : {from, to}) {
                    if (tip != hubs[first_hub]) {
                        hubs.push_back(tip);
                    }
                }
            }
        }
    }
    return hubs;
}
}  // namespace

RoomBackbone room_backbone(
    RoomDistanceField& field, const Scene& scene, const BakeSettings& settings
) {
    RoomBackbone result;
    repair_backbone_floor_gaps(field, scene, settings);
    const auto graph = routing_graph(field, scene, settings);
    const unsigned n = unsigned(graph.size());
    result.skeleton.resize(n);
    result.centres = area_centres(field, graph);
    std::vector<float> cost(n, INFINITY);
    std::vector<unsigned> owner(n, missing), parent(n, missing);
    Queue pending;
    for (unsigned area = 0; area < result.centres.size(); ++area) {
        const auto a = result.centres[area];
        cost[a] = 0;
        owner[a] = area;
        pending.emplace(0, a);
    }
    while (!pending.empty()) {
        const auto [value, a] = pending.top();
        pending.pop();
        if (value != cost[a]) {
            continue;
        }
        for (auto b : graph[a]) {
            const float next = value + weight(field, a, b);
            if (next < cost[b]) {
                cost[b] = next;
                owner[b] = owner[a];
                parent[b] = a;
                pending.emplace(next, b);
            }
        }
    }
    std::map<std::pair<unsigned, unsigned>, std::vector<Edge>> boundaries;
    for (unsigned a = 0; a < n; ++a) {
        for (auto b : graph[a]) {
            if (b > a && owner[a] != owner[b]) {
                boundaries[{std::min(owner[a], owner[b]), std::max(owner[a], owner[b])}].push_back(
                    {a, b}
                );
            }
        }
    }
    std::set<Edge> selected;
    const auto add_edge = [&](unsigned a, unsigned b) {
        Edge edge{std::min(a, b), std::max(a, b)};
        if (selected.insert(edge).second) {
            result.skeleton[a].push_back(b);
            result.skeleton[b].push_back(a);
        }
    };
    const auto attach = [&](unsigned a) {
        while (parent[a] != missing) {
            const auto b = parent[a];
            const Edge edge{std::min(a, b), std::max(a, b)};
            if (selected.contains(edge)) {
                break;
            }
            add_edge(a, b);
            a = b;
        }
    };
    for (const auto& [areas, edges] : boundaries) {
        (void)areas;
        // Separate openings between the same areas are separate entrances. Keeping
        // these connected boundary patches preserves real loops around obstacles.
        Components patches(unsigned(edges.size()));
        std::map<unsigned, unsigned> boundary_nodes;
        for (unsigned i = 0; i < edges.size(); ++i) {
            for (auto a : edges[i]) {
                const auto [found, added] = boundary_nodes.emplace(a, i);
                if (!added) {
                    patches.join(i, found->second);
                }
            }
        }
        for (const auto& [a, i] : boundary_nodes) {
            for (auto b : graph[a]) {
                const auto found = boundary_nodes.find(b);
                if (found != boundary_nodes.end() && owner[a] == owner[b]) {
                    patches.join(i, found->second);
                }
                if (owner[a] == owner[b]) {
                    for (auto c : graph[b]) {
                        const auto next = boundary_nodes.find(c);
                        if (next != boundary_nodes.end() && owner[a] == owner[c]) {
                            patches.join(i, next->second);
                        }
                    }
                }
            }
        }
        std::map<unsigned, unsigned> portals;
        const auto score = [&](unsigned i) {
            const auto [a, b] = edges[i];
            return cost[a] + weight(field, a, b) + cost[b];
        };
        for (unsigned i = 0; i < edges.size(); ++i) {
            const auto key = patches.root(i);
            const auto [found, added] = portals.emplace(key, i);
            if (!added && score(i) < score(found->second)) {
                found->second = i;
            }
        }
        for (const auto& [patch, index] : portals) {
            (void)patch;
            const auto [a, b] = edges[index];
            attach(a);
            attach(b);
            add_edge(a, b);
            result.entrances.push_back(a);
            result.entrances.push_back(b);
        }
    }
    return result;
}
}  // namespace openrac::lighting::bake_detail
