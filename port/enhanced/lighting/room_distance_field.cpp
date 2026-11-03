// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Sparse walkable-layer samples of the unsigned wall distance field.
#include "enhanced/lighting/room_distance_field.h"

#include <map>
#include <tuple>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned kMaximumSamples = 256000;
using Cell = std::tuple<unsigned, int, int>;

// The normal support window stays narrow; floor-following repairs allow a bounded deeper dip.
std::optional<Float3> supporting_floor(
    const Scene& scene, Float3 p, const BakeSettings& settings, float maximum_drop = .75f
) {
    p[2] += .75f;
    const auto hit = scene.trace(p, {0, 0, -1}, settings.ray_bias, .75f + maximum_drop);

    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID
        || std::abs(unit({hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z})[2]) < .65f) {
        return {};
    }

    p[2] -= hit.ray.tfar;
    auto start = p;
    start[2] += .05f;

    if (scene.occluded(start, {0, 0, 1}, settings.ray_bias, 2.55f)) {
        return {};
    }

    return p;
}

float cell_size(std::span<const EnvironmentFloorSample> floors) {
    std::vector<float> lengths;

    for (unsigned a = 0; a < floors.size(); ++a) {
        for (auto b : floors[a].neighbours) {
            if (b >= floors.size()) {
                throw std::runtime_error("invalid distance-field floor edge");
            }

            const auto d = add(floors[b].position, mul(floors[a].position, -1));

            if (b > a && d[0] * d[0] + d[1] * d[1] > .01f) {
                lengths.push_back(std::hypot(d[0], d[1]));
            }
        }
    }

    if (lengths.empty()) {
        return 1;
    }

    const auto middle = lengths.begin() + lengths.size() / 2;
    std::nth_element(lengths.begin(), middle, lengths.end());
    return std::clamp(*middle * .5f, .5f, 1.f);
}

// Locate ridges where opposing nearest-boundary features meet. Project onto their bisector
// to recover subcell centres; world-space support and visibility validate every movement.
void refine_ridges(RoomDistanceField& field, const Scene& scene, const BakeSettings& settings) {
    std::vector<RoomWallDistance> samples;

    for (const auto& n : field.nodes) {
        samples.push_back(n.wall);
    }

    for (auto& node : field.nodes) {
        if (field.swimming_rooms[node.room]) {
            continue;  // Water is an authored support plane; projecting it onto solid floor would sink it.
        }

        auto eye = node.position;
        eye[2] += 1.25f;
        float best = -1;
        Float3 selected = node.position;

        for (auto a : node.neighbours) {
            for (auto b : node.neighbours) {
                if (b <= a || !samples[a].found || !samples[b].found) {
                    continue;
                }

                const auto va = add(samples[a].closest, mul(eye, -1));
                const auto vb = add(samples[b].closest, mul(eye, -1));
                const float length = std::sqrt(dot(va, va) * dot(vb, vb));

                if (length < .01f) {
                    continue;
                }

                const float opposition = -dot(va, vb) / length;

                if (opposition < .3f) {
                    continue;
                }

                const auto across = add(samples[b].closest, mul(samples[a].closest, -1));

                if (dot(across, across) < field.step * field.step) {
                    continue;
                }

                const auto middle = mul(add(samples[a].closest, samples[b].closest), .5f);
                auto p =
                    add(eye,
                        mul(across, -dot(add(eye, mul(middle, -1)), across) / dot(across, across)));
                p[2] = node.position[2];
                const auto delta = add(p, mul(node.position, -1));

                if (dot(delta, delta) > .5625f * field.step * field.step) {
                    continue;
                }

                const float score = opposition - dot(delta, delta) / (field.step * field.step);

                if (score <= best) {
                    continue;
                }

                if (!field.contains(p, node.room)) {
                    continue;
                }

                const auto floor = supporting_floor(scene, p, settings);

                if (!floor || !room_path_clear(scene, node.position, *floor, settings)) {
                    continue;
                }

                auto query = *floor;
                query[2] += 1.25f;
                const auto wall = room_wall_distance(scene, query);

                if (wall.distance + .05f < node.wall.distance) {
                    continue;
                }

                best = score;
                selected = *floor;
                node.wall = wall;
            }
        }

        if (best >= 0) {
            node.position = selected;
            node.ridge = true;
        }
    }
}

// Two samples may project onto the same ridge point. Coalesce them before extracting topology
// so coincident cells do not become artificial junctions or zero-length branches.
void compact(RoomDistanceField& field, const Scene& scene, const BakeSettings& settings) {
    using Key = std::tuple<unsigned, int, int, int>;
    std::map<Key, unsigned> index;
    std::vector<unsigned> remap;
    std::vector<RoomFieldNode> nodes;
    const float scale = 20 / field.step;

    for (const auto& n : field.nodes) {
        Key key{
            n.room,
            int(std::round(n.position[0] * scale)),
            int(std::round(n.position[1] * scale)),
            int(std::round(n.position[2] * scale))
        };
        const auto [it, added] = index.try_emplace(key, static_cast<unsigned>(nodes.size()));
        remap.push_back(it->second);

        if (added) {
            nodes.push_back(n);
            nodes.back().neighbours.clear();
        } else {
            nodes[it->second].ridge = nodes[it->second].ridge || n.ridge;
        }
    }

    for (unsigned a = 0; a < field.nodes.size(); ++a) {
        for (auto b : field.nodes[a].neighbours) {
            if (remap[a] != remap[b]) {
                nodes[remap[a]].neighbours.push_back(remap[b]);
            }
        }
    }

    for (unsigned a = 0; a < nodes.size(); ++a) {
        auto& row = nodes[a].neighbours;
        std::sort(row.begin(), row.end());
        row.erase(std::unique(row.begin(), row.end()), row.end());
        std::erase_if(row, [&](auto b) {
            return !field.connects(nodes[a].position, nodes[b].position, nodes[a].room)
                   || !(
                       field.swimming_rooms[nodes[a].room]
                           ? room_body_path_clear(
                                 scene, nodes[a].position, nodes[b].position, settings, true
                             )
                           : room_path_clear(scene, nodes[a].position, nodes[b].position, settings)
                   );
        });
    }

    // The checks above run from each end, and the body check is not symmetric: keep an edge
    // only when it passed from both ends, so a path may walk it either way.
    std::vector<std::vector<unsigned>> kept(nodes.size());

    for (unsigned a = 0; a < nodes.size(); ++a) {
        for (auto b : nodes[a].neighbours) {
            if (std::binary_search(nodes[b].neighbours.begin(), nodes[b].neighbours.end(), a)) {
                kept[a].push_back(b);
            }
        }
    }

    for (unsigned a = 0; a < nodes.size(); ++a) {
        nodes[a].neighbours = std::move(kept[a]);
    }

    field.nodes = std::move(nodes);
}

// Overlapping body/head rays catch walls lying exactly on a floor-sampling boundary.
bool floor_step_clear(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings, float body_height = 2.5f
) {
    constexpr float kRayOverlap = .05f;
    const auto delta = add(b, mul(a, -1));
    const float length = std::sqrt(dot(delta, delta));

    if (length <= .001f) {
        return true;
    }

    const auto direction = mul(delta, 1 / length);

    for (float height : {body_height * .5f, body_height}) {
        auto body = add(a, mul(direction, -kRayOverlap));
        body[2] += height;

        if (scene.occluded(body, direction, settings.ray_bias, length + kRayOverlap * 2)) {
            return false;
        }
    }

    return true;
}
}  // namespace

bool RoomDistanceField::contains(Float3 position, unsigned room) const {
    // Cell boundaries belong to either neighbour; rounding a half-cell must not make edges directional.
    const float x = position[0] / step, y = position[1] / step;
    constexpr float kHalfCell = .5001f;

    for (int cx = int(std::ceil(x - kHalfCell)); cx <= int(std::floor(x + kHalfCell)); ++cx) {
        for (int cy = int(std::ceil(y - kHalfCell)); cy <= int(std::floor(y + kHalfCell)); ++cy) {
            const auto cell = domain.find({room, cx, cy});

            if (cell != domain.end()
                && std::any_of(cell->second.begin(), cell->second.end(), [&](float z) {
                       return std::abs(z - position[2]) <= .75f;
                   })) {
                return true;
            }
        }
    }

    return false;
}

bool RoomDistanceField::connects(Float3 a, Float3 b, unsigned room) const {
    const auto delta = add(b, mul(a, -1));
    const auto steps =
        std::max(1u, unsigned(std::ceil(std::sqrt(dot(delta, delta)) / (.5f * step))));

    for (unsigned i = 0; i <= steps; ++i) {
        if (!contains(add(a, mul(delta, float(i) / float(steps))), room)) {
            return false;
        }
    }

    return true;
}

std::optional<Float3> room_floor_position(
    const Scene& scene, Float3 p, const BakeSettings& settings
) {
    return supporting_floor(scene, p, settings);
}

bool room_path_clear(const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings) {
    const auto d = add(b, mul(a, -1));
    const float length = std::sqrt(dot(d, d));

    if (length < .001f) {
        return true;
    }

    for (float height : {1.25f, 2.5f}) {
        auto start = a;
        start[2] += height;

        if (scene.occluded(
                start,
                mul(d, 1 / length),
                settings.ray_bias,
                std::max(settings.ray_bias, length - .01f)
            )) {
            return false;
        }
    }

    const unsigned steps = std::max(1u, unsigned(std::ceil(length / .75f)));

    for (unsigned i = 1; i < steps; ++i) {
        const auto p = add(a, mul(d, float(i) / float(steps)));
        const auto floor = supporting_floor(scene, p, settings);

        if (!floor || std::abs((*floor)[2] - p[2]) > .75f) {
            return false;
        }
    }

    return true;
}

// Tests both endpoint margins because the room graph stores undirected connections.
bool room_body_path_clear(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings, bool swimming
) {
    // Swimming uses half-height clearance at the water surface.
    const float height = settings.traversal.player_height * (swimming ? .5f : 1.f);
    return floor_step_clear(scene, a, b, settings, height)
           && floor_step_clear(scene, b, a, settings, height);
}

std::vector<Float3> room_floor_path(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
) {
    constexpr float kMaximumDip = 2;  // Same vertical step bound as floor-component connectivity.
    constexpr float kSamplingStep = .75f;  // Match the existing walkable-path support interval.
    const auto delta = add(b, mul(a, -1));
    const unsigned steps =
        std::max(1u, unsigned(std::ceil(std::hypot(delta[0], delta[1]) / kSamplingStep)));
    std::vector<Float3> path;

    for (unsigned i = 0; i <= steps; ++i) {
        const auto p = supporting_floor(
            scene, add(a, mul(delta, float(i) / float(steps))), settings, kMaximumDip
        );

        // Endpoints must remain on their original storey even when the route dips between them.
        if (!p || ((i == 0 || i == steps) && std::abs((*p)[2] - (i ? b[2] : a[2])) > .75f)) {
            return {};
        }

        if (!path.empty()
            && (std::abs((*p)[2] - path.back()[2]) > kMaximumDip
                || !floor_step_clear(scene, path.back(), *p, settings))) {
            return {};
        }

        path.push_back(*p);
    }

    return path;
}

RoomDistanceField room_distance_field(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    const BakeSettings& settings
) {
    RoomDistanceField result;
    result.swimming_rooms.resize(layout.rooms.size());
    result.submerged_rooms.resize(layout.rooms.size());
    result.water_heights.resize(layout.rooms.size());

    for (unsigned room = 0; room < layout.rooms.size(); ++room) {
        result.swimming_rooms[room] =
            std::any_of(layout.rooms[room].begin(), layout.rooms[room].end(), [&](auto id) {
                return floors[id].swimming_surface;
            });
        result.submerged_rooms[room] = floors[layout.rooms[room].front()].underwater;
        result.water_heights[room] = floors[layout.rooms[room].front()].water_height;
    }

    result.step = cell_size(floors);
    std::map<Cell, std::vector<unsigned>> cells;
    const float radius = result.step;

    for (unsigned source = 0; source < floors.size(); ++source) {
        const auto base = floors[source].position;
        const unsigned room = layout.owners[source];

        for (int x = int(std::ceil((base[0] - radius) / result.step));
             x <= int(std::floor((base[0] + radius) / result.step));
             ++x) {
            for (int y = int(std::ceil((base[1] - radius) / result.step));
                 y <= int(std::floor((base[1] + radius) / result.step));
                 ++y) {
                const auto key = Cell{room, x, y};
                const auto cell = cells.find(key);

                if (cell != cells.end()
                    && std::any_of(cell->second.begin(), cell->second.end(), [&](auto a) {
                           return std::abs(result.nodes[a].position[2] - base[2]) < .75f;
                       })) {
                    continue;
                }

                Float3 p{float(x) * result.step, float(y) * result.step, base[2]};
                const auto floor = floors[source].swimming_surface
                                       ? std::optional<Float3>{p}
                                       : supporting_floor(scene, p, settings);

                if (!floor
                    || !(
                        floors[source].swimming_surface
                            ? room_body_path_clear(scene, base, *floor, settings, true)
                            : room_path_clear(scene, base, *floor, settings)
                    )) {
                    continue;
                }

                if (source < layout.height_bounds.size()
                    && ((*floor)[2] < layout.height_bounds[source][0] - .01f
                        || (*floor)[2] + 1.25f > layout.height_bounds[source][1] + .01f)) {
                    continue;
                }

                auto eye = *floor;
                eye[2] += 1.25f;
                const auto wall = room_wall_distance(scene, eye);

                if (wall.distance < .15f) {
                    continue;
                }

                if (result.nodes.size() == kMaximumSamples) {
                    throw std::runtime_error("medial distance field exceeds 256000 samples");
                }

                cells[key].push_back(static_cast<unsigned>(result.nodes.size()));
                result.nodes.push_back({*floor, wall, room, false, {}});
            }
        }
    }

    for (const auto& [key, indices] : cells) {
        const auto [room, x, y] = key;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (!dx && !dy) {
                    continue;
                }

                const auto other = cells.find({room, x + dx, y + dy});

                if (other == cells.end()) {
                    continue;
                }

                for (auto a : indices) {
                    for (auto b : other->second) {
                        if (b <= a
                            || std::abs(result.nodes[a].position[2] - result.nodes[b].position[2])
                                   > 2) {
                            continue;
                        }

                        const bool clear = result.swimming_rooms[room]
                                               ? room_body_path_clear(
                                                     scene,
                                                     result.nodes[a].position,
                                                     result.nodes[b].position,
                                                     settings,
                                                     true
                                                 )
                                               : room_path_clear(
                                                     scene,
                                                     result.nodes[a].position,
                                                     result.nodes[b].position,
                                                     settings
                                                 );

                        if (!clear) {
                            continue;
                        }

                        result.nodes[a].neighbours.push_back(b);
                        result.nodes[b].neighbours.push_back(a);
                    }
                }
            }
        }
    }

    for (const auto& [key, indices] : cells) {
        for (auto a : indices) {
            result.domain[key].push_back(result.nodes[a].position[2]);
        }
    }

    cells.clear();
    refine_ridges(result, scene, settings);
    compact(result, scene, settings);
    scene.check_error();
    return result;
}
}  // namespace openrac::lighting::bake_detail
