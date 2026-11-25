// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Reachability is shared by water receivers, basin floors, GI and reflection placement.
#include <algorithm>
#include <cmath>
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_water_internal.h"
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
namespace {
bool wet(const EnvironmentFloorSample& floor) {
    return floor.swimming_surface || floor.underwater;
}

// Surface meshes describe the visible waterline, which can stop at an overhang.
// Recover small basin-floor slivers from adjacent confirmed water, not XY overlap.
void fill_underwater_floor_gaps(
    const Scene& scene,
    std::vector<EnvironmentFloorSample>& floors,
    unsigned original_count,
    float spacing,
    const BakeSettings& settings
) {
    std::vector<bool> seen(original_count);
    for (unsigned first = 0; first < original_count; ++first) {
        if (seen[first] || wet(floors[first])) {
            continue;
        }
        std::vector<unsigned> component{first};
        seen[first] = true;
        std::vector<std::array<unsigned, 2>> entries;
        std::optional<float> water;
        float highest = 0;
        bool consistent = true;
        for (unsigned i = 0; i < component.size(); ++i) {
            for (auto b : floors[component[i]].neighbours) {
                if (b >= original_count) {
                    continue;
                }
                if (floors[b].underwater && floors[b].water_height) {
                    const float height = *floors[b].water_height;
                    if (!water) {
                        highest = height;
                    }
                    highest = std::max(highest, height);
                    water = water ? std::min(*water, height) : height;
                    consistent &= highest - *water <= .25f;
                    entries.push_back({component[i], b});
                } else if (!wet(floors[b]) && !seen[b]) {
                    seen[b] = true;
                    component.push_back(b);
                }
            }
        }
        if (!water || !consistent || component.size() * spacing * spacing > 64) {
            continue;
        }
        if (std::any_of(component.begin(), component.end(), [&](auto a) {
                return floors[a].position[2] > *water - settings.traversal.minimum_swim_depth;
            })) {
            continue;
        }
        const auto clear = [&](unsigned a, unsigned b) {
            return room_supported_path_clear(
                       scene, floors[a].position, floors[b].position, settings
                   )
                   && room_supported_path_clear(
                       scene, floors[b].position, floors[a].position, settings
                   );
        };
        std::vector<bool> reached(original_count);
        std::vector<unsigned> pending;
        for (auto entry : entries) {
            if (!reached[entry[0]] && clear(entry[0], entry[1])) {
                reached[entry[0]] = true;
                pending.push_back(entry[0]);
            }
        }
        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto b : floors[pending[i]].neighbours) {
                if (b < original_count && !wet(floors[b]) && !reached[b] && clear(pending[i], b)) {
                    reached[b] = true;
                    pending.push_back(b);
                }
            }
        }
        if (!std::all_of(component.begin(), component.end(), [&](auto a) { return reached[a]; })) {
            continue;
        }
        for (auto a : component) {
            auto& floor = floors[a];
            floor.underwater = true;
            floor.water_height = *water;
            floor.water_bottom = floor.position[2];
            floor.height_bounds = Float2{floor.position[2] - .8f, *water - .25f};
        }
    }
}

struct WaterDomain {
    using Key = std::array<int, 2>;
    float spacing;
    std::map<Key, std::vector<float>> columns;

    bool contains(Float3 p) const {
        const float x = p[0] / spacing, y = p[1] / spacing;
        for (int ix = int(std::ceil(x - .5001f)); ix <= int(std::floor(x + .5001f)); ++ix) {
            for (int iy = int(std::ceil(y - .5001f)); iy <= int(std::floor(y + .5001f)); ++iy) {
                const auto found = columns.find({ix, iy});
                if (found != columns.end()
                    && std::any_of(found->second.begin(), found->second.end(), [&](float z) {
                           return std::abs(z - p[2]) <= .5f;
                       })) {
                    return true;
                }
            }
        }
        return false;
    }

    bool connects(
        const EnvironmentFloorSample& a, const EnvironmentFloorSample& b, float radius
    ) const {
        auto p = a.position, q = b.position;
        p[2] = *a.water_height;
        q[2] = *b.water_height;
        const auto d = add(q, mul(p, -1));
        const float length = std::hypot(d[0], d[1]);
        const Float3 side =
            length > .001f ? Float3{-d[1] / length, d[0] / length, 0} : Float3{1, 0, 0};
        const unsigned steps = std::max(1u, unsigned(std::ceil(length / .5f)));
        for (unsigned i = 0; i <= steps; ++i) {
            const auto point = add(p, mul(d, float(i) / float(steps)));
            for (float offset : {-radius, 0.f, radius}) {
                if (!contains(add(point, mul(side, offset)))) {
                    return false;
                }
            }
        }
        return true;
    }
};

bool water_connection(
    const Scene& scene,
    const EnvironmentFloorSample& a,
    const EnvironmentFloorSample& b,
    const BakeSettings& settings,
    const WaterDomain& domain
) {
    if (!wet(a) && !wet(b)) {
        return room_body_path_clear(scene, a.position, a.position, settings)
               && room_body_path_clear(scene, b.position, b.position, settings)
               && !room_floor_path(scene, a.position, b.position, settings).empty();
    }
    if (wet(a) && wet(b)) {
        if (!a.water_height || !b.water_height || std::abs(*a.water_height - *b.water_height) > .25f
            || !domain.connects(a, b, settings.traversal.player_radius)) {
            return false;
        }
    } else {
        const auto& water = wet(a) ? a : b;
        const auto& land = wet(a) ? b : a;
        // Enter through a real shoreline, not a nearby roof or a sealed room below a pool.
        if (water.underwater || !water.water_height || land.position[2] < *water.water_height - .5f
            || land.position[2] > *water.water_height + 2.f) {
            return false;
        }
        const auto delta = add(water.position, mul(land.position, -1));
        const unsigned steps =
            std::max(1u, unsigned(std::ceil(std::hypot(delta[0], delta[1]) / .5f)));
        for (unsigned i = 0; i <= steps; ++i) {
            const auto point = add(land.position, mul(delta, float(i) / float(steps)));
            if (domain.contains({point[0], point[1], *water.water_height})) {
                continue;
            }
            const auto support = room_floor_position(scene, point, settings);
            if (!support || std::abs((*support)[2] - point[2]) > .75f) {
                return false;
            }
        }
    }
    auto feet_a = a.position, feet_b = b.position;
    const float submerged = settings.traversal.player_height * .25f;
    const auto immersion = [&](const EnvironmentFloorSample& floor) {
        return floor.water_bottom
                   ? std::min(
                         submerged, std::max(0.f, floor.position[2] - *floor.water_bottom - .06f)
                     )
                   : submerged;
    };
    if (a.swimming_surface && !a.underwater) {
        feet_a[2] -= immersion(a);
    }
    if (b.swimming_surface && !b.underwater) {
        feet_b[2] -= immersion(b);
    }
    // The lower sweep catches submerged banks and partitions; the upper sweep checks
    // headroom. Both include player radius and endpoint clearance in both directions.
    return room_body_path_clear(scene, feet_a, feet_b, settings, true)
           && room_body_path_clear(scene, a.position, b.position, settings, true);
}
}  // namespace

void connect_reachable_water(
    std::span<const GameplayTriangle> triangles,
    std::vector<EnvironmentFloorSample>& floors,
    unsigned original_count,
    float spacing,
    const BakeTraversalSettings& traversal
) {
    BakeScene geometry;
    for (const auto& triangle : triangles) {
        if (!triangle.swimmable) {
            BakeTriangle solid;
            solid.vertices = triangle.vertices;
            geometry.triangles.push_back(solid);
        }
    }
    Scene scene(geometry);
    BakeSettings settings;
    settings.traversal = traversal;
    fill_underwater_floor_gaps(scene, floors, original_count, spacing, settings);
    WaterDomain domain{spacing, {}};
    for (unsigned a = original_count; a < floors.size(); ++a) {
        floors[a].anchor = false;
        floors[a].neighbours.clear();
        if (floors[a].swimming_surface && !floors[a].underwater) {
            const auto p = floors[a].position;
            domain.columns[{int(std::round(p[0] / spacing)), int(std::round(p[1] / spacing))}]
                .push_back(p[2]);
        }
    }
    // XY bins retain vertical water-column links without comparing every floor to every pool.
    using Key = std::array<int, 2>;
    std::map<Key, std::vector<unsigned>> bins;
    const float range = spacing * 2;
    for (unsigned a = 0; a < floors.size(); ++a) {
        const auto p = floors[a].position;
        const Key cell{int(std::floor(p[0] / range)), int(std::floor(p[1] / range))};
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                const auto found = bins.find({cell[0] + dx, cell[1] + dy});
                if (found == bins.end()) {
                    continue;
                }
                for (auto b : found->second) {
                    if (a < original_count && b < original_count) {
                        continue;
                    }
                    const auto d = add(p, mul(floors[b].position, -1));
                    if (d[0] * d[0] + d[1] * d[1] > range * range
                        || (!wet(floors[a]) && !wet(floors[b]) && std::abs(d[2]) > 2)) {
                        continue;
                    }
                    if (!water_connection(scene, floors[a], floors[b], settings, domain)) {
                        continue;
                    }
                    floors[a].neighbours.push_back(b);
                    floors[b].neighbours.push_back(a);
                }
            }
        }
        bins[cell].push_back(a);
    }
    std::vector<unsigned> pending, remap(floors.size(), ~0u);
    for (unsigned a = 0; a < original_count; ++a) {
        if (floors[a].anchor) {
            remap[a] = 0;
            pending.push_back(a);
        }
    }
    for (unsigned i = 0; i < pending.size(); ++i) {
        const unsigned a = pending[i];
        for (auto b : floors[a].neighbours) {
            if (remap[b] != ~0u) {
                continue;
            }
            remap[b] = 0;
            pending.push_back(b);
            if ((a >= original_count) != (b >= original_count)
                && (wet(floors[a]) || wet(floors[b]))) {
                floors[b].evidence = 3;
            }
            if (b >= original_count) {
                floors[b].sky = floors[b].underwater ? 0.f : floors[a].sky;
                floors[b].lighting_labels = floors[a].lighting_labels;
            }
        }
    }
    std::sort(pending.begin(), pending.end());
    for (unsigned i = 0; i < pending.size(); ++i) {
        remap[pending[i]] = i;
    }
    std::vector<EnvironmentFloorSample> reachable;
    for (auto a : pending) {
        auto floor = std::move(floors[a]);
        std::erase_if(floor.neighbours, [&](auto b) { return remap[b] == ~0u; });
        for (auto& b : floor.neighbours) {
            b = remap[b];
        }
        reachable.push_back(std::move(floor));
    }
    floors = std::move(reachable);
    scene.check_error();
}
}  // namespace openrac::lighting::bake_detail
