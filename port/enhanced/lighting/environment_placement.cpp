// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Floor-relative reflection captures centred in connected playable spaces.
#include "enhanced/lighting/environment_placement.h"

#include <algorithm>
#include <limits>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_placement_internal.h"
#include "enhanced/lighting/environment_probe_merge.h"

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
    // Minimum spacing never joins separate reflection media or disconnected vertical layers.
    if (a.underwater != b.underwater || std::abs(a.position[2] - b.position[2]) > 2
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

        if (!floor.underwater && other.underwater && other.water_height
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
        for (unsigned a = 0; a < 3; ++a) {
            const auto point = floor.frame.to_local(floors[id].position);
            lo[a] = std::min(lo[a], point[a]);
            hi[a] = std::max(hi[a], point[a]);
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
            std::max(lo[a] - (a == 2 ? .8f : footprint_padding), capture[a] - options.box_distance);
        probe.maximum[a] = std::min(
            hi[a] + (a == 2 ? kEnvironmentCaptureHeight + .8f : footprint_padding),
            capture[a] + options.box_distance
        );
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

unsigned centre_floor(std::span<const unsigned> room, std::span<const EnvironmentFloor> floors) {
    Float3 centre{};

    for (auto id : room) {
        centre = add(centre, mul(floors[id].position, 1.f / float(room.size())));
    }

    unsigned selected = ~0u;
    float best = -std::numeric_limits<float>::infinity();

    for (auto id : room) {
        // Tight passages remain receivers but must not place a capture inside their ceiling.
        if (!floors[id].capture_candidate) {
            continue;
        }

        const auto d = add(floors[id].position, mul(centre, -1));
        const float score = floors[id].clearance - .25f * std::sqrt(dot(d, d));

        if (score > best) {
            best = score;
            selected = id;
        }
    }

    return selected;
}

// Every space receives its central capture before any space adds supplementary captures.
void cover_room(
    const Scene& scene,
    std::span<const unsigned> room,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    EnvironmentPlacementReport& report,
    std::vector<unsigned>& origins
) {
    std::vector<unsigned> remaining(room.begin(), room.end());
    std::erase_if(remaining, [&](unsigned id) {
        for (unsigned i = 0; i < probes.size(); ++i) {
            const auto& source = floors[origins[i]];
            const auto d = add(floors[id].capture, mul(probes[i].position, -1));

            if (environment_spaces_compatible(source, floors[id])
                && dot(d, d) <= options.spacing * options.spacing
                && covers_floor(probes[i], source, floors[id])) {
                return true;
            }
        }
        return false;
    });

    while (!remaining.empty()) {
        const auto selected = centre_floor(remaining, floors);

        if (selected == ~0u) {
            return;  // Final validation requires a nearby compatible capture to cover this passage.
        }

        const auto probe =
            environment_capture_box(scene, floors[selected], room, floors, settings, options);

        if (probes.size() >= options.maximum_probes) {
            return;  // The final per-receiver validation reports any actual remaining holes.
        }

        probes.push_back(probe);
        origins.push_back(selected);
        report.owners.push_back(floors[selected].room);
        bool progressed = false;
        std::erase_if(remaining, [&](unsigned id) {
            auto delta = add(floors[id].capture, mul(probe.position, -1));
            const bool covered = std::sqrt(dot(delta, delta)) <= options.spacing
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

// Any capture, including the initial centre, is removable only when every
// supported receiver still has another compatible, visible capture with a useful blend weight.
void prune_probes(
    std::span<const EnvironmentFloor> floors,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentPlacementReport& report
) {
    report.unpruned_probes = static_cast<unsigned>(probes.size());
    std::vector<unsigned> counts(floors.size() * kEnvironmentReceiverHeights.size());
    std::vector<std::vector<unsigned>> supported(probes.size());

    for (unsigned i = 0; i < probes.size(); ++i) {
        const auto& source = floors[origins[i]];

        for (unsigned f = 0; f < floors.size(); ++f) {
            if (!environment_spaces_compatible(source, floors[f])) {
                continue;
            }

            for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
                const auto p = environment_receiver_position(floors[f], h);

                if (!environment_receiver_supported(probes[i], source, floors[f], p)) {
                    continue;
                }

                const auto receiver =
                    f * static_cast<unsigned>(kEnvironmentReceiverHeights.size()) + h;
                supported[i].push_back(receiver);
                ++counts[receiver];
            }
        }
    }

    // Leave an incomplete plan intact for the final diagnostic pass.
    if (std::find(counts.begin(), counts.end(), 0u) != counts.end()) {
        return;
    }

    std::vector<unsigned> order;

    for (unsigned i = 0; i < probes.size(); ++i) {
        order.push_back(i);
    }

    std::sort(order.begin(), order.end(), [&](unsigned a, unsigned b) {
        if (supported[a].size() != supported[b].size()) {
            return supported[a].size() < supported[b].size();
        }
        return a > b;
    });
    std::vector<bool> keep(probes.size(), true);

    for (auto i : order) {
        if (std::any_of(supported[i].begin(), supported[i].end(), [&](unsigned receiver) {
                return counts[receiver] < 2;
            })) {
            continue;
        }

        keep[i] = false;

        for (auto receiver : supported[i]) {
            --counts[receiver];
        }

        ++report.pruned_probes;
    }

    report.receivers.resize(counts.size());

    for (unsigned f = 0; f < floors.size(); ++f) {
        for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
            auto& receiver = report.receivers[f * kEnvironmentReceiverHeights.size() + h];
            receiver.position = environment_receiver_position(floors[f], h);
            receiver.height = h;
        }
    }

    unsigned write = 0;

    for (unsigned i = 0; i < probes.size(); ++i) {
        if (!keep[i]) {
            continue;
        }

        for (auto receiver : supported[i]) {
            report.receivers[receiver].probes.push_back(write);
        }

        probes[write] = probes[i];
        origins[write] = origins[i];
        report.owners[write] = report.owners[i];
        ++write;
    }

    probes.resize(write);
    origins.resize(write);
    report.owners.resize(write);
}
}  // namespace

std::vector<EnvironmentProbe> place_environment_probes(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport* output
) {
    EnvironmentPlacementReport local;
    auto& report = output ? *output : local;
    report = {};

    if (floors.empty()) {
        throw std::runtime_error(
            "environment placement requires gameplay-anchored floor inference"
        );
    }

    auto plan = environment_floor_map(scene, volumes, floors, settings, options, report);
    auto& playable = plan.floors;
    auto& spaces = plan.rooms;
    std::vector<EnvironmentProbe> result;
    std::vector<unsigned> origins;

    for (unsigned room_id = 0; room_id < spaces.size(); ++room_id) {
        const auto& room = spaces[room_id];

        if (result.size() >= options.maximum_probes) {
            break;
        }

        auto selected = plan.centres[room_id];

        if (!playable[selected].capture_candidate) {
            selected = centre_floor(room, playable);
        }

        if (selected == ~0u) {
            continue;  // Receiver-only spaces must be covered by a compatible neighbouring capture.
        }

        result.push_back(
            environment_capture_box(scene, playable[selected], room, playable, settings, options)
        );
        origins.push_back(selected);
        report.owners.push_back(playable[selected].room);
    }

    for (const auto& room : spaces) {
        cover_room(scene, room, playable, settings, options, result, report, origins);
    }

    prune_probes(playable, result, origins, report);
    merge_environment_probes({scene, playable, spaces, settings, options}, result, origins, report);

    for (unsigned a = 0; a < result.size(); ++a) {
        for (unsigned b = a + 1; b < result.size(); ++b) {
            // Room labels cannot exempt close captures from the final spacing check.
            if (playable[origins[a]].underwater != playable[origins[b]].underwater
                || std::abs(playable[origins[a]].position[2] - playable[origins[b]].position[2])
                       > 2) {
                continue;
            }

            const auto delta = add(result[a].position, mul(result[b].position, -1));

            if (dot(delta, delta)
                >= options.minimum_capture_spacing * options.minimum_capture_spacing) {
                continue;
            }

            if (environment_capture_context_clear(
                    scene,
                    playable[origins[a]],
                    result[a].position,
                    playable[origins[b]],
                    result[b].position,
                    settings
                )) {
                report.spacing_conflicts.push_back({a, b});
            } else {
                ++report.occlusion_separated_pairs;
            }
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

            for (unsigned i = 0; i < result.size(); ++i) {
                const float weight = environment_probe_coverage(result[i], p);

                if (weight < kEnvironmentUsefulCoverage) {
                    continue;
                }

                if (!environment_spaces_compatible(playable[origins[i]], floor)) {
                    ++report.ownership_conflicts;
                } else if (environment_receiver_supported(
                               result[i], playable[origins[i]], floor, p
                           )) {
                    coverage = std::max(coverage, weight);
                }
            }

            report.minimum_coverage = std::min(report.minimum_coverage, coverage);

            if (coverage < kEnvironmentUsefulCoverage) {
                report.uncovered.push_back(p);
                report.uncovered_owners.push_back(floor.room);
            }
        }
    }

    if (result.empty() || !report.uncovered.empty() || report.ownership_conflicts != 0
        || !report.spacing_conflicts.empty()) {
        throw std::runtime_error(
            "environment placement incomplete: " + std::to_string(report.uncovered.size())
            + " uncovered samples, " + std::to_string(report.ownership_conflicts)
            + " ownership conflicts, " + std::to_string(report.spacing_conflicts.size())
            + " unresolved reflection spacing conflicts; " + std::to_string(result.size())
            + " captures, safety limit " + std::to_string(options.maximum_probes)
        );
    }

    report.coverage_hash = environment_capture_hash(result);
    return result;
}
}  // namespace openrac::lighting::bake_detail
