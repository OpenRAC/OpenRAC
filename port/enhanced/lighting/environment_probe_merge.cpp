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

float area(unsigned receiver, std::span<const EnvironmentFloor> floors) {
    return floors[receiver / 3].area;
}

float area_sum(std::span<const unsigned> receivers, std::span<const EnvironmentFloor> floors) {
    float result = 0;

    for (auto id : receivers) {
        result += area(id, floors);
    }

    return result;
}

bool valid_box(const EnvironmentProbe& probe) {
    const auto centre = probe.frame.to_local(probe.position);

    if (!probe.frame.valid()) {
        return false;
    }

    for (unsigned a = 0; a < 3; ++a) {
        if (!std::isfinite(probe.minimum[a]) || !std::isfinite(probe.maximum[a])
            || !std::isfinite(probe.projection_minimum[a])
            || !std::isfinite(probe.projection_maximum[a]) || probe.minimum[a] >= centre[a]
            || probe.maximum[a] <= centre[a] || probe.projection_minimum[a] >= centre[a]
            || probe.projection_maximum[a] <= centre[a]) {
            return false;
        }
    }

    return true;
}

// Repair the closest neighbours first; a farther merge must not sidestep the original cluster.
bool preferred_pair(const Pair& candidate, const Pair& previous) {
    if (candidate.close != previous.close) {
        return candidate.close;
    }

    if (candidate.close && candidate.distance_squared != previous.distance_squared) {
        return candidate.distance_squared < previous.distance_squared;
    }

    return candidate.overlap > previous.overlap;
}

// Keeps representative lighting and coverage bounds when original space records share one owner.
void combine_diagnostic(
    EnvironmentSpaceDiagnostic& target, const EnvironmentSpaceDiagnostic& source
) {
    const unsigned cells = target.cells + source.cells;

    // The first fragment supplies the representative authored identity and medium.
    if (target.cells == 0) {
        target = source;
        target.neighbours.clear();
        return;
    }

    target.light =
        mul(add(mul(target.light, float(target.cells)), mul(source.light, float(source.cells))),
            1.f / float(cells));

    for (unsigned axis = 0; axis < 3; ++axis) {
        target.minimum[axis] = std::min(target.minimum[axis], source.minimum[axis]);
        target.maximum[axis] = std::max(target.maximum[axis], source.maximum[axis]);
    }

    // A shared air-side space can contain both dry floors and a water surface.
    if (target.water_height != source.water_height) {
        target.water_height.reset();
    }

    target.cells = cells;
}

EnvironmentProbeMerger::EnvironmentProbeMerger(
    const EnvironmentMergeInputs& inputs,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentPlacementReport& report
)
    : m_scene(inputs.scene),
      m_floors(inputs.floors),
      m_rooms(inputs.rooms),
      m_settings(inputs.settings),
      m_options(inputs.options),
      m_probes(probes),
      m_origins(origins),
      m_report(report),
      m_active(probes.size(), true),
      m_versions(probes.size()),
      m_supported(probes.size()),
      m_counts(report.receivers.size()),
      m_room_parent(inputs.rooms.size()) {
    std::iota(m_room_parent.begin(), m_room_parent.end(), 0u);

    for (unsigned r = 0; r < report.receivers.size(); ++r) {
        m_total_area += area(r, inputs.floors);

        for (auto p : report.receivers[r].probes) {
            m_supported[p].push_back(r);
            ++m_counts[r];
        }
    }
}

void EnvironmentProbeMerger::run() {
    if (std::find(m_counts.begin(), m_counts.end(), 0u) != m_counts.end()) {
        return;
    }

    while (const auto pair = next_pair()) {
        m_tested.emplace(pair->a, m_versions[pair->a], pair->b, m_versions[pair->b]);
        ++m_report.merge_pair_attempts;
        const auto joined = join_close_rooms(*pair);
        const auto replacement = search(*pair);
        m_report.merge_position_trials += replacement.trials;
        bool repaired = false;

        if (replacement.floor != ~0u) {
            apply(*pair, replacement);
            repaired = true;
        } else if (pair->close) {
            repaired = repack(*pair) || repack_pair(*pair);
        }

        // Failed searches restore the original lighting-space boundary for subsequent pairs.
        if (joined) {
            finish_join(*joined, repaired);
        }
    }

    compact();
    compact_rooms();
}

std::optional<RoomJoin> EnvironmentProbeMerger::join_close_rooms(const Pair& pair) {
    const unsigned a = m_floors[m_origins[pair.a]].room;
    const unsigned b = m_floors[m_origins[pair.b]].room;

    if (!pair.close || a == b) {
        return {};
    }

    RoomJoin change{a, b, m_rooms[a].size(), std::move(m_rooms[b])};
    m_rooms[a].insert(m_rooms[a].end(), change.moved.begin(), change.moved.end());

    for (auto floor : change.moved) {
        m_floors[floor].room = a;
    }

    return change;
}

void EnvironmentProbeMerger::finish_join(const RoomJoin& change, bool repaired) {
    if (!repaired) {
        m_rooms[change.destination].resize(change.previous_size);
        m_rooms[change.source] = change.moved;

        for (auto floor : change.moved) {
            m_floors[floor].room = change.source;
        }

        return;
    }

    for (auto& parent : m_room_parent) {
        if (parent == change.source) {
            parent = change.destination;
        }
    }

    // Other captures in the joined domains must be reconsidered against the new candidate set.
    for (unsigned i = 0; i < m_origins.size(); ++i) {
        if (m_floors[m_origins[i]].room == change.destination) {
            ++m_versions[i];
        }
    }
}

std::optional<Pair> EnvironmentProbeMerger::candidate_pair(unsigned a, unsigned b) const {
    const auto& fa = m_floors[m_origins[a]];
    const auto& fb = m_floors[m_origins[b]];

    if (fa.underwater != fb.underwater || std::abs(fa.position[2] - fb.position[2]) > 2) {
        return {};
    }

    Pair pair{a, b, 0, {}};
    const auto delta = add(m_probes[a].position, mul(m_probes[b].position, -1));
    pair.distance_squared = dot(delta, delta);
    pair.close = pair.distance_squared
                     < m_options.minimum_capture_spacing * m_options.minimum_capture_spacing
                 && environment_capture_context_clear(
                     m_scene, fa, m_probes[a].position, fb, m_probes[b].position, m_settings
                 );

    // Different fragment IDs cannot bypass the hard spacing rule in one visible pocket.
    if (fa.room != fb.room && !pair.close) {
        return {};
    }

    if (!pair.close && !similar_environment_capture_light(fa.light, fb.light, m_options)) {
        return {};
    }

    std::set_union(
        m_supported[a].begin(),
        m_supported[a].end(),
        m_supported[b].begin(),
        m_supported[b].end(),
        std::back_inserter(pair.targets)
    );

    if (pair.targets.empty() || (!pair.close && pair.targets.size() < 12)) {
        return {};
    }

    std::vector<unsigned> shared;
    std::set_intersection(
        m_supported[a].begin(),
        m_supported[a].end(),
        m_supported[b].begin(),
        m_supported[b].end(),
        std::back_inserter(shared)
    );
    pair.overlap = area_sum(shared, m_floors) / area_sum(pair.targets, m_floors);

    if (!pair.close && pair.overlap < m_options.merge_overlap) {
        return {};
    }

    return pair;
}

std::optional<Pair> EnvironmentProbeMerger::next_pair() const {
    std::optional<Pair> best;

    for (unsigned a = 0; a < m_probes.size(); ++a) {
        if (!m_active[a]) {
            continue;
        }

        for (unsigned b = a + 1; b < m_probes.size(); ++b) {
            if (!m_active[b] || m_tested.contains({a, m_versions[a], b, m_versions[b]})) {
                continue;
            }

            const auto pair = candidate_pair(a, b);

            // Only eligible pairs participate in the deterministic best-first ranking.
            if (!pair) {
                continue;
            }

            if (!best || preferred_pair(*pair, *best)) {
                best = pair;
            }
        }
    }

    return best;
}

void EnvironmentProbeMerger::apply(const Pair& pair, const Replacement& replacement) {
    m_report.probe_merges.push_back(
        {m_probes[pair.a].position,
         m_probes[pair.b].position,
         replacement.probe.position,
         pair.overlap,
         replacement.missed_area / area_sum(pair.targets, m_floors),
         replacement.trials}
    );

    for (auto r : m_supported[pair.a]) {
        --m_counts[r];
    }

    for (auto r : m_supported[pair.b]) {
        --m_counts[r];
    }

    for (auto r : replacement.supported) {
        ++m_counts[r];
    }

    m_probes[pair.a] = replacement.probe;
    m_origins[pair.a] = replacement.floor;
    m_supported[pair.a] = replacement.supported;
    m_supported[pair.b].clear();
    m_active[pair.b] = false;
    ++m_versions[pair.a];
}

void EnvironmentProbeMerger::compact() {
    m_report.tolerated_receivers.clear();
    float missing_area = 0;

    for (unsigned r = 0; r < m_counts.size(); ++r) {
        m_report.receivers[r].probes.clear();

        if (!m_counts[r]) {
            m_report.tolerated_receivers.push_back(r);
            missing_area += area(r, m_floors);
        }
    }

    m_report.covered_fraction = 1 - missing_area / m_total_area;
    unsigned write = 0;

    for (unsigned i = 0; i < m_probes.size(); ++i) {
        if (!m_active[i]) {
            continue;
        }

        for (auto r : m_supported[i]) {
            m_report.receivers[r].probes.push_back(write);
        }

        m_probes[write] = m_probes[i];
        m_origins[write] = m_origins[i];
        m_report.owners[write] = m_floors[m_origins[i]].room;
        ++write;
    }

    m_probes.resize(write);
    m_origins.resize(write);
    m_report.owners.resize(write);
}

std::vector<EnvironmentSpaceDiagnostic> EnvironmentProbeMerger::room_diagnostics(
    std::span<const unsigned> remap, unsigned count
) const {
    // Direct merger fixtures need no room diagnostic records.
    if (m_report.spaces.size() != m_room_parent.size()) {
        return {};
    }

    std::vector<EnvironmentSpaceDiagnostic> result(count);

    for (unsigned old = 0; old < m_report.spaces.size(); ++old) {
        const unsigned room = remap[m_room_parent[old]];
        const auto& source = m_report.spaces[old];
        auto& target = result[room];
        combine_diagnostic(target, source);

        for (auto neighbour : source.neighbours) {
            const unsigned other = remap[m_room_parent[neighbour]];

            if (other != room) {
                target.neighbours.push_back(other);
            }
        }
    }

    for (auto& space : result) {
        std::sort(space.neighbours.begin(), space.neighbours.end());
        space.neighbours.erase(
            std::unique(space.neighbours.begin(), space.neighbours.end()), space.neighbours.end()
        );
    }

    return result;
}

void EnvironmentProbeMerger::compact_rooms() {
    // Ordinary same-space merges do not alter the floor map or its diagnostics.
    if (std::none_of(m_rooms.begin(), m_rooms.end(), [](const auto& room) {
            return room.empty();
        })) {
        return;
    }

    std::vector<unsigned> remap(m_rooms.size(), ~0u);
    std::vector<std::vector<unsigned>> rooms;

    for (unsigned old = 0; old < m_rooms.size(); ++old) {
        if (!m_rooms[old].empty()) {
            remap[old] = static_cast<unsigned>(rooms.size());
            rooms.push_back(std::move(m_rooms[old]));
        }
    }

    m_report.spaces = room_diagnostics(remap, static_cast<unsigned>(rooms.size()));
    m_report.lighting_spaces = static_cast<unsigned>(rooms.size());

    for (auto& floor : m_floors) {
        floor.room = remap[floor.room];
    }

    for (unsigned i = 0; i < m_origins.size(); ++i) {
        m_report.owners[i] = m_floors[m_origins[i]].room;
    }

    m_rooms = std::move(rooms);
}

void merge_environment_probes(
    const EnvironmentMergeInputs& inputs,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentPlacementReport& report
) {
    if (!inputs.options.merge_enabled || probes.size() < 2
        || report.receivers.size() != inputs.floors.size() * 3) {
        return;
    }

    EnvironmentProbeMerger(inputs, probes, origins, report).run();
}

}  // namespace openrac::lighting::bake_detail
