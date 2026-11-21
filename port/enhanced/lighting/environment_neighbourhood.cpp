// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/environment_neighbourhood.h"

#include <algorithm>
#include <cmath>
#include <queue>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_placement_internal.h"

namespace openrac::lighting::bake_detail {
EnvironmentNeighbourhoods::EnvironmentNeighbourhoods(
    std::span<const EnvironmentFloor> floors,
    float radius,
    const Scene* scene,
    const BakeSettings* settings
)
    : m_floors(floors),
      m_radius(radius),
      m_overlap(std::min(4.f, radius * .15f)),
      m_scene(scene),
      m_settings(settings) {}

float EnvironmentNeighbourhoods::distance(unsigned origin, unsigned receiver) {
    const auto [entry, fresh] = m_distances.try_emplace(origin);
    auto& distances = entry->second;
    auto& visible = m_visible[origin];
    const float reach = radius(origin) + overlap(origin);
    if (fresh) {
        distances.assign(m_floors.size(), INFINITY);
        visible.assign(m_floors.size(), false);
        visible[origin] = true;
        distances[origin] = 0;
        using Node = std::pair<float, unsigned>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> queue;
        queue.emplace(0, origin);
        // Connected, clear space should not pay a Manhattan-grid distance tax.
        // Feet and torso checks prevent shortcuts through walls or low banks.
        if (m_scene && m_settings) {
            for (unsigned f = 0; f < m_floors.size(); ++f) {
                if (f == origin || !environment_spaces_compatible(m_floors[origin], m_floors[f])) {
                    continue;
                }
                const auto delta = add(m_floors[f].position, mul(m_floors[origin].position, -1));
                const float length = std::sqrt(dot(delta, delta));
                if (length > reach) {
                    continue;
                }
                bool clear = true;
                for (float height : {.15f, 1.25f}) {
                    auto a = m_floors[origin].position, b = m_floors[f].position;
                    a[2] += height;
                    b[2] += height;
                    if (!environment_segment_clear(*m_scene, a, b, *m_settings)) {
                        clear = false;
                        break;
                    }
                }
                if (clear) {
                    distances[f] = length;
                    visible[f] = true;
                    queue.emplace(length, f);
                }
            }
        }
        while (!queue.empty()) {
            const auto [distance, current] = queue.top();
            queue.pop();
            if (distance > distances[current]) {
                continue;
            }
            for (auto next : m_floors[current].neighbours) {
                if (!environment_spaces_compatible(m_floors[origin], m_floors[next])) {
                    continue;
                }
                const auto delta =
                    add(m_floors[next].position, mul(m_floors[current].position, -1));
                const float trial = distance + std::sqrt(dot(delta, delta));
                if (trial <= reach && trial < distances[next]) {
                    distances[next] = trial;
                    queue.emplace(trial, next);
                }
            }
        }
    }
    // Extra open-space reach requires a direct, feet-and-torso-clear view.
    // Graph paths around corners retain the original locality bound.
    return distances[receiver] > m_radius + m_overlap && !visible[receiver] ? INFINITY
                                                                            : distances[receiver];
}

bool EnvironmentNeighbourhoods::contains(unsigned origin, unsigned receiver, bool blending) {
    return distance(origin, receiver) <= radius(origin) + (blending ? overlap(origin) : 0) + .001f;
}

std::vector<unsigned> EnvironmentNeighbourhoods::local(
    unsigned origin, std::span<const unsigned> candidates
) {
    std::vector<unsigned> result;
    for (auto id : candidates) {
        if (distance(origin, id) <= placement_radius(origin) + .001f) {
            result.push_back(id);
        }
    }
    return result;
}

EnvironmentAssignments assign_environment_receivers(
    std::span<const EnvironmentFloor> floors,
    std::span<const EnvironmentProbe> probes,
    std::span<const unsigned> origins,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    EnvironmentAssignments result(probes.size());
    for (unsigned f = 0; f < floors.size(); ++f) {
        for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
            const auto p = environment_receiver_position(floors[f], h);
            float nearest = INFINITY;
            std::vector<std::pair<float, unsigned>> candidates;
            for (unsigned i = 0; i < probes.size(); ++i) {
                if (!environment_spaces_compatible(floors[origins[i]], floors[f])
                    || !neighbourhoods.contains(origins[i], f, true)
                    || environment_probe_coverage(probes[i], p) < kEnvironmentUsefulCoverage) {
                    continue;
                }
                const float distance = neighbourhoods.distance(origins[i], f);
                nearest = std::min(nearest, distance);
                candidates.emplace_back(distance, i);
            }
            for (auto [distance, i] : candidates) {
                if (distance <= nearest + neighbourhoods.blend_width()) {
                    result[i].push_back(f * unsigned(kEnvironmentReceiverHeights.size()) + h);
                }
            }
        }
    }
    return result;
}
}  // namespace openrac::lighting::bake_detail
