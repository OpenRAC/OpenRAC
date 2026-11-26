// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Select photographs independently of the shape/number of their influence regions.
#include <algorithm>
#include <limits>
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_placement_internal.h"

namespace openrac::lighting::bake_detail {
std::vector<EnvironmentProbe> select_environment_captures(
    const Scene& scene,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::span<const EnvironmentFloor> floors,
    std::span<const EnvironmentProbe> coverage_regions,
    std::span<const unsigned> coverage_origins,
    EnvironmentNeighbourhoods& neighbourhoods,
    EnvironmentPlacementReport& report,
    std::vector<EnvironmentInfluence>& influences
) {
    std::vector<EnvironmentProbe> regions(coverage_regions.begin(), coverage_regions.end());
    std::vector<unsigned> origins(coverage_origins.begin(), coverage_origins.end());
    const unsigned coverage_count = unsigned(regions.size());
    auto region_owners = report.owners;
    const auto scale = [&](unsigned f) {
        return std::max(
            {floors[f].clearance, floors[f].locality_radius, settings.traversal.player_height}
        );
    };
    // Photograph candidates survive independently of coverage pruning. A compact
    // geometric/lighting domain contributes a safe medoid and a local patch;
    // the quality objective decides whether to pay for that photograph.
    std::map<std::pair<unsigned, unsigned>, std::vector<unsigned>> domains;
    for (unsigned f = 0; f < floors.size(); ++f) {
        const unsigned island = floors[f].island.value_or(floors[f].room);
        if (island != ~0u) {
            domains[{island, floors[f].component}].push_back(f);
        }
    }
    for (const auto& [key, domain] : domains) {
        if (regions.size() >= kEnvironmentProbeLimit) {
            break;
        }
        Float3 centre{};
        double weight = 0;
        for (auto f : domain) {
            const double w = floors[f].area / double(scale(f) * scale(f));
            for (unsigned a = 0; a < 3; ++a) {
                centre[a] += float(w * floors[f].position[a]);
            }
            weight += w;
        }
        if (weight <= 0) {
            continue;
        }
        centre = mul(centre, float(1 / weight));
        unsigned selected = ~0u;
        float best = INFINITY;
        for (auto f : domain) {
            if (!floors[f].capture_candidate) {
                continue;
            }
            const auto d = add(floors[f].position, mul(centre, -1));
            const float error = dot(d, d);
            if (error < best) {
                selected = f;
                best = error;
            }
        }
        if (selected == ~0u
            || std::find(origins.begin(), origins.end(), selected) != origins.end()) {
            continue;
        }
        auto local = neighbourhoods.local(selected, domain);
        std::stable_sort(local.begin(), local.end(), [&](unsigned a, unsigned b) {
            if (a == selected || b == selected) {
                return a == selected && b != selected;
            }
            return neighbourhoods.distance(selected, a) < neighbourhoods.distance(selected, b);
        });
        std::optional<EnvironmentProbe> accepted;
        // A photograph must not disappear just because one box cannot represent
        // its whole domain. Try progressively smaller local neighbourhoods;
        // other regions can still reference the same photograph afterwards.
        while (!local.empty()) {
            auto probe =
                environment_capture_box(scene, floors[selected], local, floors, settings, options);
            EnvironmentAssignments required(1);
            for (auto f : local) {
                if (!environment_spaces_compatible(floors[selected], floors[f])) {
                    continue;
                }
                for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
                    if (environment_probe_coverage(
                            probe, environment_receiver_position(floors[f], h)
                        )
                        >= kEnvironmentUsefulCoverage) {
                        required[0].push_back(f * unsigned(kEnvironmentReceiverHeights.size()) + h);
                    }
                }
            }
            std::vector<EnvironmentProbe> fitted{probe};
            if (!required[0].empty()
                && fit_environment_capture_boxes(
                       scene,
                       floors,
                       settings,
                       options,
                       fitted,
                       std::span(&selected, 1),
                       &required,
                       &neighbourhoods
                )
                       .empty()) {
                accepted = fitted.front();
                break;
            }
            if (local.size() == 1) {
                break;
            }
            local.resize((local.size() + 1) / 2);
        }
        if (!accepted) {
            continue;
        }
        regions.push_back(*accepted);
        origins.push_back(selected);
        region_owners.push_back(key.first);
    }
    const unsigned count = unsigned(regions.size());
    std::vector<std::vector<unsigned>> receivers(count);
    // Check the entire actual box domain, including compatible receivers which
    // were not linked by nearest-owner assignment. Sharing must not introduce leaks.
    for (unsigned r = 0; r < count; ++r) {
        for (unsigned f = 0; f < floors.size(); ++f) {
            for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
                if (environment_probe_coverage(
                        regions[r], environment_receiver_position(floors[f], h)
                    )
                    >= kEnvironmentUsefulCoverage) {
                    receivers[r].push_back(f);
                    break;
                }
            }
        }
    }
    std::vector<std::vector<double>> costs(count, std::vector<double>(count, INFINITY));
    for (unsigned c = 0; c < count; ++c) {
        for (unsigned r = 0; r < count; ++r) {
            if (c != r
                && !environment_capture_visible(
                    scene,
                    floors[origins[c]],
                    regions[c].position,
                    floors[origins[r]],
                    regions[r].position,
                    settings
                )) {
                continue;
            }
            double error = 0;
            bool valid = true;
            for (auto f : receivers[r]) {
                if (!environment_spaces_compatible(floors[origins[c]], floors[f])
                    || !neighbourhoods.contains(origins[c], f, true)) {
                    valid = false;
                    break;
                }
                const double d = neighbourhoods.distance(origins[c], f);
                error += d * d;
            }
            if (valid) {
                costs[c][r] = error / std::max(std::size_t(1), receivers[r].size());
            }
        }
    }
    // Deterministic weighted set cover. Capture count is the primary objective;
    // graph-distance error breaks ties in favour of a central photograph.
    std::vector<bool> covered(count), chosen(count);
    unsigned remaining = coverage_count;
    while (remaining) {
        unsigned best = ~0u;
        std::size_t best_gain = 0;
        double best_error = INFINITY;
        for (unsigned c = 0; c < count; ++c) {
            if (chosen[c]) {
                continue;
            }
            std::size_t gain = 0;
            double error = 0;
            for (unsigned r = 0; r < coverage_count; ++r) {
                if (!covered[r] && std::isfinite(costs[c][r])) {
                    const auto weight = std::max(std::size_t(1), receivers[r].size());
                    gain += weight;
                    error += costs[c][r] * double(weight);
                }
            }
            if (gain > best_gain || (gain && gain == best_gain && error < best_error)) {
                best = c;
                best_gain = gain;
                best_error = error;
            }
        }
        if (best == ~0u) {
            throw std::runtime_error("reflection region has no safe capture");
        }
        chosen[best] = true;
        for (unsigned r = 0; r < coverage_count; ++r) {
            if (!covered[r] && std::isfinite(costs[best][r])) {
                covered[r] = true;
                --remaining;
            }
        }
    }
    // Facility-location refinement: one photograph costs one nominal spacing
    // square; receiver error is area-weighted displacement / enclosure size.
    // Evaluate physical receivers once, so subdividing a region cannot increase
    // its importance or manufacture a reason for another capture.
    std::vector<std::vector<float>> errors(count, std::vector<float>(floors.size(), INFINITY));
    for (unsigned c = 0; c < count; ++c) {
        for (unsigned r = 0; r < count; ++r) {
            if (!std::isfinite(costs[c][r])) {
                continue;
            }
            for (auto f : receivers[r]) {
                const float d = neighbourhoods.distance(origins[c], f) / scale(f);
                errors[c][f] = std::min(errors[c][f], d * d);
            }
        }
    }
    std::vector<float> best_error(floors.size(), INFINITY);
    unsigned photographs = 0;
    for (unsigned c = 0; c < count; ++c) {
        if (!chosen[c]) {
            continue;
        }
        ++photographs;
        for (unsigned f = 0; f < floors.size(); ++f) {
            best_error[f] = std::min(best_error[f], errors[c][f]);
        }
    }
    while (photographs < options.maximum_probes) {
        unsigned best = ~0u;
        double best_gain = 0;
        for (unsigned c = 0; c < count; ++c) {
            if (chosen[c]) {
                continue;
            }
            double gain = 0;
            for (unsigned f = 0; f < floors.size(); ++f) {
                if (errors[c][f] < best_error[f]) {
                    gain += floors[f].area * double(best_error[f] - errors[c][f]);
                }
            }
            const double radius = neighbourhoods.radius(origins[c]);
            const double net_gain = gain - radius * radius;
            if (net_gain > best_gain + 1e-5) {
                best = c;
                best_gain = net_gain;
            }
        }
        if (best == ~0u) {
            break;
        }
        chosen[best] = true;
        ++photographs;
        for (unsigned f = 0; f < floors.size(); ++f) {
            best_error[f] = std::min(best_error[f], errors[best][f]);
        }
    }
    std::vector<EnvironmentProbe> captures;
    std::vector<unsigned> compact(count, ~0u), owners;
    report.influence_owners.clear();
    report.influence_captures.clear();
    report.capture_positions.clear();
    report.submerged_captures.clear();
    for (unsigned c = 0; c < count; ++c) {
        if (!chosen[c]) {
            continue;
        }
        compact[c] = unsigned(captures.size());
        if (floors[origins[c]].underwater) {
            report.submerged_captures.push_back(compact[c]);
        }
        captures.push_back(regions[c]);
        owners.push_back(region_owners[c]);
        report.capture_positions.push_back(regions[c].position);
    }
    influences.clear();
    report.capture_projections.clear();
    for (unsigned r = 0; r < count; ++r) {
        unsigned best = ~0u;
        double error = INFINITY;
        for (unsigned c = 0; c < count; ++c) {
            if (chosen[c] && costs[c][r] < error) {
                best = c;
                error = costs[c][r];
            }
        }
        if (best == ~0u) {
            continue;
        }
        // Optional medoid patches are useful only when a newly considered
        // photograph actually wins them; borrowed copies add no new quality.
        if (r >= coverage_count && best < coverage_count) {
            continue;
        }
        const unsigned capture = compact[best];
        report.influence_captures.push_back(capture);
        report.influence_owners.push_back(region_owners[r]);
        // Retain the region's geometry proxy instead of substituting the chosen
        // photograph's unrelated proxy. Include the complete useful domain so
        // no supported receiver silently falls back to unprojected reflections.
        EnvironmentProjection projection{
            regions[r].frame, regions[r].projection_minimum, regions[r].projection_maximum
        };
        for (unsigned a = 0; a < 3; ++a) {
            // The shader selects influence with an offset receiver but projects
            // from the original surface. Match its half-unit offset bound.
            projection.minimum[a] = std::min(projection.minimum[a], regions[r].minimum[a] - .5f);
            projection.maximum[a] = std::max(projection.maximum[a], regions[r].maximum[a] + .5f);
        }
        influences.push_back(
            {capture, regions[r].frame, regions[r].minimum, regions[r].maximum, projection}
        );
        auto diagnostic = regions[r];
        diagnostic.projection_minimum = projection.minimum;
        diagnostic.projection_maximum = projection.maximum;
        report.capture_projections.push_back(diagnostic);
    }
    // Distance reassignment can make an earlier greedy choice unnecessary.
    // Store/filter only photographs still referenced by a region.
    std::vector<bool> used(captures.size());
    for (const auto& region : influences) {
        used[region.capture] = true;
    }
    std::vector<unsigned> remap(captures.size());
    unsigned write = 0;
    for (unsigned i = 0; i < captures.size(); ++i) {
        if (!used[i]) {
            continue;
        }
        remap[i] = write;
        captures[write] = captures[i];
        owners[write] = owners[i];
        ++write;
    }
    captures.resize(write);
    owners.resize(write);
    for (auto& region : influences) {
        region.capture = remap[region.capture];
    }
    for (auto& id : report.influence_captures) {
        id = remap[id];
    }
    std::erase_if(report.submerged_captures, [&](unsigned id) { return !used[id]; });
    for (auto& id : report.submerged_captures) {
        id = remap[id];
    }
    report.capture_positions.clear();
    for (const auto& capture : captures) {
        report.capture_positions.push_back(capture.position);
    }
    std::vector<EnvironmentProbe> influence_boxes;
    for (const auto& region : influences) {
        EnvironmentProbe box;
        box.frame = region.frame;
        box.minimum = region.minimum;
        box.maximum = region.maximum;
        influence_boxes.push_back(box);
    }
    for (auto& receiver : report.receivers) {
        receiver.probes.clear();
        for (unsigned r = 0; r < influences.size(); ++r) {
            if (environment_probe_coverage(influence_boxes[r], receiver.position)
                >= kEnvironmentUsefulCoverage) {
                receiver.probes.push_back(influences[r].capture);
            }
        }
        std::sort(receiver.probes.begin(), receiver.probes.end());
        receiver.probes.erase(
            std::unique(receiver.probes.begin(), receiver.probes.end()), receiver.probes.end()
        );
    }
    report.unpruned_probes =
        std::max(report.unpruned_probes, coverage_count) + count - coverage_count;
    report.pruned_probes = report.unpruned_probes - unsigned(influences.size());
    report.capture_bounds.clear();
    for (const auto& region : influences) {
        EnvironmentProbe box;
        box.frame = region.frame;
        box.minimum = region.minimum;
        box.maximum = region.maximum;
        report.capture_bounds.push_back(box);
    }
    report.owners = std::move(owners);

    return captures;
}
}  // namespace openrac::lighting::bake_detail
