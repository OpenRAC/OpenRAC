// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Ridge-biased connections and collision-checked smoothing of a sparse distance field.
#include <algorithm>
#include <format>
#include <set>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_backbone.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
using Edge = std::array<unsigned, 2>;

Edge edge(unsigned a, unsigned b) {
    return {std::min(a, b), std::max(a, b)};
}

float distance(Float3 a, Float3 b) {
    const auto d = add(a, mul(b, -1));
    return std::sqrt(dot(d, d));
}

bool clear_segment(
    const Scene& scene,
    const RoomDistanceField& field,
    Float3 a,
    Float3 b,
    unsigned room,
    const BakeSettings& settings
) {
    // The ray checks differ by direction, and a path may walk a segment either way: both must pass.
    auto clear = [&](Float3 from, Float3 to) {
        return field.path_clear(scene, from, to, room, settings);
    };
    if (!field.connects(a, b, room)) {
        return false;
    }
    if (clear(a, b) && clear(b, a)) {
        return true;
    }
    return field.water_footprint && !(field.swimming_rooms[room] && !field.submerged_rooms[room])
           && room_supported_path_clear(scene, a, b, settings)
           && room_supported_path_clear(scene, b, a, settings);
}

// Split a chain where a straight segment no longer follows it, and at a bounded length.
void simplify(
    std::span<const unsigned> path,
    std::span<const Float3> points,
    const Scene& scene,
    const BakeSettings& settings,
    const RoomDistanceField& field,
    unsigned room,
    std::vector<Edge>& output
) {
    if (path.size() < 2) {
        return;
    }

    std::vector<Edge> pending{{0, static_cast<unsigned>(path.size() - 1)}};

    while (!pending.empty()) {
        const auto interval = pending.back();
        pending.pop_back();
        const auto a = points[path[interval[0]]], b = points[path[interval[1]]];
        const auto d = add(b, mul(a, -1));
        unsigned split = interval[0];
        float error = .5625f * field.step * field.step;

        for (unsigned i = interval[0] + 1; i < interval[1]; ++i) {
            const auto q = add(points[path[i]], mul(a, -1));
            const float t = std::clamp(dot(q, d) / std::max(.0001f, dot(d, d)), 0.f, 1.f);
            const auto offset = add(q, mul(d, -t));

            if (dot(offset, offset) > error) {
                error = dot(offset, offset);
                split = i;
            }
        }

        if (split == interval[0] && distance(a, b) > 12 && interval[1] > interval[0] + 1) {
            split = (interval[0] + interval[1]) / 2;
        }

        if (split == interval[0] && !clear_segment(scene, field, a, b, room, settings)) {
            if (interval[1] == interval[0] + 1) {
                throw std::runtime_error(
                    std::format(
                        "invalid medial field connection in room {} (swimming {}): {} {} {} to {} "
                        "{} "
                        "{}",
                        room,
                        bool(field.swimming_rooms[room]),
                        a[0],
                        a[1],
                        a[2],
                        b[0],
                        b[1],
                        b[2]
                    )
                );
            }

            split = (interval[0] + interval[1]) / 2;
        }

        if (split == interval[0]) {
            output.push_back({path[interval[0]], path[interval[1]]});
        } else {
            pending.push_back({split, interval[1]});
            pending.push_back({interval[0], split});
        }
    }
}
}  // namespace

void RoomLayout::rebuild_medial(
    const Scene& scene, std::span<const EnvironmentFloorSample> floors, const BakeSettings& settings
) {
    medial_edges.clear();
    medial_centres.clear();
    medial_entrances.clear();
    medial_points.clear();
    medial_owners.clear();
    medial_display_offsets.clear();
    auto field = room_distance_field(scene, floors, *this, settings);
    medial_cell_size = field.step;
    medial_ridge_samples = static_cast<unsigned>(
        std::count_if(field.nodes.begin(), field.nodes.end(), [](const auto& n) { return n.ridge; })
    );
    auto backbone = room_backbone(field, scene, settings);
    const auto n = field.nodes.size();
    for (const auto& node : field.nodes) {
        medial_points.push_back(node.position);
        medial_owners.push_back(node.room);
    }
    auto& skeleton = backbone.skeleton;
    medial_centres = std::move(backbone.centres);
    medial_entrances = std::move(backbone.entrances);
    std::vector<bool> fixed(n);
    for (auto a : medial_centres) {
        fixed[a] = true;
    }
    for (auto a : medial_entrances) {
        fixed[a] = true;
    }

    // Smooth only degree-two points. Junctions/endpoints retain topology, and each
    // candidate must keep its clearance and remain connected over real floor geometry.
    const auto original = medial_points;

    for (unsigned a = 0; a < n; ++a) {
        if (skeleton[a].size() == 2 && !fixed[a]) {
            const auto left = skeleton[a][0], right = skeleton[a][1];
            if (medial_owners[left] != medial_owners[a]
                || medial_owners[right] != medial_owners[a]) {
                continue;
            }
            const auto proposed =
                add(mul(original[a], .5f), mul(add(original[left], original[right]), .25f));
            const auto floor = field.swimming_rooms[medial_owners[a]]
                                   ? std::optional<Float3>{proposed}
                                   : room_floor_position(scene, proposed, settings);

            if (!floor
                || !clear_segment(scene, field, original[a], *floor, medial_owners[a], settings)
                || !clear_segment(
                    scene, field, medial_points[left], *floor, medial_owners[a], settings
                )
                || !clear_segment(
                    scene, field, *floor, medial_points[right], medial_owners[a], settings
                )) {
                continue;
            }

            auto eye = *floor;
            eye[2] += 1.25f;

            const auto room = medial_owners[a];
            if (!(field.water_footprint && field.swimming_rooms[room]
                  && !field.submerged_rooms[room])
                && room_wall_distance(scene, eye).distance + .2f * field.step
                       < field.nodes[a].wall.distance) {
                continue;
            }

            medial_points[a] = *floor;
        }
    }

    std::set<Edge> visited;

    for (unsigned a = 0; a < n; ++a) {
        if (skeleton[a].size() == 2 && !fixed[a]) {
            continue;
        }

        for (auto b : skeleton[a]) {
            if (visited.contains(edge(a, b))) {
                continue;
            }

            std::vector<unsigned> path{a};
            unsigned previous = a, current = b;

            for (;;) {
                visited.insert(edge(previous, current));
                path.push_back(current);

                if (skeleton[current].size() != 2 || fixed[current]) {
                    break;
                }

                const auto next =
                    skeleton[current][0] == previous ? skeleton[current][1] : skeleton[current][0];
                previous = current;
                current = next;
            }

            // Preserve medium transitions as checked field edges; simplify each side
            // inside its own known domain rather than treating a lighting tag as a wall.
            unsigned start = 0;
            for (unsigned i = 1; i < path.size(); ++i) {
                if (medial_owners[path[i]] == medial_owners[path[i - 1]]) {
                    continue;
                }
                simplify(
                    std::span(path).subspan(start, i - start),
                    medial_points,
                    scene,
                    settings,
                    field,
                    medial_owners[path[start]],
                    medial_edges
                );
                medial_edges.push_back({path[i - 1], path[i]});
                start = i;
            }
            simplify(
                std::span(path).subspan(start),
                medial_points,
                scene,
                settings,
                field,
                medial_owners[path[start]],
                medial_edges
            );
        }
    }

    connect_room_links(*this, field, scene, settings);
}
}  // namespace openrac::lighting::bake_detail
