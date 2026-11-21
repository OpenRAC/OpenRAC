// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Post-watershed cleanup inspired by Recast's compatible region merging.
#include <algorithm>
#include <map>
#include <numeric>
#include <set>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr float kMergeArea = 64;        // World XY area, independent of adaptive sample density.
constexpr float kBasinMergeArea = 256;  // Requires a verified open swimming connection.
using Pair = std::array<unsigned, 2>;

bool compatible(const EnvironmentFloorSample& a, const EnvironmentFloorSample& b) {
    // Watershed ownership describes geometry. Lighting labels stay on each sample
    // and remain independent constraints on reflection sharing.
    return a.underwater == b.underwater
           && (a.swimming_surface == b.swimming_surface || a.underwater)
           && (!a.water_height || !b.water_height
               || std::abs(*a.water_height - *b.water_height) <= .25f);
}

bool single_boundary(
    std::span<const EnvironmentFloorSample> floors,
    const std::vector<unsigned>& owners,
    const Pair& pair,
    const std::set<unsigned>& boundary
) {
    // Check each side separately: two disjoint shared arcs must not be joined
    // through the interior of the other region. One ring tolerates sparse corners.
    for (auto owner : pair) {
        std::set<unsigned> side, allowed;
        for (auto a : boundary) {
            if (owners[a] == owner) {
                side.insert(a);
                allowed.insert(a);
                for (auto b : floors[a].neighbours) {
                    if (owners[b] == owner) {
                        allowed.insert(b);
                    }
                }
            }
        }
        std::set<unsigned> reached;
        std::vector<unsigned> pending{*side.begin()};
        reached.insert(pending.front());
        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto b : floors[pending[i]].neighbours) {
                if (allowed.contains(b) && reached.insert(b).second) {
                    pending.push_back(b);
                }
            }
        }
        for (auto a : side) {
            if (!reached.contains(a)) {
                return false;
            }
        }
    }
    return true;
}

bool overlapping_floors(
    std::span<const EnvironmentFloorSample> floors,
    const std::vector<unsigned>& a,
    const std::vector<unsigned>& b
) {
    // As in Recast's floor-overlap guard, never collapse vertically stacked layers.
    std::map<std::array<int, 2>, std::vector<float>> columns;
    for (auto i : a) {
        const auto p = floors[i].position;
        columns[{int(std::round(p[0] * 2)), int(std::round(p[1] * 2))}].push_back(p[2]);
    }
    for (auto i : b) {
        const auto p = floors[i].position;
        const auto found = columns.find({int(std::round(p[0] * 2)), int(std::round(p[1] * 2))});
        if (found != columns.end()
            && std::any_of(found->second.begin(), found->second.end(), [&](float z) {
                   return std::abs(z - p[2]) > .75f;
               })) {
            return true;
        }
    }
    return false;
}
}  // namespace

void merge_small_watershed_regions(
    std::span<const EnvironmentFloorSample> floors,
    RoomLayout& layout,
    const Scene* scene,
    const BakeSettings& settings
) {
    const unsigned count = unsigned(layout.rooms.size());
    std::vector<std::set<unsigned>> water_parents(count);
    using Cell = std::array<int, 2>;
    std::vector<float> water_step(count, 2);
    for (unsigned a = 0; a < floors.size(); ++a) {
        if (floors[a].swimming_surface && !floors[a].underwater) {
            for (auto b : floors[a].neighbours) {
                if (layout.owners[a] == layout.owners[b] && floors[b].swimming_surface
                    && !floors[b].underwater) {
                    const auto d = add(floors[a].position, mul(floors[b].position, -1));
                    const float step = std::hypot(d[0], d[1]);
                    if (step > .01f) {
                        water_step[layout.owners[a]] = std::min(water_step[layout.owners[a]], step);
                    }
                }
            }
        }
    }
    std::vector<std::map<Cell, std::vector<float>>> water_columns(count);
    for (unsigned a = 0; a < floors.size(); ++a) {
        if (floors[a].swimming_surface && !floors[a].underwater) {
            const auto p = floors[a].position;
            const unsigned room = layout.owners[a];
            const float step = water_step[room];
            water_columns[room][{int(std::round(p[0] / step)), int(std::round(p[1] / step))}]
                .push_back(p[2]);
        }
    }
    if (scene) {
        for (unsigned a = 0; a < floors.size(); ++a) {
            if (!floors[a].underwater || !floors[a].water_height) {
                continue;
            }
            for (auto b : floors[a].neighbours) {
                if (floors[b].swimming_surface && !floors[b].underwater
                    && std::abs(floors[b].position[2] - *floors[a].water_height) <= .25f) {
                    water_parents[layout.owners[a]].insert(layout.owners[b]);
                }
            }
        }
    }
    const auto open_basin = [&](unsigned a, unsigned b) {
        if (!scene || !floors[a].underwater || !floors[b].underwater || !floors[a].water_height
            || !floors[b].water_height) {
            return false;
        }
        const float surface = std::min(*floors[a].water_height, *floors[b].water_height);
        auto p = floors[a].position, q = floors[b].position;
        const float swim_floor = surface - settings.traversal.player_height - .1f;
        if (std::max(p[2], q[2]) + .1f > swim_floor) {
            return false;
        }
        p[2] = q[2] = swim_floor;
        const auto delta = add(q, mul(p, -1));
        const float length = std::hypot(delta[0], delta[1]);
        const Float3 side =
            length > .001f ? Float3{-delta[1] / length, delta[0] / length, 0} : Float3{1, 0, 0};
        for (auto parent : water_parents[layout.owners[a]]) {
            if (!water_parents[layout.owners[b]].contains(parent)) {
                continue;
            }
            bool known = true;
            const unsigned steps = std::max(1u, unsigned(std::ceil(length / .25f)));
            for (unsigned i = 0; i <= steps && known; ++i) {
                for (float radius :
                     {-settings.traversal.player_radius, 0.f, settings.traversal.player_radius}) {
                    const auto point =
                        add(add(p, mul(delta, float(i) / float(steps))), mul(side, radius));
                    const float step = water_step[parent];
                    const auto column = water_columns[parent].find(
                        {int(std::round(point[0] / step)), int(std::round(point[1] / step))}
                    );
                    if (column == water_columns[parent].end()
                        || !std::any_of(column->second.begin(), column->second.end(), [&](float z) {
                               return std::abs(z - surface) <= .5f;
                           })) {
                        known = false;
                        break;
                    }
                }
            }
            if (known && room_body_path_clear(*scene, p, q, settings)
                && room_body_path_clear(*scene, q, p, settings)) {
                return true;
            }
        }
        return false;
    };
    auto receiver_settings = settings;
    receiver_settings.traversal.player_radius = 0;
    std::vector<Pair> receiver_contacts;
    if (scene) {
        for (const auto& room : layout.rooms) {
            if (room.empty() || room.size() > 4
                || !std::all_of(room.begin(), room.end(), [&](auto a) {
                       return floors[a].swimming_surface && floors[a].underwater;
                   })) {
                continue;
            }
            for (auto a : room) {
                for (unsigned b = 0; b < floors.size(); ++b) {
                    if (floors[b].swimming_surface || layout.owners[a] == layout.owners[b]
                        || !compatible(floors[a], floors[b])) {
                        continue;
                    }
                    const auto p = floors[a].position, q = floors[b].position;
                    const auto d = add(p, mul(q, -1));
                    if (dot(d, d) > 3.01f * 3.01f || std::abs(d[2]) > .75f) {
                        continue;
                    }
                    // Receivers at wall edges need ownership, not a traversable player centre.
                    // These contacts never enter the floor graph or authorize playable space.
                    const bool supported = room_path_clear(*scene, p, q, settings)
                                           && room_path_clear(*scene, q, p, settings)
                                           && !room_floor_path(*scene, p, q, settings).empty();
                    // A steep submerged face need not be walkable. Known connections
                    // to the same surface region prove a shared swimming volume.
                    const auto& parents = water_parents[layout.owners[a]];
                    const auto& other_parents = water_parents[layout.owners[b]];
                    const bool same_pool =
                        std::any_of(parents.begin(), parents.end(), [&](auto parent) {
                            return other_parents.contains(parent);
                        });
                    const bool submerged =
                        same_pool && floors[a].water_height
                        && *floors[a].water_height
                               > std::max(p[2], q[2]) + settings.traversal.player_height + .06f;
                    if (supported
                        || (submerged && room_body_path_clear(*scene, p, q, receiver_settings)
                            && room_body_path_clear(*scene, q, p, receiver_settings))) {
                        receiver_contacts.push_back({a, b});
                    }
                }
            }
        }
    }
    std::vector<float> area(count), peak(count), core_area(count), footprints(floors.size());
    std::vector<unsigned> core_points(count);
    for (unsigned a = 0; a < floors.size(); ++a) {
        float footprint = 4;  // Two-unit lattice; refined samples contribute less area.
        for (auto b : floors[a].neighbours) {
            const auto d = add(floors[a].position, mul(floors[b].position, -1));
            const float squared = d[0] * d[0] + d[1] * d[1];
            if (squared > .01f && compatible(floors[a], floors[b])) {
                footprint = std::min(footprint, squared);
            }
        }
        footprints[a] = footprint;
        area[layout.owners[a]] += footprint;
        peak[layout.owners[a]] = std::max(peak[layout.owners[a]], layout.clearance[a]);
    }
    const auto measure_core = [&](unsigned room) {
        float result = 0;
        core_points[room] = 0;
        for (auto a : layout.rooms[room]) {
            if (layout.clearance[a] >= std::max(1.5f, .65f * peak[room])) {
                result += footprints[a];
                ++core_points[room];
            }
        }
        return result;
    };
    for (unsigned room = 0; room < count; ++room) {
        core_area[room] = measure_core(room);
    }
    for (;;) {
        struct Boundary {
            float saddle = 0;
            bool compatible = true;
            std::set<unsigned> nodes;
            std::vector<Pair> edges;
        };

        std::map<Pair, Boundary> boundaries;
        const auto add_contact = [&](unsigned a, unsigned b) {
            const unsigned first = layout.owners[a], second = layout.owners[b];
            if (first == second) {
                return;
            }
            auto& boundary = boundaries[{std::min(first, second), std::max(first, second)}];
            boundary.compatible &= compatible(floors[a], floors[b]);
            boundary.saddle =
                std::max(boundary.saddle, std::min(layout.clearance[a], layout.clearance[b]));
            boundary.nodes.insert(a);
            boundary.nodes.insert(b);
            boundary.edges.push_back({a, b});
        };
        for (unsigned a = 0; a < floors.size(); ++a) {
            for (auto b : floors[a].neighbours) {
                if (b > a) {
                    add_contact(a, b);
                }
            }
        }
        for (auto pair : receiver_contacts) {
            add_contact(pair[0], pair[1]);
        }
        std::vector<unsigned> order(count);
        std::iota(order.begin(), order.end(), 0u);
        std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) {
            return area[a] < area[b];
        });
        bool merged = false;
        for (auto a : order) {
            if (layout.rooms[a].empty() || area[a] >= kBasinMergeArea
                || (area[a] >= kMergeArea && !floors[layout.rooms[a].front()].underwater)) {
                continue;
            }
            unsigned target = count;
            std::size_t contact = 0;
            for (const auto& [pair, boundary] : boundaries) {
                if (pair[0] != a && pair[1] != a) {
                    continue;
                }
                const unsigned b = pair[0] == a ? pair[1] : pair[0];
                if (area[b] < area[a] || !boundary.compatible) {
                    continue;
                }
                const bool swimming =
                    std::any_of(boundary.edges.begin(), boundary.edges.end(), [&](auto edge) {
                        return open_basin(edge[0], edge[1]);
                    });
                // Preserve doorway and boundary topology between real interiors.
                // Thin fragments may wrap around props without defining a room.
                const bool interiors = core_points[a] >= 2 && core_points[b] >= 2
                                       && core_area[a] >= std::max(4.f, .25f * peak[a] * peak[a])
                                       && core_area[b] >= std::max(4.f, .25f * peak[b] * peak[b]);
                if ((area[a] >= kMergeArea && !swimming)
                    || (!swimming && interiors && boundary.saddle < 1.5f
                        && boundary.saddle + .25f < .65f * std::min(peak[a], peak[b]))
                    // Lighting islands can wrap around submerged clutter. Unlike a
                    // navigation polygon, no contour is collapsed by this merge.
                    || (!swimming && interiors
                        && !single_boundary(floors, layout.owners, pair, boundary.nodes))
                    || overlapping_floors(floors, layout.rooms[a], layout.rooms[b])) {
                    continue;
                }
                if (target == count || boundary.nodes.size() > contact
                    || (boundary.nodes.size() == contact && area[b] > area[target])) {
                    target = b;
                    contact = boundary.nodes.size();
                }
            }
            if (target == count) {
                continue;  // Keep isolated or incompatible small playable areas.
            }
            for (auto sample : layout.rooms[a]) {
                layout.owners[sample] = target;
                layout.rooms[target].push_back(sample);
            }
            layout.rooms[a].clear();
            area[target] += area[a];
            peak[target] = std::max(peak[target], peak[a]);
            core_area[target] = measure_core(target);
            water_parents[target].insert(water_parents[a].begin(), water_parents[a].end());
            area[a] = 0;
            merged = true;
            break;  // Rebuild adjacency before another merge.
        }
        if (!merged) {
            break;
        }
    }
    std::vector<std::vector<unsigned>> rooms;
    for (auto& room : layout.rooms) {
        if (!room.empty()) {
            for (auto sample : room) {
                layout.owners[sample] = unsigned(rooms.size());
            }
            rooms.push_back(std::move(room));
        }
    }
    layout.rooms = std::move(rooms);
}
}  // namespace openrac::lighting::bake_detail
