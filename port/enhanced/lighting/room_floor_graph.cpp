// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// One physically connected floor domain shared by GI orientation and reflection placement.
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
// Intermediate supported nodes keep the distance-field mask continuous and expose narrow
// doorways to later room segmentation. A long shortcut must not jump over their clearance.
void subdivide_bridge(
    const Scene& scene,
    std::vector<EnvironmentFloorSample>& floors,
    unsigned a,
    unsigned b,
    const BakeSettings& settings
) {
    const auto from = floors[a], to = floors[b];
    const auto path = room_floor_path(scene, from.position, to.position, settings);
    std::vector<EnvironmentFloorSample> intermediate;

    for (unsigned i = 1; i + 1 < path.size(); ++i) {
        const float t = float(i) / float(path.size() - 1);
        auto sample = from;
        sample.position = path[i];
        sample.sky = from.sky * (1 - t) + to.sky * t;
        sample.anchor = false;
        sample.neighbours.clear();

        if (from.water_height && to.water_height) {
            sample.water_height = *from.water_height * (1 - t) + *to.water_height * t;
        }

        if (from.height_bounds && to.height_bounds) {
            sample.height_bounds = Float2{
                (*from.height_bounds)[0] * (1 - t) + (*to.height_bounds)[0] * t,
                (*from.height_bounds)[1] * (1 - t) + (*to.height_bounds)[1] * t
            };
        }

        intermediate.push_back(std::move(sample));
    }

    std::erase(floors[a].neighbours, b);
    std::erase(floors[b].neighbours, a);

    if (intermediate.empty()) {
        return;
    }

    unsigned previous = a;

    for (auto& sample : intermediate) {
        const auto next = unsigned(floors.size());
        floors[previous].neighbours.push_back(next);
        sample.neighbours.push_back(previous);
        floors.push_back(std::move(sample));
        previous = next;
    }

    floors[previous].neighbours.push_back(b);
    floors[b].neighbours.push_back(previous);
}
}  // namespace

std::vector<EnvironmentFloorSample> connected_room_floors(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> source,
    const BakeSettings& settings,
    EnvironmentPlacementReport* report
) {
    std::vector<EnvironmentFloorSample> result(source.begin(), source.end());
    std::vector<EnvironmentFloor> supported;

    for (const auto& sample : source) {
        EnvironmentFloor floor;
        floor.position = sample.position;
        floor.capture = sample.position;
        floor.capture[2] += kEnvironmentCaptureHeight;
        // Authored labels constrain this physical phase; actual GI refines reflection rooms later.
        floor.light = {1, 1, 1};
        supported.push_back(floor);
    }

    EnvironmentPlacementReport local;
    connect_environment_floor_fragments(
        scene, result, supported, settings, {}, report ? *report : local, false
    );
    std::vector<std::array<unsigned, 2>> bridges;

    for (unsigned a = 0; a < source.size(); ++a) {
        for (auto b : result[a].neighbours) {
            if (b > a
                && std::find(source[a].neighbours.begin(), source[a].neighbours.end(), b)
                       == source[a].neighbours.end()) {
                bridges.push_back({a, b});
            }
        }
    }

    for (auto [a, b] : bridges) {
        subdivide_bridge(scene, result, a, b, settings);
    }

    return result;
}
}  // namespace openrac::lighting::bake_detail
