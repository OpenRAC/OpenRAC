// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/room_layout.h"

#include <algorithm>
#include <limits>
#include <numbers>
#include <queue>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned kMissing = ~0u;
constexpr float kCoreClearance = 1.5f;

bool connected(std::span<const EnvironmentFloorSample> floors, unsigned a, unsigned b) {
    return floors[a].region == floors[b].region && floors[a].underwater == floors[b].underwater
           && (floors[a].swimming_surface == floors[b].swimming_surface || floors[a].underwater)
           && (floors[a].underwater
               || std::abs(floors[a].position[2] - floors[b].position[2]) <= 2);
}
}  // namespace

RoomLayout prepare_room_layout(
    const BakeScene& source,
    std::vector<EnvironmentFloorSample>& floors,
    const BakeSettings& settings,
    unsigned threads
) {
    const profiling::Zone timing("Prepare room layout");

    Scene scene(source, settings.sunlight_direction, threads);
    floors = connected_room_floors(scene, floors, settings);
    return room_layout(scene, floors, settings);
}

RoomLayout room_layout(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    bool build_medial,
    bool split_doorways
) {
    RoomLayout result;
    result.owners.resize(floors.size(), kMissing);
    result.clearance.resize(floors.size(), 12);
    result.ceilings.resize(floors.size(), std::numeric_limits<float>::infinity());
    result.height_bounds.resize(floors.size());

    for (unsigned a = 0; a < floors.size(); ++a) {
        result.height_bounds[a] = floors[a].height_bounds.value_or(
            Float2{floors[a].position[2] - .8f, floors[a].position[2] + 3.8f}
        );

        if (floors[a].underwater && floors[a].water_height) {
            result.height_bounds[a][1] =
                std::min(result.height_bounds[a][1], *floors[a].water_height - .25f);
        }

        auto eye = floors[a].position;
        eye[2] += 1.25f;
        result.clearance[a] = room_wall_distance(scene, eye).distance;
        // Match the original height extension: a locally continuous roof plus surrounding
        // enclosure is evidence of an interior. Tall walls alone never raise the analysis space.
        const auto roof = scene.trace(eye, {0, 0, 1}, settings.ray_bias, 64);

        if (roof.hit.geomID != RTC_INVALID_GEOMETRY_ID && roof.ray.tfar >= 1.5f
            && floors[a].sky <= .1875f) {
            unsigned covered = 1, walls = 0;

            for (auto shift : {Float2{-.5f, 0}, Float2{.5f, 0}, Float2{0, -.5f}, Float2{0, .5f}}) {
                auto start = eye;
                start[0] += shift[0];
                start[1] += shift[1];
                const auto hit = scene.trace(start, {0, 0, 1}, settings.ray_bias, 64);
                covered += hit.hit.geomID != RTC_INVALID_GEOMETRY_ID
                           && std::abs(hit.ray.tfar - roof.ray.tfar) <= 4;
            }

            for (unsigned ray = 0; ray < 8; ++ray) {
                const float angle = float(ray) * std::numbers::pi_v<float> / 4;
                walls += scene.occluded(
                    eye, {std::cos(angle), std::sin(angle), 0}, settings.ray_bias, 32
                );
            }

            if (covered >= 4 && walls >= 5) {
                result.ceilings[a] = eye[2] + roof.ray.tfar;
                result.height_bounds[a][1] = std::max(
                    result.height_bounds[a][1],
                    std::min(result.height_bounds[a][1] + 32, result.ceilings[a])
                );
            }
        }

        for (auto b : floors[a].neighbours) {
            if (b >= floors.size()) {
                throw std::runtime_error("invalid room floor edge");
            }
        }
    }

    // Wide floor cores cannot connect through a narrow doorway. Local exposure changes have no
    // role here: a patch of foliage or skylight must not create a separate reflection room.
    auto flood = [&](unsigned first, bool cores) {
        const auto id = static_cast<unsigned>(result.rooms.size());
        result.rooms.emplace_back();
        std::queue<unsigned> pending;
        pending.push(first);
        result.owners[first] = id;

        while (!pending.empty()) {
            const auto a = pending.front();
            pending.pop();
            result.rooms.back().push_back(a);

            for (auto b : floors[a].neighbours) {
                if (result.owners[b] != kMissing || !connected(floors, a, b)
                    || (cores && result.clearance[b] < kCoreClearance)) {
                    continue;
                }

                result.owners[b] = id;
                pending.push(b);
            }
        }
    };
    const bool partition_cores =
        split_doorways || std::any_of(floors.begin(), floors.end(), [](const auto& floor) {
            return !floor.lighting_labels.has_value();
        });

    // A continuous swimming volume is not partitioned by its uneven basin floor or shoreline clutter.
    for (unsigned a = 0; a < floors.size(); ++a) {
        if (result.owners[a] == kMissing && floors[a].swimming_surface) {
            flood(a, false);
        }
    }

    if (partition_cores) {
        for (unsigned a = 0; a < floors.size(); ++a) {
            if (result.owners[a] == kMissing && result.clearance[a] >= kCoreClearance) {
                flood(a, true);
            }
        }
    }

    // Grow all cores simultaneously into boundary strips and corridor approaches.
    std::queue<unsigned> pending;

    for (unsigned a = 0; a < floors.size(); ++a) {
        if (result.owners[a] != kMissing) {
            pending.push(a);
        }
    }

    while (!pending.empty()) {
        const auto a = pending.front();
        pending.pop();

        for (auto b : floors[a].neighbours) {
            if (result.owners[b] != kMissing || !connected(floors, a, b)) {
                continue;
            }

            result.owners[b] = result.owners[a];
            result.rooms[result.owners[a]].push_back(b);
            pending.push(b);
        }
    }

    // A narrow pipe with no wide core is still one connected playable space.
    for (unsigned a = 0; a < floors.size(); ++a) {
        if (result.owners[a] == kMissing) {
            flood(a, false);
        }
    }

    if (build_medial) {
        result.rebuild_medial(scene, floors, settings);
    }

    return result;
}

unsigned RoomLayout::centre(std::span<const EnvironmentFloorSample> floors, unsigned id) const {
    const auto& room = rooms.at(id);
    Float3 mean{};
    float weight = 0;

    for (auto a : room) {
        const float w = 1;
        mean = add(mean, mul(floors[a].position, w));
        weight += w;
    }

    mean = mul(mean, 1 / weight);
    unsigned best = room.front();
    float best_score = -std::numeric_limits<float>::infinity();

    for (auto a : room) {
        const auto delta = add(floors[a].position, mul(mean, -1));
        // Once comfortably clear of walls, prefer the room centre instead of chasing
        // the largest open-sky radius at an outdoor edge.
        const float score = std::min(clearance[a], 8.f) - .25f * std::sqrt(dot(delta, delta));

        if (score > best_score) {
            best = a;
            best_score = score;
        }
    }

    return best;
}

ProbeFrame RoomLayout::frame(
    std::span<const EnvironmentFloorSample> /*floors*/, unsigned room, Float3 position, float radius
) const {
    float nearest = radius * radius;
    ProbeFrame result;

    for (const auto edge : medial_edges) {
        if (medial_owners[edge[0]] != room) {
            continue;
        }

        const auto a = medial_points[edge[0]], b = medial_points[edge[1]];
        const auto delta = add(b, mul(a, -1));
        const float length_squared = delta[0] * delta[0] + delta[1] * delta[1];

        if (length_squared < .01f) {
            continue;
        }

        const auto relative = add(position, mul(a, -1));
        const float t = std::clamp(dot(relative, delta) / dot(delta, delta), 0.f, 1.f);
        const auto distance = add(relative, mul(delta, -t));
        const float score = dot(distance, distance);

        if (score < nearest) {
            nearest = score;
            const float length = std::sqrt(length_squared);
            result.axis = {delta[0] / length, delta[1] / length};
        }
    }

    return result;
}
}  // namespace openrac::lighting::bake_detail
