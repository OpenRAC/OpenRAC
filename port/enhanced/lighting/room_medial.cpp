// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Ridge-biased connections and collision-checked smoothing of a sparse distance field.
#include <algorithm>
#include <format>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned kMissing = ~0u;
using Edge = std::array<unsigned, 2>;

Edge edge(unsigned a, unsigned b) {
    return {std::min(a, b), std::max(a, b)};
}

float distance(Float3 a, Float3 b) {
    const auto d = add(a, mul(b, -1));
    return std::sqrt(dot(d, d));
}

bool clear_segment(
    const Scene& scene,
    const RoomDistanceField& field,
    Float3 a,
    Float3 b,
    unsigned room,
    const BakeSettings& settings
) {
    return field.connects(a, b, room)
           && (field.swimming_rooms[room] ? room_body_path_clear(scene, a, b, settings, true)
                                          : room_path_clear(scene, a, b, settings));
}

// Split a chain where a straight segment no longer follows it, and at a bounded length.
void simplify(
    std::span<const unsigned> path,
    std::span<const Float3> points,
    const Scene& scene,
    const BakeSettings& settings,
    const RoomDistanceField& field,
    unsigned room,
    std::vector<Edge>& output
) {
    if (path.size() < 2) {
        return;
    }

    std::vector<Edge> pending{{0, static_cast<unsigned>(path.size() - 1)}};

    while (!pending.empty()) {
        const auto interval = pending.back();
        pending.pop_back();
        const auto a = points[path[interval[0]]], b = points[path[interval[1]]];
        const auto d = add(b, mul(a, -1));
        unsigned split = interval[0];
        float error = .5625f * field.step * field.step;

        for (unsigned i = interval[0] + 1; i < interval[1]; ++i) {
            const auto q = add(points[path[i]], mul(a, -1));
            const float t = std::clamp(dot(q, d) / std::max(.0001f, dot(d, d)), 0.f, 1.f);
            const auto offset = add(q, mul(d, -t));

            if (dot(offset, offset) > error) {
                error = dot(offset, offset);
                split = i;
            }
        }

        if (split == interval[0] && distance(a, b) > 12 && interval[1] > interval[0] + 1) {
            split = (interval[0] + interval[1]) / 2;
        }

        if (split == interval[0] && !clear_segment(scene, field, a, b, room, settings)) {
            if (interval[1] == interval[0] + 1) {
                throw std::runtime_error(
                    std::format(
                        "invalid medial field connection in room {} (swimming {}): {} {} {} to {} "
                        "{} "
                        "{}",
                        room,
                        bool(field.swimming_rooms[room]),
                        a[0],
                        a[1],
                        a[2],
                        b[0],
                        b[1],
                        b[2]
                    )
                );
            }

            split = (interval[0] + interval[1]) / 2;
        }

        if (split == interval[0]) {
            output.push_back({path[interval[0]], path[interval[1]]});
        } else {
            pending.push_back({split, interval[1]});
            pending.push_back({interval[0], split});
        }
    }
}
}  // namespace

void RoomLayout::rebuild_medial(
    const Scene& scene, std::span<const EnvironmentFloorSample> floors, const BakeSettings& settings
) {
    medial_edges.clear();
    medial_centres.clear();
    medial_points.clear();
    medial_owners.clear();
    auto field = room_distance_field(scene, floors, *this, settings);
    medial_cell_size = field.step;
    medial_ridge_samples = static_cast<unsigned>(
        std::count_if(field.nodes.begin(), field.nodes.end(), [](const auto& n) { return n.ridge; })
    );
    const auto n = field.nodes.size();
    std::vector<std::vector<unsigned>> neighbours(n), skeleton(n), field_rooms(rooms.size());

    for (unsigned a = 0; a < n; ++a) {
        medial_points.push_back(field.nodes[a].position);
        medial_owners.push_back(field.nodes[a].room);
        neighbours[a] = field.nodes[a].neighbours;
        field_rooms[field.nodes[a].room].push_back(a);
    }

    auto width = [&](unsigned a) {
        return std::clamp(field.nodes[a].wall.distance, .25f, 12.f);
    };
    using Entry = std::pair<float, unsigned>;
    std::vector<bool> completed(n), selected(n);
    std::vector<float> cost(n), reach(n);
    std::vector<unsigned> parent(n);

    for (unsigned room = 0; room < field_rooms.size(); ++room) {
        // Small gaps merged for lighting purposes may still be physically disconnected. Each
        // component gets a seed; no medial edge is invented across a missing walkable connection.
        if (field_rooms[room].empty()) {
            continue;
        }

        const auto target = floors[centre(floors, room)].position;
        auto seeds = field_rooms[room];
        std::stable_sort(seeds.begin(), seeds.end(), [&](auto a, auto b) {
            const auto score = [&](auto node) {
                return distance(medial_points[node], target) - .25f * std::min(width(node), 8.f)
                       - (field.nodes[node].ridge ? field.step : 0);
            };
            return score(a) < score(b);
        });

        for (auto root : seeds) {
            if (completed[root]) {
                continue;
            }

            std::fill(cost.begin(), cost.end(), INFINITY);
            std::fill(parent.begin(), parent.end(), kMissing);
            cost[root] = 0;
            std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
            pending.emplace(0, root);
            std::vector<unsigned> component;

            while (!pending.empty()) {
                const auto [value, a] = pending.top();
                pending.pop();

                if (value != cost[a]) {
                    continue;
                }

                completed[a] = true;
                component.push_back(a);

                for (auto b : neighbours[a]) {
                    const float w = std::min(width(a), width(b));
                    const float next =
                        value
                        + distance(medial_points[a], medial_points[b]) * (1 + 4 / (w * w))
                              * (field.nodes[a].ridge && field.nodes[b].ridge ? .5f : 1.f);

                    if (next < cost[b]) {
                        cost[b] = next;
                        parent[b] = a;
                        pending.emplace(next, b);
                    }
                }
            }

            medial_centres.push_back(root);
            selected[root] = true;

            for (;;) {
                // Geodesic distance to the current skeleton avoids cutting through walls.
                std::fill(reach.begin(), reach.end(), INFINITY);

                for (auto a : component) {
                    if (selected[a]) {
                        reach[a] = 0;
                        pending.emplace(0, a);
                    }
                }

                while (!pending.empty()) {
                    const auto [value, a] = pending.top();
                    pending.pop();

                    if (value != reach[a]) {
                        continue;
                    }

                    for (auto b : neighbours[a]) {
                        const float next = value + distance(medial_points[a], medial_points[b]);

                        if (next < reach[b]) {
                            reach[b] = next;
                            pending.emplace(next, b);
                        }
                    }
                }

                unsigned tip = kMissing;
                float excess = 4;

                for (auto a : component) {
                    const float value = reach[a] - 1.5f * width(a);

                    if (value > excess) {
                        excess = value;
                        tip = a;
                    }
                }

                if (tip == kMissing) {
                    break;
                }

                // The weighted shortest-path tree prefers high-clearance centres. Only
                // substantial uncovered branches survive, rather than one branch per obstacle.
                while (!selected[tip] && parent[tip] != kMissing) {
                    selected[tip] = true;
                    const auto next = parent[tip];
                    skeleton[tip].push_back(next);
                    skeleton[next].push_back(tip);
                    tip = next;
                }
            }
        }
    }

    // Smooth only degree-two points. Junctions/endpoints retain topology, and each
    // candidate must keep its clearance and remain connected over real floor geometry.
    const auto original = medial_points;

    for (unsigned a = 0; a < n; ++a) {
        if (skeleton[a].size() == 2) {
            const auto left = skeleton[a][0], right = skeleton[a][1];
            const auto proposed =
                add(mul(original[a], .5f), mul(add(original[left], original[right]), .25f));
            const auto floor = field.swimming_rooms[medial_owners[a]]
                                   ? std::optional<Float3>{proposed}
                                   : room_floor_position(scene, proposed, settings);

            if (!floor
                || !clear_segment(scene, field, original[a], *floor, medial_owners[a], settings)
                || !clear_segment(
                    scene, field, medial_points[left], *floor, medial_owners[a], settings
                )
                || !clear_segment(
                    scene, field, *floor, medial_points[right], medial_owners[a], settings
                )) {
                continue;
            }

            auto eye = *floor;
            eye[2] += 1.25f;

            if (room_wall_distance(scene, eye).distance + .2f * field.step
                < field.nodes[a].wall.distance) {
                continue;
            }

            medial_points[a] = *floor;
        }
    }

    std::set<Edge> visited;

    for (unsigned a = 0; a < n; ++a) {
        if (skeleton[a].size() == 2) {
            continue;
        }

        for (auto b : skeleton[a]) {
            if (visited.contains(edge(a, b))) {
                continue;
            }

            std::vector<unsigned> path{a};
            unsigned previous = a, current = b;

            for (;;) {
                visited.insert(edge(previous, current));
                path.push_back(current);

                if (skeleton[current].size() != 2) {
                    break;
                }

                const auto next =
                    skeleton[current][0] == previous ? skeleton[current][1] : skeleton[current][0];
                previous = current;
                current = next;
            }

            simplify(path, medial_points, scene, settings, field, medial_owners[a], medial_edges);
        }
    }

    connect_room_links(*this, field, scene, settings);
}
}  // namespace openrac::lighting::bake_detail
