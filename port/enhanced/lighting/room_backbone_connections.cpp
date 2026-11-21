// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Repair sampled floor gaps across medium tags using real intermediate support.
#include <algorithm>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_backbone.h"

namespace openrac::lighting::bake_detail {
namespace {
bool checked_path(
    const Scene& scene, const std::vector<Float3>& path, const BakeSettings& settings
) {
    if (path.size() < 2) {
        return false;
    }
    for (unsigned i = 1; i < path.size(); ++i) {
        if (!room_supported_path_clear(scene, path[i - 1], path[i], settings)
            || !room_supported_path_clear(scene, path[i], path[i - 1], settings)) {
            return false;
        }
    }
    return true;
}

// A short detour can follow uneven ground where the endpoint chord crosses a ledge.
// Every edge still requires actual support and the normal player clearance checks.
std::vector<Float3> local_floor_path(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
) {
    struct Node {
        Float3 p;
        float cost;
        unsigned parent;
    };

    std::vector<Node> nodes{{a, 0, 0}};
    using Key = std::array<int, 3>;
    std::map<Key, float> costs;
    using Entry = std::pair<float, unsigned>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
    const auto distance = [](Float3 p, Float3 q) {
        const auto d = add(p, mul(q, -1));
        return std::sqrt(dot(d, d));
    };
    const auto key = [&](Float3 p) {
        return Key{
            int(std::round((p[0] - a[0]) * 4)),
            int(std::round((p[1] - a[1]) * 4)),
            int(std::round(p[2] * 4))
        };
    };
    costs[key(a)] = 0;
    pending.push({distance(a, b), 0});
    while (!pending.empty() && nodes.size() < 4096) {
        const unsigned index = pending.top().second;
        pending.pop();
        const auto current = nodes[index];
        if (current.cost > costs[key(current.p)]) {
            continue;
        }
        if (distance(current.p, b) <= 1 && checked_path(scene, {current.p, b}, settings)) {
            std::vector<Float3> path{b};
            for (unsigned i = index;; i = nodes[i].parent) {
                path.push_back(nodes[i].p);
                if (i == 0) {
                    break;
                }
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        for (int x = -3; x <= 3; ++x) {
            for (int y = -3; y <= 3; ++y) {
                if ((!x && !y) || x * x + y * y > 9) {
                    continue;
                }
                auto target = add(current.p, Float3{float(x) * .25f, float(y) * .25f, 0});
                if (target[0] < std::min(a[0], b[0]) - 2 || target[0] > std::max(a[0], b[0]) + 2
                    || target[1] < std::min(a[1], b[1]) - 2
                    || target[1] > std::max(a[1], b[1]) + 2) {
                    continue;
                }
                target[2] += 2;
                const auto hit = scene.trace(target, {0, 0, -1}, settings.ray_bias, 4);
                if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                    continue;
                }
                target[2] -= hit.ray.tfar;
                const auto floor = room_floor_position(scene, target, settings);
                if (!floor || std::abs((*floor)[2] - a[2]) > 3
                    || std::abs((*floor)[2] - b[2]) > 3) {
                    continue;
                }
                const float cost = current.cost + distance(current.p, *floor);
                if (cost + distance(*floor, b) > 18) {
                    continue;
                }
                const auto k = key(*floor);
                const auto found = costs.find(k);
                if (found != costs.end() && found->second <= cost) {
                    continue;
                }
                if (!checked_path(scene, {current.p, *floor}, settings)) {
                    continue;
                }
                costs[k] = cost;
                nodes.push_back({*floor, cost, index});
                pending.push({cost + distance(*floor, b), unsigned(nodes.size() - 1)});
            }
        }
    }
    return {};
}
}  // namespace

void repair_backbone_floor_gaps(
    RoomDistanceField& field, const Scene& scene, const BakeSettings& settings
) {
    if (!field.water_footprint) {
        return;
    }
    const unsigned count = unsigned(field.nodes.size());
    std::vector<unsigned> parent(count);
    std::iota(parent.begin(), parent.end(), 0u);
    const auto root = [&](unsigned a) {
        while (parent[a] != a) {
            parent[a] = parent[parent[a]];
            a = parent[a];
        }
        return a;
    };
    for (unsigned a = 0; a < count; ++a) {
        for (auto b : field.nodes[a].neighbours) {
            parent[root(b)] = root(a);
        }
    }

    struct Candidate {
        unsigned a, b;
        float squared;
    };

    std::vector<Candidate> candidates;
    constexpr float range = 8;
    using Key = std::array<int, 2>;
    std::map<Key, std::vector<unsigned>> bins;
    for (unsigned a = 0; a < count; ++a) {
        const auto& node = field.nodes[a];
        if (field.swimming_rooms[node.room] && !field.submerged_rooms[node.room]) {
            continue;
        }
        const auto p = node.position;
        const Key bin{int(std::floor(p[0] / range)), int(std::floor(p[1] / range))};
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                const auto found = bins.find({bin[0] + x, bin[1] + y});
                if (found == bins.end()) {
                    continue;
                }
                for (auto b : found->second) {
                    if (root(a) == root(b)) {
                        continue;
                    }
                    const auto d = add(p, mul(field.nodes[b].position, -1));
                    const float squared = dot(d, d);
                    if (std::abs(d[2]) <= 2 && squared <= range * range) {
                        candidates.push_back({a, b, squared});
                    }
                }
            }
        }
        bins[bin].push_back(a);
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        if (a.squared != b.squared) {
            return a.squared < b.squared;
        }
        return std::pair{a.a, a.b} < std::pair{b.a, b.b};
    });
    std::map<std::pair<unsigned, unsigned>, unsigned> trials;
    for (const auto& candidate : candidates) {
        const auto a = candidate.a, b = candidate.b;
        const auto ra = root(a), rb = root(b);
        if (ra == rb) {
            continue;
        }
        if (trials[{std::min(ra, rb), std::max(ra, rb)}]++ >= 32) {
            continue;
        }
        const auto p = field.nodes[a].position, q = field.nodes[b].position;
        auto path = room_floor_path(scene, p, q, settings);
        if (!checked_path(scene, path, settings)) {
            path = local_floor_path(scene, p, q, settings);
        }
        if (path.size() < 2) {
            continue;
        }
        const unsigned room = field.nodes[a].room;
        // The source mask has no columns in this repaired gap. Record the checked
        // route densely enough that smoothing can follow ramps and legal steps too.
        for (unsigned i = 1; i < path.size(); ++i) {
            const auto d = add(path[i], mul(path[i - 1], -1));
            const unsigned steps =
                std::max(1u, unsigned(std::ceil(std::sqrt(dot(d, d)) / (field.step * .25f))));
            for (unsigned j = 0; j <= steps; ++j) {
                const auto p = add(path[i - 1], mul(d, float(j) / float(steps)));
                field
                    .domain[{
                        room, int(std::round(p[0] / field.step)), int(std::round(p[1] / field.step))
                    }]
                    .push_back(p[2]);
            }
        }
        unsigned previous = a;
        for (unsigned i = 1; i + 1 < path.size(); ++i) {
            if (field.nodes.size() >= 256000) {
                throw std::runtime_error("backbone floor repair exceeds field sample budget");
            }
            auto eye = path[i];
            eye[2] += 1.25f;
            const auto index = unsigned(field.nodes.size());
            field.nodes.push_back(
                {path[i], room_wall_distance(scene, eye), room, false, {previous}}
            );
            field.nodes[previous].neighbours.push_back(index);
            field
                .domain
                    [{room,
                      int(std::round(path[i][0] / field.step)),
                      int(std::round(path[i][1] / field.step))}]
                .push_back(path[i][2]);
            previous = index;
        }
        field.nodes[previous].neighbours.push_back(b);
        field.nodes[b].neighbours.push_back(previous);
        parent[rb] = ra;
    }
    scene.check_error();
}
}  // namespace openrac::lighting::bake_detail
