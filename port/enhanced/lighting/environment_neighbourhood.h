// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <algorithm>
#include <map>

#include "enhanced/lighting/environment_floor_map.h"

namespace openrac::lighting::bake_detail {
// Bounded floor distances with collision-checked visibility shortcuts. This
// lighting-only graph never adds gameplay traversal edges.
class EnvironmentNeighbourhoods {
public:
    EnvironmentNeighbourhoods(
        std::span<const EnvironmentFloor> floors,
        float radius,
        const Scene* scene = nullptr,
        const BakeSettings* settings = nullptr
    );
    float distance(unsigned origin, unsigned receiver);
    bool contains(unsigned origin, unsigned receiver, bool blending = false);
    std::vector<unsigned> local(unsigned origin, std::span<const unsigned> candidates);

    float radius() const { return m_radius; }

    float radius(unsigned origin) const {
        return std::max(m_radius, m_floors[origin].locality_radius);
    }

    float overlap() const { return m_overlap; }

    float overlap(unsigned origin) const { return std::min(4.f, radius(origin) * .15f); }

    float blend_width() const { return std::min(1.5f, m_radius * .1f); }

    // Leave room for the diagonal of a fitted rectangular footprint and its apron.
    float placement_radius(unsigned origin) const { return radius(origin) * .70710678f; }

private:
    std::span<const EnvironmentFloor> m_floors;
    float m_radius, m_overlap;
    std::map<unsigned, std::vector<float>> m_distances;
    std::map<unsigned, std::vector<bool>> m_visible;
    const Scene* m_scene;
    const BakeSettings* m_settings;
};

using EnvironmentAssignments =
    std::vector<std::vector<unsigned>>;  // Floor index * 3 + receiver height.
EnvironmentAssignments assign_environment_receivers(
    std::span<const EnvironmentFloor> floors,
    std::span<const EnvironmentProbe> probes,
    std::span<const unsigned> origins,
    EnvironmentNeighbourhoods& neighbourhoods
);
}  // namespace openrac::lighting::bake_detail
