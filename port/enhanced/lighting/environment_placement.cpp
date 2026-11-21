// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Floor-relative reflection captures centred in connected playable spaces.
#include "enhanced/lighting/environment_placement.h"

#include <algorithm>
#include <limits>
#include <optional>

#include "common/log.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_placement_internal.h"

namespace openrac::lighting::bake_detail {

bool environment_segment_clear(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
) {
    const auto d = add(b, mul(a, -1));
    const float distance = std::sqrt(dot(d, d));
    return distance < .01f
           || !scene.occluded(
               a,
               mul(d, 1 / distance),
               settings.ray_bias,
               std::max(settings.ray_bias, distance - .05f)
           );
}

bool environment_capture_context_clear(
    const Scene& scene,
    const EnvironmentFloor& a,
    Float3 capture_a,
    const EnvironmentFloor& b,
    Float3 capture_b,
    const BakeSettings& settings
) {
    // Nearby replacement has a stricter same-level requirement than sharing a
    // photograph between independently fitted regions along a ramp.
    return std::abs(a.position[2] - b.position[2]) <= 2
           && environment_capture_visible(scene, a, capture_a, b, capture_b, settings);
}

bool environment_capture_visible(
    const Scene& scene,
    const EnvironmentFloor& a,
    Float3 capture_a,
    const EnvironmentFloor& b,
    Float3 capture_b,
    const BakeSettings& settings
) {
    if (a.underwater != b.underwater
        || (a.underwater && a.water_height && b.water_height
            && std::abs(*a.water_height - *b.water_height) > .25f)) {
        return false;
    }

    if (!environment_segment_clear(scene, capture_a, capture_b, settings)) {
        return false;
    }

    for (float height : {1.25f, 2.5f}) {
        capture_a[2] = a.position[2] + height;
        capture_b[2] = b.position[2] + height;

        if (!environment_segment_clear(scene, capture_a, capture_b, settings)) {
            return false;
        }
    }

    return true;
}

// Placement, pruning and validation must agree on which lighting spaces can share a capture.
bool environment_spaces_compatible(
    const EnvironmentFloor& source, const EnvironmentFloor& receiver
) {
    if (source.component != ~0u && receiver.component != ~0u) {
        const bool same_mood =
            source.lighting_labels && receiver.lighting_labels
            && (source.lighting_labels->empty() || receiver.lighting_labels->empty()
                || *source.lighting_labels == *receiver.lighting_labels);
        return source.underwater == receiver.underwater
               && (source.component == receiver.component || same_mood);
    }
    return source.underwater == receiver.underwater
           && (source.room == receiver.room
               || (source.lighting_labels && receiver.lighting_labels
                   && similar_environment_light(source.light, receiver.light)));
}

bool environment_receiver_supported(
    const EnvironmentProbe& probe,
    const EnvironmentFloor& source,
    const EnvironmentFloor& receiver,
    Float3 position
) {
    // Geometry and doorway checks establish room membership; props must not split its influence.
    return environment_spaces_compatible(source, receiver)
           && environment_probe_coverage(probe, position) >= kEnvironmentUsefulCoverage;
}

namespace {
// A cut must retain the source landing's whole player-height envelope, not only its capture centre.
void clip_receiver(
    EnvironmentProbe& probe, const EnvironmentFloor& floor, const EnvironmentFloor& other, Float3 p
) {
    const auto capture = floor.frame.to_local(floor.capture);
    const auto position = floor.frame.to_local(floor.position);
    const auto other_position = floor.frame.to_local(other.position);
    p = floor.frame.to_local(p);
    std::array<unsigned, 3> axes{0, 1, 2};
    std::stable_sort(axes.begin(), axes.end(), [&](unsigned a, unsigned b) {
        return std::abs(other_position[a] - position[a])
               > std::abs(other_position[b] - position[b]);
    });

    for (unsigned axis : axes) {
        const float separation = p[axis] - capture[axis];

        if (std::abs(separation) < .001f) {
            continue;
        }

        const float plane =
            p[axis] - std::copysign(std::min(.25f, std::abs(separation) * .5f), separation);
        bool keeps_landing = true;

        for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
            const auto receiver = floor.frame.to_local(environment_receiver_position(floor, h));
            keeps_landing = keeps_landing
                            && (separation > 0 ? receiver[axis] + .25f <= plane
                                               : receiver[axis] - .25f >= plane);
        }

        if (!keeps_landing || (separation > 0 ? capture[axis] >= plane : capture[axis] <= plane)) {
            continue;
        }

        if (separation > 0) {
            probe.maximum[axis] = std::min(probe.maximum[axis], plane);
        } else {
            probe.minimum[axis] = std::max(probe.minimum[axis], plane);
        }

        return;
    }
}

// Apply waterline separation before looking for room-boundary planes; enclosed rooms below pools stay air.
void clip_foreign_spaces(
    EnvironmentProbe& probe, const EnvironmentFloor& floor, std::span<const EnvironmentFloor> floors
) {
    for (const auto& other : floors) {
        if (environment_spaces_compatible(floor, other)) {
            continue;
        }

        if (floor.frame.pitch == Float2{1, 0} && floor.frame.roll == Float2{1, 0}
            && !floor.underwater && other.underwater && other.water_height
            && floor.position[2] >= *other.water_height - .01f) {
            probe.minimum[2] = std::max(probe.minimum[2], *other.water_height - .5f);
            continue;
        }

        for (unsigned height = 0; height < kEnvironmentReceiverHeights.size(); ++height) {
            const auto p = environment_receiver_position(other, height);

            if (environment_probe_coverage(probe, p) >= kEnvironmentUsefulCoverage) {
                clip_receiver(probe, floor, other, p);
            }
        }
    }
}
}  // namespace

EnvironmentProbe environment_capture_box(
    const Scene& scene,
    const EnvironmentFloor& floor,
    std::span<const unsigned> room,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options
) {
    const auto capture = floor.frame.to_local(floor.capture);
    const auto position = floor.frame.to_local(floor.position);
    EnvironmentProbe probe{floor.capture, capture, capture};
    probe.frame = floor.frame;
    Float3 lo = position, hi = lo;

    for (auto id : room) {
        for (float height : {-.8f, kEnvironmentCaptureHeight + .8f}) {
            auto p = floors[id].position;
            p[2] += height;
            const auto point = floor.frame.to_local(p);
            for (unsigned a = 0; a < 3; ++a) {
                lo[a] = std::min(lo[a], point[a]);
                hi[a] = std::max(hi[a], point[a]);
            }
        }
    }

    for (unsigned a = 0; a < 3; ++a) {
        for (float sign : {-1.f, 1.f}) {
            Float3 direction{};
            direction[a] = sign;
            const auto hit = scene.trace(
                floor.capture,
                floor.frame.to_world(direction),
                settings.ray_bias,
                options.box_distance
            );
            const float distance = hit.ray.tfar;
            const float bound = capture[a] + sign * distance;

            if (sign < 0) {
                probe.minimum[a] = std::max(bound, lo[a] - (a == 2 ? .8f : 2.f));
            } else {
                probe.maximum[a] = std::min(bound, hi[a] + (a == 2 ? options.box_distance : 2.f));
            }
        }
    }

    probe.projection_minimum = probe.minimum;
    probe.projection_maximum = probe.maximum;

    // Include each floor sample's footprint and a full blend border, not just its centre point.
    float footprint_padding = .8f;

    for (auto id : room) {
        footprint_padding = std::max(footprint_padding, std::sqrt(.5f * floors[id].area) + .75f);
    }

    // Influence covers sloping floors independently of the local parallax geometry proxy.
    for (unsigned a = 0; a < 3; ++a) {
        probe.minimum[a] =
            std::max(lo[a] - (a == 2 ? 0.f : footprint_padding), capture[a] - options.box_distance);
        probe.maximum[a] =
            std::min(hi[a] + (a == 2 ? 0.f : footprint_padding), capture[a] + options.box_distance);
    }

    // Waterline clipping keeps air and submerged capture influence separate without changing the GPU format.
    if (floor.water_height) {
        if (floor.underwater) {
            probe.maximum[2] =
                *floor
                     .water_height;  // Swimming occupies the whole column, not just the basin floor.
            probe.projection_maximum[2] =
                std::min(probe.projection_maximum[2], *floor.water_height);
        } else {
            probe.minimum[2] = std::max(probe.minimum[2], *floor.water_height - .5f);
        }
    }

    clip_foreign_spaces(probe, floor, floors);

    return probe;
}

namespace {
bool covers_floor(
    const EnvironmentProbe& probe, const EnvironmentFloor& source, const EnvironmentFloor& floor
) {
    for (unsigned height = 0; height < kEnvironmentReceiverHeights.size(); ++height) {
        const auto p = environment_receiver_position(floor, height);

        if (!environment_receiver_supported(probe, source, floor, p)) {
            return false;
        }
    }

    return true;
}

// Grow a capture's box, in its own frame, over floor id's receivers. The grown box is clipped
// against foreign spaces again and returned only if it covers the floor and every floor the
// capture covered before.
std::optional<EnvironmentProbe> grow_capture(
    std::span<const EnvironmentFloor> floors,
    unsigned id,
    const EnvironmentBakeSettings& options,
    const EnvironmentProbe& probe,
    const EnvironmentFloor& source,
    unsigned origin,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    const auto& floor = floors[id];

    if (!environment_spaces_compatible(source, floor)) {
        return {};
    }

    auto grown = probe;
    const auto capture = grown.frame.to_local(grown.position);

    for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
        const auto p = grown.frame.to_local(environment_receiver_position(floor, h));

        for (unsigned a = 0; a < 3; ++a) {
            // A full blend border (.75) and a little more around each receiver.
            grown.minimum[a] =
                std::max(std::min(grown.minimum[a], p[a] - .8f), capture[a] - options.box_distance);
            grown.maximum[a] =
                std::min(std::max(grown.maximum[a], p[a] + .8f), capture[a] + options.box_distance);
        }
    }

    clip_foreign_spaces(grown, source, floors);

    if (!covers_floor(grown, source, floor)) {
        return {};
    }

    for (unsigned f = 0; f < floors.size(); ++f) {
        if (neighbourhoods.contains(origin, f, true) && covers_floor(probe, source, floors[f])
            && !covers_floor(grown, source, floors[f])) {
            return {};
        }
    }

    return grown;
}

// Grow the nearest compatible capture within spacing over floor id.
void extend_nearest_capture(
    std::span<const EnvironmentFloor> floors,
    unsigned id,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    std::span<const unsigned> origins,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    std::vector<std::pair<float, unsigned>> nearest;

    for (unsigned i = 0; i < probes.size(); ++i) {
        if (environment_spaces_compatible(floors[origins[i]], floors[id])
            && neighbourhoods.contains(origins[i], id)) {
            nearest.emplace_back(neighbourhoods.distance(origins[i], id), i);
        }
    }

    std::sort(nearest.begin(), nearest.end());

    for (auto [distance, i] : nearest) {
        if (const auto grown = grow_capture(
                floors, id, options, probes[i], floors[origins[i]], origins[i], neighbourhoods
            )) {
            probes[i] = *grown;
            return;
        }
    }
}

unsigned centre_floor(
    std::span<const unsigned> room,
    std::span<const EnvironmentFloor> floors,
    bool low_capture = false,
    float nominal_spacing = 0
) {
    Float3 centre{};

    for (auto id : room) {
        centre = add(centre, mul(floors[id].position, 1.f / float(room.size())));
    }

    unsigned selected = ~0u;
    float best = -std::numeric_limits<float>::infinity();

    for (auto id : room) {
        // Tight passages remain receivers but must not place a capture inside their ceiling.
        if (!floors[id].capture_candidate
            && (!low_capture || floors[id].capture[2] - floors[id].position[2] < .25f)) {
            continue;
        }

        const auto d = add(floors[id].position, mul(centre, -1));
        // Clearance is a safety margin, not a reason to chase an outdoor edge.
        const float reach_gain = std::max(0.f, floors[id].locality_radius - nominal_spacing);
        const float score = std::min(floors[id].clearance, 2.f) + reach_gain - std::sqrt(dot(d, d));

        if (score > best) {
            best = score;
            selected = id;
        }
    }

    return selected;
}

// Add a capture only for floor coverage not already served by an existing capture.
void cover_room(
    const Scene& scene,
    std::span<const unsigned> room,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    EnvironmentPlacementReport& report,
    std::vector<unsigned>& origins,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    std::vector<unsigned> remaining(room.begin(), room.end());
    std::erase_if(remaining, [&](unsigned id) {
        for (unsigned i = 0; i < probes.size(); ++i) {
            const auto& source = floors[origins[i]];
            if (environment_spaces_compatible(source, floors[id])
                && neighbourhoods.contains(origins[i], id)
                && covers_floor(probes[i], source, floors[id])) {
                return true;
            }
        }
        return false;
    });

    bool first_capture = true;
    while (!remaining.empty()) {
        // Secondary captures serve the remaining edge or low-level receivers,
        // rather than duplicating the main room and making its central capture redundant.
        const bool primary_capture = first_capture;
        auto selected = centre_floor(remaining, floors, false, options.spacing);
        std::optional<EnvironmentProbe> central_probe;
        if (first_capture) {
            // Neighbouring captures can cover only one side of an island. Keep its
            // own capture central when that position also serves the uncovered side.
            const auto central = centre_floor(room, floors, false, options.spacing);
            if (central != ~0u) {
                const auto local = neighbourhoods.local(central, room);
                const auto probe = environment_capture_box(
                    scene, floors[central], local, floors, settings, options
                );
                if (std::any_of(remaining.begin(), remaining.end(), [&](unsigned id) {
                        return neighbourhoods.distance(central, id)
                                   <= neighbourhoods.placement_radius(central) + .001f
                               && covers_floor(probe, floors[central], floors[id]);
                    })) {
                    selected = central;
                    central_probe = probe;
                }
            }
            first_capture = false;
        }

        if (selected == ~0u) {
            // Prefer borrowing coverage under a low roof. If this space has no suitable
            // neighbour, a lower camera with checked clearance must still serve its receivers.
            for (auto id : remaining) {
                extend_nearest_capture(floors, id, options, probes, origins, neighbourhoods);
            }
            std::erase_if(remaining, [&](unsigned id) {
                for (unsigned i = 0; i < probes.size(); ++i) {
                    if (neighbourhoods.contains(origins[i], id)
                        && covers_floor(probes[i], floors[origins[i]], floors[id])) {
                        return true;
                    }
                }
                return false;
            });
            if (remaining.empty()) {
                return;
            }
            selected = centre_floor(remaining, floors, true, options.spacing);
            if (selected == ~0u) {
                return;  // Final validation reports genuinely unusable camera space.
            }
        }

        const auto local = neighbourhoods.local(
            selected, primary_capture ? room : std::span<const unsigned>(remaining)
        );
        const auto probe = central_probe
                               ? *central_probe
                               : environment_capture_box(
                                     scene, floors[selected], local, floors, settings, options
                                 );

        if (probes.size() >= options.maximum_probes) {
            return;  // The final per-receiver validation reports any actual remaining holes.
        }

        probes.push_back(probe);
        origins.push_back(selected);
        report.owners.push_back(floors[selected].room);
        bool progressed = false;
        std::erase_if(remaining, [&](unsigned id) {
            const bool covered = neighbourhoods.distance(selected, id)
                                     <= neighbourhoods.placement_radius(selected) + .001f
                                 && covers_floor(probe, floors[selected], floors[id]);
            progressed |= covered;
            return covered;
        });

        if (!progressed) {
            // Different captures can cover different receiver heights; only final validation
            // decides whether this floor remains uncovered after all spaces have been filled.
            probes.pop_back();
            origins.pop_back();
            report.owners.pop_back();
            std::erase(remaining, selected);
        }
    }
}

// Ownership is provisional: trial-remove small captures, reassign their unique
// receivers, and refit the surviving boxes before deciding a capture is needed.
bool prune_probes(
    const Scene& scene,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentPlacementReport& report,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    auto assignments = assign_environment_receivers(floors, probes, origins, neighbourhoods);
    std::vector<unsigned> order;
    for (unsigned i = 0; i < probes.size(); ++i) {
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](unsigned a, unsigned b) {
        if (assignments[a].size() != assignments[b].size()) {
            return assignments[a].size() < assignments[b].size();
        }
        return a > b;
    });
    // Stable IDs retain the preferred central captures when support is equal.
    std::vector<unsigned> ids = order;
    std::sort(ids.begin(), ids.end());
    for (auto id : order) {
        const auto found = std::find(ids.begin(), ids.end(), id);
        const unsigned i = unsigned(found - ids.begin());
        if (remove_environment_capture(
                scene, floors, settings, options, i, probes, origins, assignments, neighbourhoods
            )) {
            report.owners.erase(report.owners.begin() + i);
            ids.erase(found);
        }
    }
    return ids.size() != order.size();
}

}  // namespace

std::vector<EnvironmentProbe> place_environment_probes(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport* output,
    const Scene* gameplay_collision,
    std::vector<EnvironmentInfluence>* influences
) {
    EnvironmentPlacementReport local;
    auto& report = output ? *output : local;
    report = {};

    if (floors.empty()) {
        if (!options.require_full_coverage) {
            return {};
        }
        throw std::runtime_error(
            "environment placement requires gameplay-anchored floor inference"
        );
    }

    auto plan = environment_floor_map(
        scene, volumes, floors, settings, options, report, gameplay_collision
    );
    auto region_options = options;
    // Coverage complexity and photograph storage are independent budgets.
    if (influences) {
        region_options.maximum_probes = kEnvironmentProbeLimit;
    }
    auto& playable = plan.floors;
    auto& spaces = plan.rooms;
    std::vector<EnvironmentProbe> result;
    std::vector<unsigned> origins;
    EnvironmentNeighbourhoods neighbourhoods(
        playable, options.spacing, gameplay_collision ? gameplay_collision : &scene, &settings
    );

    for (const auto& room : spaces) {
        if (room.empty()) {
            continue;
        }
        cover_room(
            scene, room, playable, settings, region_options, result, report, origins, neighbourhoods
        );
    }

    unsigned generated = unsigned(result.size());
    log::info(
        "reflection placement: {} coverage candidates over {} floors; pruning",
        result.size(),
        playable.size()
    );
    while (prune_probes(
        scene, playable, settings, region_options, result, origins, report, neighbourhoods
    )) {
        // A successful refit can make an earlier failed removal feasible.
    }
    for (unsigned iteration = 0; iteration < 12; ++iteration) {
        const auto assignments =
            assign_environment_receivers(playable, result, origins, neighbourhoods);
        const auto failed = fit_environment_capture_boxes(
            scene,
            playable,
            settings,
            region_options,
            result,
            origins,
            &assignments,
            &neighbourhoods
        );
        if (failed.empty()) {
            break;
        }
        bool added = false;
        // A non-convex local receiver patch may not fit one OBB without crossing
        // a wall or reaching too far. Seed its least-served end and repartition.
        for (auto probe : failed) {
            if (result.size() >= region_options.maximum_probes) {
                break;
            }
            unsigned selected = ~0u;
            float best = -INFINITY;
            for (auto receiver : assignments[probe]) {
                const unsigned f = receiver / unsigned(kEnvironmentReceiverHeights.size());
                if (std::find(origins.begin(), origins.end(), f) != origins.end()
                    || playable[f].capture[2] - playable[f].position[2] < .25f) {
                    continue;
                }
                float nearest = INFINITY;
                for (auto origin : origins) {
                    if (environment_spaces_compatible(playable[origin], playable[f])) {
                        nearest = std::min(nearest, neighbourhoods.distance(origin, f));
                    }
                }
                if (!std::isfinite(nearest)) {
                    continue;
                }
                const float score = nearest - (playable[f].capture_candidate ? 0 : options.spacing);
                if (score > best) {
                    best = score;
                    selected = f;
                }
            }
            if (selected == ~0u) {
                continue;
            }
            std::vector<unsigned> candidates;
            for (auto receiver : assignments[probe]) {
                candidates.push_back(receiver / unsigned(kEnvironmentReceiverHeights.size()));
            }
            std::sort(candidates.begin(), candidates.end());
            candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
            const auto local = neighbourhoods.local(selected, candidates);
            result.push_back(environment_capture_box(
                scene, playable[selected], local, playable, settings, options
            ));
            origins.push_back(selected);
            report.owners.push_back(playable[selected].room);
            ++generated;
            added = true;
        }
        if (!added) {
            break;
        }
    }
    // Repair seeds must participate in fitting before pruning. Removing a seed
    // while its initial box is still provisional can repeatedly undo the repair.
    while (prune_probes(
        scene, playable, settings, region_options, result, origins, report, neighbourhoods
    )) {
    }
    report.unpruned_probes = generated;
    report.pruned_probes = generated - unsigned(result.size());
    const auto final_assignments =
        assign_environment_receivers(playable, result, origins, neighbourhoods);
    report.receivers.assign(playable.size() * kEnvironmentReceiverHeights.size(), {});
    for (unsigned f = 0; f < playable.size(); ++f) {
        for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
            auto& receiver = report.receivers[f * unsigned(kEnvironmentReceiverHeights.size()) + h];
            receiver.position = environment_receiver_position(playable[f], h);
            receiver.height = h;
            receiver.water_surface = playable[f].swimming_surface && !playable[f].underwater;
        }
    }
    for (unsigned i = 0; i < final_assignments.size(); ++i) {
        for (auto receiver : final_assignments[i]) {
            report.receivers[receiver].probes.push_back(i);
        }
    }
    for (unsigned i = 0; i < result.size(); ++i) {
        report.capture_positions.push_back(result[i].position);

        if (playable[origins[i]].underwater) {
            report.submerged_captures.push_back(i);
        }
    }

    report.validation_samples =
        report.playable_floors * static_cast<unsigned>(kEnvironmentReceiverHeights.size());
    report.capture_bounds = result;

    for (unsigned f = 0; f < playable.size(); ++f) {
        const auto& floor = playable[f];

        for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
            const auto p = environment_receiver_position(floor, h);
            float coverage = 0;
            EnvironmentCoverageReceiver receiver{p, h, {}};

            for (unsigned i = 0; i < result.size(); ++i) {
                const float weight = environment_probe_coverage(result[i], p);

                if (weight < kEnvironmentUsefulCoverage) {
                    continue;
                }

                const bool compatible = environment_spaces_compatible(playable[origins[i]], floor);
                const bool local_receiver =
                    compatible && neighbourhoods.contains(origins[i], f, true);
                if (!local_receiver) {
                    report.placement_conflicts.push_back(
                        {p,
                         playable[origins[i]].position,
                         i,
                         origins[i],
                         f,
                         neighbourhoods.distance(origins[i], f),
                         neighbourhoods.radius(origins[i]) + neighbourhoods.overlap(origins[i]),
                         !compatible}
                    );
                }
                if (!compatible) {
                    ++report.ownership_conflicts;
                } else if (!local_receiver) {
                    ++report.locality_conflicts;
                } else if (std::binary_search(
                               final_assignments[i].begin(),
                               final_assignments[i].end(),
                               f * unsigned(kEnvironmentReceiverHeights.size()) + h
                           )
                           && environment_receiver_supported(
                               result[i], playable[origins[i]], floor, p
                           )) {
                    coverage = std::max(coverage, weight);
                    if (!options.require_full_coverage) {
                        receiver.probes.push_back(i);
                    }
                }
            }
            if (!options.require_full_coverage) {
                report.receivers.push_back(std::move(receiver));
            }

            report.minimum_coverage = std::min(report.minimum_coverage, coverage);

            if (coverage < kEnvironmentUsefulCoverage) {
                report.uncovered.push_back(p);
                report.uncovered_owners.push_back(floor.room);
            }
        }
    }

    if (report.ownership_conflicts != 0 || report.locality_conflicts != 0
        || !report.spacing_conflicts.empty()
        || (options.require_full_coverage && (result.empty() || !report.uncovered.empty()))) {
        throw std::runtime_error(
            "environment placement incomplete: " + std::to_string(report.uncovered.size())
            + " uncovered samples, " + std::to_string(report.ownership_conflicts)
            + " ownership conflicts, " + std::to_string(report.spacing_conflicts.size())
            + " spacing conflicts, " + std::to_string(report.locality_conflicts)
            + " nonlocal receivers; " + std::to_string(result.size()) + " captures, safety limit "
            + std::to_string(region_options.maximum_probes)
        );
    }

    if (influences) {
        log::info("reflection placement: {} valid regions; selecting photographs", result.size());
        result = select_environment_captures(
            gameplay_collision ? *gameplay_collision : scene,
            settings,
            options,
            playable,
            result,
            origins,
            neighbourhoods,
            report,
            *influences
        );
        if (result.size() > options.maximum_probes) {
            throw std::runtime_error("reflection capture budget exceeded after region planning");
        }
    }
    report.coverage_hash = environment_capture_hash(result);
    return result;
}
}  // namespace openrac::lighting::bake_detail
