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

std::vector<unsigned> EnvironmentProbeMerger::candidates(const Pair& pair) const {
    const auto& fa = m_floors[m_origins[pair.a]];
    const auto& fb = m_floors[m_origins[pair.b]];
    const auto middle = mul(add(fa.position, fb.position), .5f);
    std::vector<std::pair<float, unsigned>> ordered;

    for (auto id : m_rooms[fa.room]) {
        const auto& floor = m_floors[id];

        if (!floor.capture_candidate) {
            continue;
        }

        const auto d = add(floor.position, mul(middle, -1));

        if (d[0] * d[0] + d[1] * d[1] > (pair.close ? m_options.spacing * m_options.spacing : 64)
            || std::abs(d[2]) > 2
            || (!pair.close
                && (!similar_environment_capture_light(floor.light, fa.light, m_options)
                    || !similar_environment_capture_light(floor.light, fb.light, m_options)))) {
            continue;
        }

        ordered.emplace_back(std::sqrt(dot(d, d)) - .25f * floor.clearance, id);
    }

    std::sort(ordered.begin(), ordered.end());
    std::vector<unsigned> result{m_origins[pair.a]};

    if (m_origins[pair.b] != result.front()) {
        result.push_back(m_origins[pair.b]);
    }

    for (auto [score, id] : ordered) {
        if (result.size() == (pair.close ? 128u : kCandidateLimit)) {
            break;
        }

        if (std::find(result.begin(), result.end(), id) == result.end()) {
            result.push_back(id);
        }
    }

    return result;
}

std::optional<std::vector<unsigned>> EnvironmentProbeMerger::support(
    const EnvironmentProbe& probe, unsigned origin
) const {
    // Receivers, rather than visibility of old capture centres, define equivalence.
    // A supported replacement may see the complete required area around an occluder
    // without seeing a previous camera point.
    if (!valid_box(probe)) {
        return {};
    }

    std::vector<unsigned> result;

    for (unsigned r = 0; r < m_report.receivers.size(); ++r) {
        const auto p = m_report.receivers[r].position;

        if (environment_probe_coverage(probe, p) < kEnvironmentUsefulCoverage) {
            continue;
        }

        if (!environment_spaces_compatible(m_floors[origin], m_floors[r / 3])) {
            return {};
        }

        if (environment_receiver_supported(probe, m_floors[origin], m_floors[r / 3], p)) {
            result.push_back(r);
        }
    }

    return result;
}

std::optional<std::pair<float, float>> EnvironmentProbeMerger::replacement_coverage(
    const ReplacementSearch& search, std::span<const unsigned> accepted
) const {
    std::vector<bool> visible(m_counts.size());

    for (auto r : accepted) {
        visible[r] = true;
    }

    float missing_area = 0;

    for (auto r : search.pair.targets) {
        if (!visible[r]) {
            missing_area += area(r, m_floors);
        }
    }

    if (!search.pair.close
        && missing_area > m_options.merge_loss_fraction * search.target_area + 1e-5f) {
        return {};
    }

    std::vector<unsigned> misses, newly_lost;

    for (unsigned r = 0; r < m_counts.size(); ++r) {
        if (m_counts[r] - search.contribution[r] + unsigned(visible[r]) != 0) {
            continue;
        }

        misses.push_back(r);

        if (m_counts[r]) {
            newly_lost.push_back(r);
        }
    }

    // Reflection influence must cover every playable receiver, including floor-level samples.
    if (!misses.empty()
        || !environment_merge_loss_allowed(
            misses,
            newly_lost,
            {m_report.receivers,
             m_floors,
             search.target_area,
             m_total_area,
             m_options,
             search.pair.close}
        )) {
        return {};
    }

    return std::pair{area_sum(newly_lost, m_floors), missing_area};
}

void EnvironmentProbeMerger::consider(
    ReplacementSearch& search, unsigned origin, const EnvironmentFloor& floor
) const {
    ++search.best.trials;
    const auto probe = environment_capture_box(
        m_scene, floor, m_rooms[floor.room], m_floors, m_settings, m_options
    );
    auto accepted = support(probe, origin);

    if (!accepted) {
        return;
    }

    const auto coverage = replacement_coverage(search, *accepted);

    // Reject a replacement before ranking when it would leave unsupported receivers.
    if (!coverage) {
        return;
    }

    const auto d = add(probe.position, mul(search.middle, -1));
    const float quality = std::sqrt(dot(d, d)) - .25f * m_floors[origin].clearance;
    const auto [loss, missing_area] = *coverage;

    if (search.best.floor == ~0u
        || std::tie(loss, missing_area, quality)
               < std::tie(search.best.new_loss, search.best.missed_area, search.best.quality)) {
        search.best.floor = origin;
        search.best.probe = probe;
        search.best.supported = std::move(*accepted);
        search.best.new_loss = loss;
        search.best.missed_area = missing_area;
        search.best.quality = quality;
    }
}

void EnvironmentProbeMerger::consider_subcell(
    ReplacementSearch& search, unsigned origin, float dx, float dy
) const {
    auto start = m_floors[origin].position;
    start[0] += dx;
    start[1] += dy;
    start[2] += 1;
    const auto ground = m_scene.trace(start, {0, 0, -1}, m_settings.ray_bias, 2);

    if (ground.hit.geomID == RTC_INVALID_GEOMETRY_ID
        || std::abs(unit({ground.hit.Ng_x, ground.hit.Ng_y, ground.hit.Ng_z})[2]) < .65f) {
        return;
    }

    auto floor = m_floors[origin];
    floor.position = start;
    floor.position[2] -= ground.ray.tfar;
    auto eye = floor.position;
    eye[2] += .05f;
    const auto roof = m_scene.trace(eye, {0, 0, 1}, m_settings.ray_bias, 3.4f);

    for (float height : {3.f, 3.2f, 2.8f}) {
        const float h = std::min(height, roof.ray.tfar - .05f);

        if (h < 2.55f) {
            continue;
        }

        floor.capture = floor.position;
        floor.capture[2] += h;
        consider(search, origin, floor);
    }
}

void EnvironmentProbeMerger::consider_orientations(
    ReplacementSearch& search, std::span<const unsigned> locations
) const {
    for (auto origin : locations) {
        auto floor = m_floors[origin];

        for (auto frame :
             {m_probes[search.pair.a].frame, m_probes[search.pair.b].frame, ProbeFrame{}}) {
            if (frame.axis == m_floors[origin].frame.axis) {
                continue;
            }

            floor.frame = frame;
            consider(search, origin, floor);
        }
    }
}

void EnvironmentProbeMerger::consider_subcells(
    ReplacementSearch& search, std::span<const unsigned> locations
) const {
    for (unsigned n = 0; n < std::min(24u, unsigned(locations.size())); ++n) {
        const auto origin = locations[n];

        for (float dx : {-.5f, 0.f, .5f}) {
            for (float dy : {-.5f, 0.f, .5f}) {
                consider_subcell(search, origin, dx, dy);
            }
        }
    }
}

Replacement EnvironmentProbeMerger::search(const Pair& pair) const {
    ReplacementSearch search{
        pair,
        std::vector<unsigned>(m_counts.size()),
        mul(add(m_probes[pair.a].position, m_probes[pair.b].position), .5f),
        area_sum(pair.targets, m_floors),
        {}
    };

    for (auto r : m_supported[pair.a]) {
        ++search.contribution[r];
    }

    for (auto r : m_supported[pair.b]) {
        ++search.contribution[r];
    }

    const auto locations = candidates(pair);

    for (auto origin : locations) {
        consider(search, origin, m_floors[origin]);
    }

    if (pair.close && search.best.floor == ~0u) {
        // A junction can have several valid medial tangents. Try either contributor's
        // axis, then an unrotated box, before requiring another capture. Ownership and
        // receiver visibility remain identical checks; this changes only the box fit.
        consider_orientations(search, locations);
    }

    if (pair.close && (search.best.floor == ~0u || search.best.new_loss > 0)) {
        // A one-unit floor lattice can miss the clear position between nearby occluders.
        // Try a bounded subcell search, always rechecking the real floor and head clearance.
        consider_subcells(search, locations);
    }

    return search.best;
}
}  // namespace openrac::lighting::bake_detail
