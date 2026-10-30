// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Gameplay reachability and lighting-space segmentation for reflection captures.
#include "enhanced/lighting/environment_floor_map.h"

#include <queue>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_gi.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
// Large brightness/hue changes are room boundaries even when floors connect through an opening.
bool similar_environment_light(Float3 a, Float3 b) {
    const float la = a[0] + a[1] + a[2], lb = b[0] + b[1] + b[2];

    if (std::max(la, lb) > 8 * std::min(la, lb) + .5f) {
        return false;
    }

    if (std::min(la, lb) < .03f) {
        return true;
    }

    float difference = 0;

    for (unsigned axis = 0; axis < 3; ++axis) {
        difference += std::abs(a[axis] / la - b[axis] / lb);
    }

    return difference < 1.f;
}

namespace {
// Half-edge receivers exercise the blend boundaries between the original two-unit floor samples.
std::vector<EnvironmentFloorSample> refine_floor_map(
    std::span<const EnvironmentFloorSample> input
) {
    std::vector<EnvironmentFloorSample> result(input.begin(), input.end());

    for (unsigned a = 0; a < input.size(); ++a) {
        for (auto b : input[a].neighbours) {
            if (b >= input.size()) {
                throw std::runtime_error("invalid floor graph neighbour");
            }

            if (b <= a) {
                continue;
            }

            const auto p = mul(add(input[a].position, input[b].position), .5f);
            const auto id = static_cast<unsigned>(result.size());
            auto sample = input[a];
            sample.position = p;
            sample.anchor = false;
            sample.neighbours = {a, b};

            if (input[a].water_height && input[b].water_height) {
                sample.water_height = (*input[a].water_height + *input[b].water_height) * .5f;
            }

            result.push_back(std::move(sample));

            if (input[a].height_bounds && input[b].height_bounds) {
                result.back().height_bounds = Float2{
                    std::min((*input[a].height_bounds)[0], (*input[b].height_bounds)[0]),
                    std::max((*input[a].height_bounds)[1], (*input[b].height_bounds)[1])
                };
            }

            result[a].neighbours.push_back(id);
            result[b].neighbours.push_back(id);
        }
    }

    return result;
}

// Floor support and capture clearance are independent: a low passage still needs influence coverage.
std::optional<Float3> supported(const Scene& scene, Float3& floor, const BakeSettings& settings) {
    floor[2] += .75f;
    const auto hit = scene.trace(floor, {0, 0, -1}, settings.ray_bias, 1.5f);

    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID
        || std::abs(unit({hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z})[2]) < .65f) {
        return {};
    }

    floor[2] -= hit.ray.tfar;
    auto start = floor;
    start[2] += .05f;
    const auto roof =
        scene.trace(start, {0, 0, 1}, settings.ray_bias, kEnvironmentCaptureHeight + .1f);

    // Match the walkability planner's 2.5-unit player clearance, rather than capture clearance.
    if (roof.ray.tfar + .05f < 2.5f) {
        return {};
    }

    const float height = std::min(kEnvironmentCaptureHeight, roof.ray.tfar - .05f);

    auto capture = floor;
    capture[2] += height;
    return capture;
}

std::vector<EnvironmentFloor> playable_floors(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> samples,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport& report
) {
    std::vector<bool> reached(samples.size());
    std::queue<unsigned> pending;

    for (unsigned i = 0; i < samples.size(); ++i) {
        if (samples[i].anchor) {
            reached[i] = true;
            pending.push(i);
        }
    }

    while (!pending.empty()) {
        const auto current = pending.front();
        pending.pop();

        for (auto next : samples[current].neighbours) {
            if (next >= samples.size()) {
                throw std::runtime_error("invalid floor graph neighbour");
            }

            if (!reached[next]) {
                reached[next] = true;
                pending.push(next);
            }
        }
    }

    const CaptureGi field(volumes);
    std::vector<EnvironmentFloor> result;
    report.input_floors = static_cast<unsigned>(samples.size());

    for (unsigned i = 0; i < samples.size(); ++i) {
        if (!reached[i]) {
            ++report.rejected_floors;
            continue;
        }

        auto ground = samples[i].position;
        const auto position = samples[i].swimming_surface
                                  ? std::optional<Float3>{Float3{
                                        ground[0], ground[1], ground[2] + kEnvironmentCaptureHeight
                                    }}
                                  : supported(scene, ground, settings);

        if (!position) {
            ++report.unsupported_floors;
            report.unsupported.push_back(samples[i].position);
            continue;
        }

        EnvironmentFloor floor;
        floor.capture = *position;
        floor.position = ground;
        floor.water_height = samples[i].water_height;
        floor.underwater = samples[i].underwater;

        if (floor.water_height) {
            if (floor.underwater) {
                floor.capture[2] = std::min(floor.capture[2], *floor.water_height - .3f);
                floor.receiver_height =
                    std::clamp(*floor.water_height - ground[2] - .5f, .1f, 2.5f);
            } else if (samples[i].swimming_surface) {
                auto start = ground;
                start[2] += .05f;
                const auto roof = scene.trace(start, {0, 0, 1}, settings.ray_bias, 3.1f);
                floor.capture[2] = std::min(floor.capture[2], start[2] + roof.ray.tfar - .1f);
                floor.receiver_height = std::min(1.25f, std::max(.1f, roof.ray.tfar - .25f));
            }
        }

        floor.capture_candidate = floor.capture[2] - ground[2] >= 2.55f;

        if (floor.water_height) {
            floor.capture_candidate = floor.capture[2] - ground[2] >= .25f;
        }

        if (!floor.capture_candidate) {
            ++report.receiver_only_floors;
        }

        floor.source = i;
        floor.lighting_labels = samples[i].lighting_labels;
        floor.clearance = 8;
        unsigned walls = 0;

        for (unsigned ray = 0; ray < 8; ++ray) {
            const float angle = float(ray) * kPi / 4;
            auto eye = floor.position;
            eye[2] += 1.25f;
            const auto hit =
                scene.trace(eye, {std::cos(angle), std::sin(angle), 0}, settings.ray_bias, 24);
            floor.clearance = std::min(floor.clearance, hit.ray.tfar);
            walls += hit.hit.geomID != RTC_INVALID_GEOMETRY_ID;
        }

        const bool roof =
            scene.occluded(floor.capture, {0, 0, 1}, settings.ray_bias, options.box_distance);
        floor.sky_band = samples[i].sky <= .1875f && roof && walls >= 5 ? 0 : 1;
        auto receiver = floor.position;
        receiver[2] += 1.25f;
        floor.light = field.sample(receiver, {0, 0, 1}, {0, 0, 1}, options);
        result.push_back(floor);
    }

    report.playable_floors = static_cast<unsigned>(result.size());
    return result;
}

}  // namespace

EnvironmentFloorMap environment_floor_map(
    const Scene& scene,
    std::span<const ProbeVolume> volumes,
    std::span<const EnvironmentFloorSample> input,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport& report
) {
    // Capture validation refines the floor graph, but orientation comes from the same
    // unrefined analysis as GI fitting and its debug view. Extra validation samples must
    // not change corridor axes.
    const auto samples = connected_room_floors(scene, input, settings, &report);
    const auto alignment = room_layout(scene, samples, settings);
    const auto refined = refine_floor_map(samples);
    EnvironmentFloorMap result;
    result.floors = playable_floors(scene, volumes, refined, settings, options, report);
    // Original nodes and edge midpoints share the horizontal footprint of the source lattice.
    std::vector<float> edge_areas;

    for (unsigned a = 0; a < samples.size(); ++a) {
        for (auto b : samples[a].neighbours) {
            if (b <= a) {
                continue;
            }

            const auto d = add(samples[a].position, mul(samples[b].position, -1));
            const float squared = d[0] * d[0] + d[1] * d[1];

            if (squared > .01f) {
                edge_areas.push_back(squared);
            }
        }
    }

    float footprint = 1;

    if (!edge_areas.empty()) {
        const auto middle = edge_areas.begin() + edge_areas.size() / 2;
        std::nth_element(edge_areas.begin(), middle, edge_areas.end());
        footprint = *middle * .25f;
    }

    for (auto& floor : result.floors) {
        floor.area = footprint * (floor.source < samples.size() ? 1.f : 1.5f);
    }

    std::vector<EnvironmentFloorSample> supported;
    std::vector<unsigned> index(refined.size(), ~0u);

    for (unsigned a = 0; a < result.floors.size(); ++a) {
        auto sample = refined[result.floors[a].source];
        sample.position = result.floors[a].position;
        sample.neighbours.clear();
        index[result.floors[a].source] = a;
        supported.push_back(std::move(sample));
    }

    for (unsigned a = 0; a < supported.size(); ++a) {
        for (auto b : refined[result.floors[a].source].neighbours) {
            if (index[b] != ~0u) {
                supported[a].neighbours.push_back(index[b]);
            }
        }
    }

    connect_environment_floor_fragments(scene, supported, result.floors, settings, options, report);
    auto geometry = room_layout(scene, supported, settings, false, true);
    result.rooms = geometry.rooms;

    for (unsigned a = 0; a < result.floors.size(); ++a) {
        result.floors[a].room = geometry.owners[a];
        result.floors[a].clearance = geometry.clearance[a];
    }

    merge_environment_spaces(
        scene,
        settings,
        result.floors,
        refined,
        result.rooms,
        report,
        options.minimum_space_area,
        options
    );
    geometry.rooms = result.rooms;

    for (unsigned room = 0; room < result.rooms.size(); ++room) {
        result.centres.push_back(geometry.centre(supported, room));

        for (auto a : result.rooms[room]) {
            const auto source = result.floors[a].source;
            auto original = source;

            if (original >= samples.size()) {
                const auto& ends = refined[source].neighbours;
                original = ends.front();

                for (auto candidate : ends) {
                    const auto p =
                        add(samples[candidate].position, mul(result.floors[a].position, -1));
                    const auto q =
                        add(samples[original].position, mul(result.floors[a].position, -1));

                    if (dot(p, p) < dot(q, q)) {
                        original = candidate;
                    }
                }
            }

            result.floors[a].frame = alignment.frame(
                samples,
                alignment.owners[original],
                result.floors[a].position,
                std::max(8.f, options.spacing)
            );
        }
    }

    report.lighting_spaces = static_cast<unsigned>(result.rooms.size());
    return result;
}
}  // namespace openrac::lighting::bake_detail
