// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements reflection capture replacement for environment_probe_merge_internal.h.

#include <algorithm>
#include <iterator>
#include <limits>
#include <numeric>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_probe_merge_internal.h"

namespace openrac::lighting::bake_detail {

bool EnvironmentProbeMerger::separated_from_active(
    const EnvironmentProbe& probe, unsigned origin, std::span<const unsigned> excluded
) const {
    const auto& floor = m_floors[origin];

    for (unsigned other = 0; other < m_probes.size(); ++other) {
        if (std::find(excluded.begin(), excluded.end(), other) != excluded.end()
            || !m_active[other]) {
            continue;
        }

        const auto& other_floor = m_floors[m_origins[other]];

        if (other_floor.underwater != floor.underwater
            || std::abs(other_floor.position[2] - floor.position[2]) > 2) {
            continue;
        }

        const auto delta = add(probe.position, mul(m_probes[other].position, -1));

        if (dot(delta, delta)
                < m_options.minimum_capture_spacing * m_options.minimum_capture_spacing
            && environment_capture_context_clear(
                m_scene, floor, probe.position, other_floor, m_probes[other].position, m_settings
            )) {
            return false;
        }
    }

    return true;
}

Replacement EnvironmentProbeMerger::relocation(const Pair& pair, unsigned moving) {
    Replacement chosen;
    chosen.quality = std::numeric_limits<float>::infinity();
    std::vector<unsigned> required;

    for (auto r : m_supported[moving]) {
        if (m_counts[r] == 1) {
            required.push_back(r);
        }
    }

    for (auto origin : candidates(pair)) {
        const auto& floor = m_floors[origin];
        const auto probe = environment_capture_box(
            m_scene, floor, m_rooms[floor.room], m_floors, m_settings, m_options
        );
        ++m_report.merge_position_trials;

        if (!valid_box(probe)) {
            continue;
        }

        const std::array<unsigned, 1> excluded{moving};

        // A single move must respect every other active capture, including its paired neighbour.
        if (!separated_from_active(probe, origin, excluded)) {
            continue;
        }

        auto accepted = support(probe, origin);

        if (!accepted
            || !std::includes(
                accepted->begin(), accepted->end(), required.begin(), required.end()
            )) {
            continue;
        }

        const auto delta = add(probe.position, mul(m_probes[moving].position, -1));
        const float quality = std::sqrt(dot(delta, delta)) - .1f * floor.clearance;

        if (quality < chosen.quality) {
            chosen.quality = quality;
            chosen.floor = origin;
            chosen.probe = probe;
            chosen.supported = std::move(*accepted);
        }
    }

    return chosen;
}

bool EnvironmentProbeMerger::repack(const Pair& pair) {
    unsigned moving = ~0u;
    Replacement chosen;
    chosen.quality = std::numeric_limits<float>::infinity();

    // Preserve the original candidate order when equal-quality positions are available.
    for (unsigned id : {pair.a, pair.b}) {
        auto location = relocation(pair, id);

        if (location.floor != ~0u && location.quality < chosen.quality) {
            moving = id;
            chosen = std::move(location);
        }
    }

    // A failed relocation leaves the original capture and coverage counts intact.
    if (moving == ~0u) {
        return false;
    }

    move_capture(moving, chosen);
    return true;
}

bool EnvironmentProbeMerger::separated_from_others(
    const Pair& pair, const EnvironmentProbe& probe, unsigned origin
) const {
    const auto& floor = m_floors[origin];

    for (unsigned other = 0; other < m_probes.size(); ++other) {
        if (other == pair.a || other == pair.b || !m_active[other]) {
            continue;
        }

        const auto& neighbour = m_floors[m_origins[other]];
        const auto delta = add(probe.position, mul(m_probes[other].position, -1));

        if (floor.underwater == neighbour.underwater
            && std::abs(floor.position[2] - neighbour.position[2]) <= 2
            && dot(delta, delta)
                   < m_options.minimum_capture_spacing * m_options.minimum_capture_spacing
            && environment_capture_context_clear(
                m_scene, floor, probe.position, neighbour, m_probes[other].position, m_settings
            )) {
            return false;
        }
    }

    return true;
}

void EnvironmentProbeMerger::move_capture(unsigned id, const Replacement& location) {
    m_report.capture_moves.push_back({m_probes[id].position, location.probe.position});

    for (auto r : m_supported[id]) {
        --m_counts[r];
    }

    for (auto r : location.supported) {
        ++m_counts[r];
    }

    m_probes[id] = location.probe;
    m_origins[id] = location.floor;
    m_supported[id] = location.supported;
    ++m_versions[id];
}

std::vector<Replacement> EnvironmentProbeMerger::pair_locations(const Pair& pair, Float3 centre) {
    const auto& room = m_rooms[m_floors[m_origins[pair.a]].room];
    std::vector<std::pair<float, unsigned>> ordered;

    for (auto id : room) {
        const auto delta = add(m_floors[id].position, mul(centre, -1));
        ordered.emplace_back(dot(delta, delta), id);
    }

    std::sort(ordered.begin(), ordered.end());
    std::vector<Replacement> locations;

    for (unsigned i = 0; i < std::min(128u, unsigned(ordered.size())); ++i) {
        const auto id = ordered[i].second;
        const auto& floor = m_floors[id];

        if (!floor.capture_candidate) {
            continue;
        }

        const auto probe =
            environment_capture_box(m_scene, floor, room, m_floors, m_settings, m_options);
        ++m_report.merge_position_trials;

        if (!valid_box(probe) || !separated_from_others(pair, probe, id)) {
            continue;
        }

        auto accepted = support(probe, id);

        if (accepted) {
            Replacement location;
            location.floor = id;
            location.probe = probe;
            location.supported = std::move(*accepted);
            locations.push_back(std::move(location));
        }
    }

    return locations;
}

std::array<unsigned, 2> EnvironmentProbeMerger::choose_pair(
    std::span<const Replacement> locations, std::span<const unsigned> required, Float3 centre
) const {
    std::array<unsigned, 2> chosen{~0u, ~0u};
    float best = std::numeric_limits<float>::infinity();

    for (unsigned a = 0; a < locations.size(); ++a) {
        for (unsigned b = a + 1; b < locations.size(); ++b) {
            const auto delta =
                add(locations[a].probe.position, mul(locations[b].probe.position, -1));

            if (dot(delta, delta)
                < m_options.minimum_capture_spacing * m_options.minimum_capture_spacing) {
                continue;
            }

            std::vector<unsigned> covered;
            std::set_union(
                locations[a].supported.begin(),
                locations[a].supported.end(),
                locations[b].supported.begin(),
                locations[b].supported.end(),
                std::back_inserter(covered)
            );

            if (!std::includes(covered.begin(), covered.end(), required.begin(), required.end())) {
                continue;
            }

            const auto da = add(locations[a].probe.position, mul(centre, -1));
            const auto db = add(locations[b].probe.position, mul(centre, -1));
            const float quality = dot(da, da) + dot(db, db);

            if (quality < best) {
                best = quality;
                chosen = {a, b};
            }
        }
    }

    return chosen;
}

bool EnvironmentProbeMerger::repack_pair(const Pair& pair) {
    std::vector<unsigned> required;

    for (auto r : pair.targets) {
        const auto contribution =
            unsigned(std::binary_search(m_supported[pair.a].begin(), m_supported[pair.a].end(), r))
            + unsigned(
                std::binary_search(m_supported[pair.b].begin(), m_supported[pair.b].end(), r)
            );

        if (m_counts[r] == contribution) {
            required.push_back(r);
        }
    }

    const auto centre = mul(add(m_probes[pair.a].position, m_probes[pair.b].position), .5f);
    const auto locations = pair_locations(pair, centre);
    const auto chosen = choose_pair(locations, required, centre);

    if (chosen[0] == ~0u) {
        return false;
    }

    move_capture(pair.a, locations[chosen[0]]);
    move_capture(pair.b, locations[chosen[1]]);
    return true;
}
}  // namespace openrac::lighting::bake_detail
