// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Bounded airborne connections between separately supported walking components.
#include <algorithm>
#include <map>
#include <queue>
#include <tuple>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
// Normalised ballistic time permits unequal landing heights without assuming a game tick rate.
Float3 room_jump_position(Float3 a, Float3 b, float apex, float t) {
    auto p = add(mul(a, 1 - t), mul(b, t));
    const float up = std::sqrt(std::max(0.f, apex - a[2]));
    const float down = std::sqrt(std::max(0.f, apex - b[2]));
    const float height = up - t * (up + down);
    p[2] = apex - height * height;
    return p;
}

namespace {
constexpr unsigned kMissing = ~0u;
constexpr unsigned kTrials = 16;  // Bound collision queries at each candidate component boundary.
using Pair = std::pair<unsigned, unsigned>;
using Bin = std::array<int, 3>;

// Sweep a conservative cross-section, including feet, waist and head, against two-sided geometry.
bool arc_clear(const Scene& scene, Float3 a, Float3 b, float apex, const BakeSettings& settings) {
    const auto delta = add(b, mul(a, -1));
    const unsigned steps = std::max(8u, unsigned(std::ceil(std::hypot(delta[0], delta[1]) / .5f)));
    const float radius = settings.traversal.player_radius;

    for (unsigned i = 0; i < steps; ++i) {
        const auto from = room_jump_position(a, b, apex, float(i) / float(steps));
        const auto to = room_jump_position(a, b, apex, float(i + 1) / float(steps));
        const auto d = add(to, mul(from, -1));
        const float length = std::sqrt(dot(d, d));

        for (Float2 offset :
             {Float2{},
              Float2{radius, 0},
              Float2{-radius, 0},
              Float2{0, radius},
              Float2{0, -radius}}) {
            for (float height :
                 {.15f, settings.traversal.player_height * .5f, settings.traversal.player_height}) {
                auto start = from;
                start[0] += offset[0];
                start[1] += offset[1];
                start[2] += height;

                if (length > settings.ray_bias
                    && scene.occluded(start, mul(d, 1 / length), settings.ray_bias, length)) {
                    return false;
                }
            }
        }
    }

    return true;
}

// Try low arcs first; a ceiling can reject a high jump while a shorter trajectory still fits.
std::optional<float> jump_apex(
    const Scene& scene, Float3 a, Float3 b, const BakeSettings& settings
) {
    if (b[2] - a[2] > settings.traversal.jump_rise || a[2] - b[2] > settings.traversal.jump_drop) {
        return {};
    }

    for (float clearance : {.5f, 1.f, 2.f, 3.f, 4.f}) {
        const float apex = std::max(a[2], b[2]) + clearance;

        if (apex - a[2] <= settings.traversal.jump_rise && arc_clear(scene, a, b, apex, settings)) {
            return apex;
        }
    }

    return {};
}

// Field connectivity, rather than lighting ownership, decides whether a jump is necessary.
std::vector<unsigned> components(const RoomDistanceField& field) {
    std::vector<unsigned> ids(field.nodes.size(), kMissing);
    unsigned id = 0;

    for (unsigned start = 0; start < ids.size(); ++start) {
        if (ids[start] != kMissing) {
            continue;
        }

        std::vector<unsigned> pending{start};
        ids[start] = id++;

        for (unsigned i = 0; i < pending.size(); ++i) {
            for (auto b : field.nodes[pending[i]].neighbours) {
                if (ids[b] == kMissing) {
                    ids[b] = ids[start];
                    pending.push_back(b);
                }
            }
        }
    }

    return ids;
}

struct Candidate {
    unsigned from = 0, to = 0;
    float score = 0;
};

// Keep several approaches so one nearby wall cannot hide a valid route around its end.
void consider(std::map<Pair, std::vector<Candidate>>& pairs, Pair key, Candidate candidate) {
    auto& values = pairs[key];

    if (values.size() < kTrials || candidate.score < values.back().score) {
        values.push_back(candidate);
        std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) {
            return std::tie(a.score, a.from, a.to) < std::tie(b.score, b.from, b.to);
        });

        if (values.size() > kTrials) {
            values.pop_back();
        }
    }
}

// Authored water bodies can connect vertically through their swimming column.
bool same_water(const RoomDistanceField& field, unsigned a, unsigned b) {
    return field.water_heights[a] && field.water_heights[b]
           && std::abs(*field.water_heights[a] - *field.water_heights[b]) < .25f;
}

// Spatial bins bound candidate enumeration without mixing physical and lighting component IDs.
struct CandidateSearch {
    const RoomDistanceField& field;
    const BakeSettings& settings;
    std::vector<unsigned> ids;
    std::map<Bin, std::vector<unsigned>> bins;
    std::map<Pair, std::vector<Candidate>> pairs;

    // Directed rise/drop checks happen later, after symmetric nearby approaches are collected.
    void offer(unsigned a, unsigned b) {
        const auto& node = field.nodes[a];
        const auto& other = field.nodes[b];
        const auto d = add(node.position, mul(other.position, -1));
        const bool water = same_water(field, node.room, other.room);
        const float vertical_limit =
            water ? 64.f : std::max(settings.traversal.jump_drop, settings.traversal.jump_rise);
        const float range = settings.traversal.jump_distance;

        if (ids[a] == ids[b] || d[0] * d[0] + d[1] * d[1] > range * range
            || std::abs(d[2]) > vertical_limit) {
            return;
        }

        const float score = dot(d, d);
        consider(pairs, {ids[a], ids[b]}, {a, b, score});
        consider(pairs, {ids[b], ids[a]}, {b, a, score});
    }

    // Landing centres need room for the player's body, independently of the swept airborne arc.
    void insert(unsigned a) {
        const auto& node = field.nodes[a];

        if (node.wall.distance < settings.traversal.player_radius) {
            return;
        }

        const auto p = node.position;
        const float range = settings.traversal.jump_distance;
        const Bin bin{int(std::floor(p[0] / range)), int(std::floor(p[1] / range)), 0};

        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                const auto found = bins.find({bin[0] + x, bin[1] + y, 0});

                if (found != bins.end()) {
                    for (auto b : found->second) {
                        offer(a, b);
                    }
                }
            }
        }

        bins[bin].push_back(a);
    }
};

// Search only nearby landing cells, preserving a bounded number of alternate approaches per pair.
std::map<Pair, std::vector<Candidate>> candidates(
    const RoomDistanceField& field, const BakeSettings& settings
) {
    CandidateSearch search{field, settings, components(field), {}, {}};

    for (unsigned a = 0; a < field.nodes.size(); ++a) {
        search.insert(a);
    }

    return std::move(search.pairs);
}

// A supported doorway is walking; water is swimming; only the remaining gaps need an airborne arc.
std::optional<RoomLink> choose_link(
    const RoomLayout& layout,
    const RoomDistanceField& field,
    const Scene& scene,
    const Candidate& candidate,
    const BakeSettings& settings
) {
    const auto from = layout.medial_points[candidate.from], to = layout.medial_points[candidate.to];
    const auto from_room = field.nodes[candidate.from].room,
               to_room = field.nodes[candidate.to].room;
    const bool water = same_water(field, from_room, to_room);

    if (water && room_body_path_clear(scene, from, to, settings, true)) {
        return RoomLink{candidate.from, candidate.to, std::max(from[2], to[2]), RoomLinkKind::Swim};
    }

    if (!water && room_path_clear(scene, from, to, settings)) {
        return RoomLink{candidate.from, candidate.to, std::max(from[2], to[2]), RoomLinkKind::Walk};
    }

    if (const auto apex = jump_apex(scene, from, to, settings)) {
        return RoomLink{candidate.from, candidate.to, *apex, RoomLinkKind::Jump};
    }

    return {};
}

// Extend the walking skeleton to each landing through its existing field graph, never across air.
void attach(
    RoomLayout& layout,
    const RoomDistanceField& field,
    unsigned landing,
    std::vector<bool>& selected
) {
    std::vector<unsigned> previous(field.nodes.size(), kMissing), pending{landing};
    previous[landing] = landing;
    unsigned reached = kMissing;

    for (unsigned i = 0; i < pending.size(); ++i) {
        const auto a = pending[i];

        if (selected[a]) {
            reached = a;
            break;
        }

        for (auto b : field.nodes[a].neighbours) {
            if (previous[b] == kMissing) {
                previous[b] = a;
                pending.push_back(b);
            }
        }
    }

    while (reached != kMissing && reached != landing) {
        const auto next = previous[reached];
        layout.medial_edges.push_back({reached, next});
        selected[next] = true;
        reached = next;
    }
}
}  // namespace

void connect_room_links(
    RoomLayout& layout,
    const RoomDistanceField& field,
    const Scene& scene,
    const BakeSettings& settings
) {
    layout.links.clear();

    if (settings.traversal.jump_distance <= 0) {
        return;
    }

    std::vector<bool> selected(field.nodes.size());

    for (auto edge : layout.medial_edges) {
        selected[edge[0]] = selected[edge[1]] = true;
    }

    for (auto centre : layout.medial_centres) {
        selected[centre] = true;
    }

    for (const auto& [pair, choices] : candidates(field, settings)) {
        for (const auto& candidate : choices) {
            const auto link = choose_link(layout, field, scene, candidate, settings);

            if (!link) {
                continue;
            }

            attach(layout, field, candidate.from, selected);
            attach(layout, field, candidate.to, selected);
            layout.links.push_back(*link);
            break;
        }
    }
}
}  // namespace openrac::lighting::bake_detail
