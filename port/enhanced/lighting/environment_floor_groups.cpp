// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Reconnect sampled floor fragments before geometry-based reflection room segmentation.
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_floor_map.h"
#include "enhanced/lighting/environment_probe_merge.h"
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr float kGap = 4.01f;  // One missing sample in the two-unit collision-floor lattice.
constexpr float kPhysicalGap =
    12.01f;  // Recover longer sampling gaps only over verified walkable floor.
constexpr float kCoreClearance = 1.5f;  // Same wide-core threshold as room_layout.
using Cell = std::array<int, 3>;

struct Group {
    unsigned parent = 0, count = 0;
    Float3 light{};
    std::optional<std::vector<unsigned>> labels;
};

unsigned root(std::vector<Group>& groups, unsigned id) {
    while (groups[id].parent != id) {
        groups[id].parent = groups[groups[id].parent].parent;
        id = groups[id].parent;
    }

    return id;
}

// An unlabelled patch can inherit a mood, but cannot form a chain between conflicting labels.
bool compatible(const Group& a, const Group& b, const EnvironmentBakeSettings& options) {
    if (!a.labels || !b.labels
        || (!a.labels->empty() && !b.labels->empty() && *a.labels != *b.labels)) {
        return false;
    }

    return similar_environment_capture_light(
        mul(a.light, 1.f / float(a.count)), mul(b.light, 1.f / float(b.count)), options
    );
}

void join(std::vector<Group>& groups, unsigned a, unsigned b) {
    if (a > b) {
        std::swap(a, b);
    }

    groups[b].parent = a;
    groups[a].count += groups[b].count;
    groups[a].light = add(groups[a].light, groups[b].light);

    if (groups[a].labels->empty()) {
        groups[a].labels = groups[b].labels;
    }
}

Cell bin(Float3 p, float gap) {
    return {int(std::floor(p[0] / gap)), int(std::floor(p[1] / gap)), int(std::floor(p[2] / gap))};
}

// Point visibility alone could bridge a crack, a lower storey or a doorway between two wide cores.
bool supported_connection(
    const Scene& scene,
    const EnvironmentFloor& a,
    const EnvironmentFloor& b,
    const BakeSettings& settings,
    bool preserve_doorways
) {
    const auto delta = add(b.position, mul(a.position, -1));
    const float horizontal = delta[0] * delta[0] + delta[1] * delta[1];
    const float gap = preserve_doorways ? kGap : kPhysicalGap;

    if (horizontal < .01f || horizontal > gap * gap || std::abs(delta[2]) > 2) {
        return false;
    }

    const bool supported = preserve_doorways
                               ? room_path_clear(scene, a.position, b.position, settings)
                               : !room_floor_path(scene, a.position, b.position, settings).empty();

    if (!supported) {
        return false;
    }

    if (preserve_doorways && std::min(a.clearance, b.clearance) >= kCoreClearance) {
        const unsigned steps = std::max(2u, unsigned(std::ceil(std::sqrt(horizontal) / .75f)));

        for (unsigned i = 1; i < steps; ++i) {
            auto p = add(a.position, mul(delta, float(i) / float(steps)));
            p[2] += 1.25f;

            if (room_wall_distance(scene, p).distance < kCoreClearance) {
                return false;
            }
        }
    }

    return true;
}

struct Connections {
    const Scene& scene;
    std::vector<EnvironmentFloorSample>& samples;
    std::span<const EnvironmentFloor> floors;
    const BakeSettings& settings;
    const EnvironmentBakeSettings& options;
    EnvironmentPlacementReport& report;
    bool preserve_doorways;
    std::vector<Group> groups;
    std::vector<unsigned> ids;

    void connect(unsigned a, unsigned b) {
        if (samples[a].swimming_surface || samples[b].swimming_surface) {
            return;  // Swimming connections are explicit; a solid-floor query must never sink the waterline.
        }

        // Swimming is reachable gameplay, but neither waterline nor virtual surface is walking support.
        if (samples[a].underwater != samples[b].underwater
            || (samples[a].swimming_surface != samples[b].swimming_surface
                && !samples[a].underwater)) {
            return;
        }

        if (samples[a].underwater && samples[a].water_height && samples[b].water_height
            && std::abs(*samples[a].water_height - *samples[b].water_height) > .25f) {
            return;  // Separate submerged pools must not connect through an intervening dry passage.
        }

        if (ids[a] == ids[b]) {
            return;
        }

        const auto ga = root(groups, ids[a]), gb = root(groups, ids[b]);

        // The physical pass needs connectivity, not every possible shortcut through the same space.
        if (!preserve_doorways && ga == gb) {
            return;
        }

        if ((ga != gb && !compatible(groups[ga], groups[gb], options))
            || !supported_connection(scene, floors[a], floors[b], settings, preserve_doorways)) {
            return;
        }

        if (ga != gb) {
            join(groups, ga, gb);
            ++report.merged_floor_components;
        }

        if (std::find(samples[a].neighbours.begin(), samples[a].neighbours.end(), b)
            == samples[a].neighbours.end()) {
            samples[a].neighbours.push_back(b);
            samples[b].neighbours.push_back(a);
            ++report.bridged_floor_edges;
        }
    }
};
}  // namespace

void connect_environment_floor_fragments(
    const Scene& scene,
    std::vector<EnvironmentFloorSample>& samples,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    EnvironmentPlacementReport& report,
    bool preserve_doorways
) {
    if (samples.size() != floors.size()) {
        throw std::runtime_error("reflection floor grouping needs matching samples and floors");
    }

    Connections context{
        scene, samples, floors, settings, options, report, preserve_doorways, {}, {}
    };
    std::map<unsigned, unsigned> components;

    for (unsigned a = 0; a < samples.size(); ++a) {
        const auto [entry, fresh] =
            components.emplace(samples[a].region, unsigned(context.groups.size()));
        const auto id = entry->second;

        if (fresh) {
            context.groups.push_back({id, 0, {}, samples[a].lighting_labels});
        }

        context.ids.push_back(id);
        auto& group = context.groups[id];
        ++group.count;
        group.light = add(group.light, floors[a].light);
    }

    std::map<Cell, std::vector<unsigned>> bins;

    for (unsigned a = 0; a < samples.size(); ++a) {
        const auto cell = bin(floors[a].position, preserve_doorways ? kGap : kPhysicalGap);

        for (int z = -1; z <= 1; ++z) {
            for (int y = -1; y <= 1; ++y) {
                for (int x = -1; x <= 1; ++x) {
                    const auto found = bins.find({cell[0] + x, cell[1] + y, cell[2] + z});

                    if (found != bins.end()) {
                        for (auto b : found->second) {
                            context.connect(a, b);
                        }
                    }
                }
            }
        }

        bins[cell].push_back(a);
    }

    for (unsigned a = 0; a < samples.size(); ++a) {
        samples[a].region = root(context.groups, context.ids[a]);
        samples[a].lighting_labels = context.groups[samples[a].region].labels;
        std::sort(samples[a].neighbours.begin(), samples[a].neighbours.end());
        samples[a].neighbours.erase(
            std::unique(samples[a].neighbours.begin(), samples[a].neighbours.end()),
            samples[a].neighbours.end()
        );
    }

    scene.check_error();
}
}  // namespace openrac::lighting::bake_detail
