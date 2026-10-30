// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Consolidate small floor patches without joining rooms across lighting or enclosure boundaries.
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_probe_merge.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned kMissing = ~0u;
constexpr unsigned kTinyCells = 12;

// Both collision reachability and visible body/head clearance must agree on an open boundary.
bool open_boundary(
    const Scene& scene,
    const BakeSettings& settings,
    const EnvironmentFloor& a,
    const EnvironmentFloor& b,
    bool preserve_sky = true
) {
    if ((preserve_sky && a.sky_band != b.sky_band) || std::abs(a.position[2] - b.position[2]) > 2) {
        return false;
    }

    for (float height : {1.25f, 2.5f}) {
        auto p = a.position, q = b.position;
        p[2] = q[2] = std::max(a.position[2], b.position[2]) + height;
        const auto delta = add(q, mul(p, -1));
        const float distance = std::sqrt(dot(delta, delta));

        if (distance > .01f
            && scene.occluded(
                p,
                mul(delta, 1 / distance),
                settings.ray_bias,
                std::max(settings.ray_bias, distance - .05f)
            )) {
            return false;
        }
    }

    return true;
}

// Report a stable representative colour without modifying any floor or GI sample.
Float3 median_light(std::span<const unsigned> cells, std::span<const EnvironmentFloor> floors) {
    Float3 result{};

    for (unsigned axis = 0; axis < 3; ++axis) {
        std::vector<float> values;

        for (auto id : cells) {
            values.push_back(floors[id].light[axis]);
        }

        std::sort(values.begin(), values.end());
        const auto middle = values.size() / 2;
        result[axis] = (values[middle] + values[(values.size() - 1) / 2]) * .5f;
    }

    return result;
}

struct Boundary {
    bool direct = false;
};

// A rejected capture location must not sever a valid collision-floor connection. Bridge at most
// one missing sample, within three units, and recheck visible geometry and authored lighting.
std::map<unsigned, Boundary> room_boundaries(
    const Scene& scene,
    const BakeSettings& settings,
    std::span<const unsigned> fragment,
    std::span<const EnvironmentFloor> floors,
    std::span<const EnvironmentFloorSample> samples,
    std::span<const unsigned> index,
    bool small_patch = false
) {
    std::map<unsigned, Boundary> result;

    for (auto a : fragment) {
        const auto authored = samples[floors[a].source].region;
        auto accept = [&](unsigned source, bool gap) {
            const auto b = index[source];

            if (b == kMissing || floors[b].room == floors[a].room
                || floors[b].underwater != floors[a].underwater
                || samples[source].region != authored) {
                return;
            }

            auto light_a = floors[a].light, light_b = floors[b].light;

            if (small_patch) {
                // Small patches cannot establish a brightness/sky boundary by themselves. Keep
                // hue and direct visibility. Tight enclosed patches retain doorway protection;
                // exposed slivers beside a wall do not qualify as separate rooms.
                if (gap
                    || (floors[a].sky_band == 0
                        && std::min(floors[a].clearance, floors[b].clearance) < 1.25f)) {
                    return;
                }

                const float la = light_a[0] + light_a[1] + light_a[2];
                const float lb = light_b[0] + light_b[1] + light_b[2];

                if (std::min(la, lb) >= .03f) {
                    light_a = mul(light_a, 1 / la);
                    light_b = mul(light_b, 1 / lb);
                } else {
                    light_a = light_b = {};
                }
            }

            if (!similar_environment_light(light_a, light_b)) {
                return;
            }

            const auto d = add(floors[a].position, mul(floors[b].position, -1));
            const bool authored_mood = samples[floors[a].source].lighting_labels.has_value()
                                       && samples[source].lighting_labels.has_value();

            if ((gap && dot(d, d) > 9)
                || !open_boundary(
                    scene, settings, floors[a], floors[b], !small_patch && !authored_mood
                )) {
                return;
            }

            result[floors[b].room].direct |= !gap;
        };

        for (auto source : samples[floors[a].source].neighbours) {
            if (index[source] != kMissing) {
                accept(source, false);
                continue;
            }

            if (samples[source].region != authored) {
                continue;
            }

            for (auto next : samples[source].neighbours) {
                accept(next, true);
            }
        }
    }

    return result;
}
}  // namespace

void merge_environment_spaces(
    const Scene& scene,
    const BakeSettings& settings,
    std::vector<EnvironmentFloor>& floors,
    std::span<const EnvironmentFloorSample> samples,
    std::vector<std::vector<unsigned>>& rooms,
    EnvironmentPlacementReport& report,
    float minimum_space_area,
    const EnvironmentBakeSettings& options
) {
    report.spaces_before_merge = static_cast<unsigned>(rooms.size());
    report.merged_fragments = report.merged_graph_gaps = 0;
    report.spaces.clear();
    std::vector<unsigned> index(samples.size(), kMissing);

    for (unsigned i = 0; i < floors.size(); ++i) {
        index[floors[i].source] = i;
    }

    auto area = [&](unsigned room) {
        float result = 0;

        for (auto id : rooms[room]) {
            result += floors[id].area;
        }

        return result;
    };
    bool changed = true;
    const bool known_moods = std::all_of(samples.begin(), samples.end(), [](const auto& sample) {
        return sample.lighting_labels.has_value();
    });

    while (changed) {
        changed = false;

        for (unsigned room = 0; room < rooms.size(); ++room) {
            if (rooms[room].empty()) {
                continue;
            }

            const bool substantial =
                rooms[room].size() >= kTinyCells && area(room) >= minimum_space_area;

            if (substantial && !known_moods) {
                continue;
            }

            const auto patch_area = area(room);
            auto boundaries = room_boundaries(scene, settings, rooms[room], floors, samples, index);

            if (patch_area < minimum_space_area) {
                const auto open =
                    room_boundaries(scene, settings, rooms[room], floors, samples, index, true);

                for (const auto& [other, boundary] : open) {
                    if (area(other) >= std::max(minimum_space_area, 4 * patch_area)) {
                        boundaries[other].direct |= boundary.direct;
                    }
                }
            }

            unsigned target = kMissing;
            bool gap = false;

            for (const auto& [other, boundary] : boundaries) {
                if (substantial
                    && !similar_environment_capture_light(
                        median_light(rooms[room], floors),
                        median_light(rooms[other], floors),
                        options
                    )) {
                    continue;
                }

                if (target == kMissing || rooms[other].size() > rooms[target].size()) {
                    target = other;
                    gap = !boundary.direct;
                }
            }

            if (target == kMissing) {
                continue;
            }

            for (auto id : rooms[room]) {
                floors[id].room = target;
                rooms[target].push_back(id);
            }

            rooms[room].clear();
            changed = true;
            ++report.merged_fragments;
            report.merged_graph_gaps += gap;
        }
    }

    std::erase_if(rooms, [](const auto& room) { return room.empty(); });

    for (unsigned room = 0; room < rooms.size(); ++room) {
        for (auto id : rooms[room]) {
            floors[id].room = room;
        }
    }

    for (const auto& room : rooms) {
        EnvironmentSpaceDiagnostic diagnostic;
        diagnostic.cells = static_cast<unsigned>(room.size());
        const auto& first = floors[room.front()];
        diagnostic.authored_region = samples[first.source].region;
        diagnostic.sky_band = first.sky_band;
        diagnostic.underwater = first.underwater;
        diagnostic.water_height = first.water_height;
        diagnostic.minimum = diagnostic.maximum = first.position;
        diagnostic.light = median_light(room, floors);

        for (auto id : room) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                diagnostic.minimum[axis] =
                    std::min(diagnostic.minimum[axis], floors[id].position[axis]);
                diagnostic.maximum[axis] =
                    std::max(diagnostic.maximum[axis], floors[id].position[axis]);
            }

            for (auto source : samples[floors[id].source].neighbours) {
                const auto other = index[source];

                if (other != kMissing && floors[other].room != first.room) {
                    diagnostic.neighbours.push_back(floors[other].room);
                }
            }
        }

        std::sort(diagnostic.neighbours.begin(), diagnostic.neighbours.end());
        diagnostic.neighbours.erase(
            std::unique(diagnostic.neighbours.begin(), diagnostic.neighbours.end()),
            diagnostic.neighbours.end()
        );
        report.spaces.push_back(std::move(diagnostic));
    }
}
}  // namespace openrac::lighting::bake_detail
