// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Constrained fitting of the actual reflection receiver domain, independent of capture placement.
#include <algorithm>
#include <cmath>
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_placement_internal.h"
#include "enhanced/lighting/probe_fit.h"

namespace openrac::lighting::bake_detail {
namespace {
struct Receiver {
    Float3 position;
    float margin;
};

// A box is convex. On an exact horizontal receiver plane, a foreign point
// strictly inside the required receivers' hull cannot be excluded by any rotation.
// This is a feasibility proof, not an approximation of the orientation search.
bool convex_conflict(
    std::span<const Receiver> required, std::span<const Float3> foreign, float margin
) {
    std::map<float, std::vector<Float2>> planes;
    for (const auto& r : required) {
        if (r.margin >= margin) {
            planes[r.position[2]].push_back({r.position[0], r.position[1]});
        }
    }
    const auto cross = [](Float2 a, Float2 b, Float2 c) {
        return (double(b[0]) - a[0]) * (double(c[1]) - a[1])
               - (double(b[1]) - a[1]) * (double(c[0]) - a[0]);
    };
    for (auto& [z, points] : planes) {
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
        if (points.size() < 3) {
            points.clear();
            continue;
        }
        std::vector<Float2> hull;
        for (const auto p : points) {
            while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 0) {
                hull.pop_back();
            }
            hull.push_back(p);
        }
        const auto lower = hull.size();
        for (auto it = points.rbegin() + 1; it != points.rend(); ++it) {
            while (hull.size() > lower && cross(hull[hull.size() - 2], hull.back(), *it) <= 0) {
                hull.pop_back();
            }
            hull.push_back(*it);
        }
        hull.pop_back();
        points = std::move(hull);
    }
    for (auto p : foreign) {
        const auto plane = planes.find(p[2]);
        if (plane == planes.end() || plane->second.size() < 3) {
            continue;
        }
        const auto& hull = plane->second;
        bool inside = true;
        for (unsigned i = 0; i < hull.size(); ++i) {
            inside &= cross(hull[i], hull[(i + 1) % hull.size()], {p[0], p[1]}) > 1e-6;
        }
        if (inside) {
            return true;
        }
    }
    return false;
}

float boundary_distance(const EnvironmentProbe& p, Float3 position) {
    const auto local = p.frame.to_local(position);
    float distance = .75f;
    for (unsigned a = 0; a < 3; ++a) {
        distance = std::min({distance, local[a] - p.minimum[a], p.maximum[a] - local[a]});
    }
    return distance;
}

double volume(const EnvironmentProbe& p) {
    return double(p.maximum[0] - p.minimum[0]) * (p.maximum[1] - p.minimum[1])
           * (p.maximum[2] - p.minimum[2]);
}

float useful_distance() {
    float lo = 0, hi = .75f;
    for (unsigned i = 0; i < 24; ++i) {
        const float m = (lo + hi) * .5f, t = m / .75f;
        if (t * t * (3 - 2 * t) < kEnvironmentUsefulCoverage) {
            lo = m;
        } else {
            hi = m;
        }
    }
    return hi;
}
}  // namespace

std::vector<unsigned> fit_environment_capture_boxes(
    const Scene& scene,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    std::vector<EnvironmentProbe>& probes,
    std::span<const unsigned> origins,
    const EnvironmentAssignments* assignments,
    EnvironmentNeighbourhoods* neighbourhoods,
    std::span<const float> inherited_margins
) {
    const float useful = useful_distance();
    std::vector<unsigned> failed;
    for (unsigned i = 0; i < probes.size(); ++i) {
        const auto original = probes[i];
        const auto& source = floors[origins[i]];
        std::vector<Receiver> required;
        std::vector<bool> assigned(
            floors.size() * kEnvironmentReceiverHeights.size(), !assignments
        );
        if (assignments) {
            for (auto receiver : (*assignments)[i]) {
                assigned[receiver] = true;
            }
        }
        std::vector<Float3> points{original.position};
        Float3 lo = original.position, hi = lo;
        for (unsigned f = 0; f < floors.size(); ++f) {
            const auto& floor = floors[f];
            if (!environment_spaces_compatible(source, floor)) {
                continue;
            }
            for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
                if (!assigned[f * unsigned(kEnvironmentReceiverHeights.size()) + h]) {
                    continue;
                }
                const auto position = environment_receiver_position(floor, h);
                const bool covered =
                    environment_probe_coverage(original, position) >= kEnvironmentUsefulCoverage;
                if (!assignments && !covered) {
                    continue;
                }
                // Preserve existing blend strength, rather than reducing every
                // boundary to the minimum useful weight merely to get a smaller box.
                const unsigned receiver = f * unsigned(kEnvironmentReceiverHeights.size()) + h;
                const float inherited = inherited_margins.empty() ? 0 : inherited_margins[receiver];
                const float margin =
                    inherited > 0
                        ? std::max(inherited, covered ? boundary_distance(original, position) : 0.f)
                        : (covered ? boundary_distance(original, position) : .75f);
                required.push_back({position, margin + .0001f});
                points.push_back(position);
                for (unsigned a = 0; a < 3; ++a) {
                    lo[a] = std::min(lo[a], position[a]);
                    hi[a] = std::max(hi[a], position[a]);
                }
            }
        }
        if (required.empty()) {
            continue;
        }
        // Any candidate's corners lie inside this conservative support sphere.
        // Discard distant foreign receivers before evaluating angular candidates.
        const auto centre = mul(add(lo, hi), .5f);
        const float radius =
            std::sqrt(dot(add(hi, mul(lo, -1)), add(hi, mul(lo, -1)))) * .5f + .751f;
        std::vector<Float3> foreign;
        bool original_leaks = false;
        for (unsigned f = 0; f < floors.size(); ++f) {
            const auto& floor = floors[f];
            if (environment_spaces_compatible(source, floor)
                && (!neighbourhoods || neighbourhoods->contains(origins[i], f, true))) {
                continue;
            }
            for (unsigned h = 0; h < kEnvironmentReceiverHeights.size(); ++h) {
                const auto p = environment_receiver_position(floor, h), d = add(p, mul(centre, -1));
                original_leaks |=
                    environment_probe_coverage(original, p) >= kEnvironmentUsefulCoverage;
                if (dot(d, d) <= 3 * radius * radius) {
                    foreign.push_back(p);
                }
            }
        }
        if (convex_conflict(required, foreign, useful)) {
            failed.push_back(i);
            continue;
        }
        auto best = original;
        // The old box need not lie inside the new domain's broad-phase sphere.
        // Validate it against every foreign receiver before retaining it.
        double best_volume = original_leaks ? INFINITY : volume(original);
        for (const auto& receiver : required) {
            if (boundary_distance(original, receiver.position) + .0002f < receiver.margin) {
                best_volume = INFINITY;
            }
        }
        for (auto p : foreign) {
            if (environment_probe_coverage(original, p) >= kEnvironmentUsefulCoverage) {
                best_volume = INFINITY;
            }
        }
        const bool original_valid = std::isfinite(best_volume);
        search_probe_frame(
            points,
            [&](ProbeFrame frame) {
                auto candidate = original;
                candidate.frame = frame;
                const auto camera = frame.to_local(original.position);
                for (unsigned a = 0; a < 3; ++a) {
                    candidate.minimum[a] = camera[a] - .01f;
                    candidate.maximum[a] = camera[a] + .01f;
                }
                for (const auto& receiver : required) {
                    const auto p = frame.to_local(receiver.position);
                    for (unsigned a = 0; a < 3; ++a) {
                        candidate.minimum[a] =
                            std::min(candidate.minimum[a], p[a] - receiver.margin);
                        candidate.maximum[a] =
                            std::max(candidate.maximum[a], p[a] + receiver.margin);
                    }
                }
                // Swimming receivers describe the whole submerged column. A
                // tighter fit must retain its waterline even when the highest
                // sampled receiver is lower than that boundary.
                if (source.underwater && source.water_height && frame.pitch == Float2{1, 0}
                    && frame.roll == Float2{1, 0}) {
                    candidate.maximum[2] = std::max(candidate.maximum[2], *source.water_height);
                }
                for (unsigned a = 0; a < 3; ++a) {
                    if (camera[a] - candidate.minimum[a] > options.box_distance + .001f
                        || candidate.maximum[a] - camera[a] > options.box_distance + .001f) {
                        return double(INFINITY);
                    }
                }
                for (auto p : foreign) {
                    if (boundary_distance(candidate, p) >= useful - .0001f) {
                        return double(INFINITY);
                    }
                }
                const double cost = volume(candidate);
                if (cost < best_volume - 1e-5) {
                    best_volume = cost;
                    best = candidate;
                }
                return cost;
            },
            std::span(&original.frame, 1),
            !source.water_height
        );
        if (!std::isfinite(best_volume)
            || (original_valid && best_volume >= volume(original) - 1e-5)) {
            if (!std::isfinite(best_volume)) {
                failed.push_back(i);
            }
            continue;
        }
        // Projection is geometric and independent of influence. Trace it once
        // after choosing a valid fit, rather than six rays for every trial frame.
        const auto camera = best.frame.to_local(best.position);
        for (unsigned a = 0; a < 3; ++a) {
            for (float sign : {-1.f, 1.f}) {
                Float3 direction{};
                direction[a] = sign;
                const auto hit = scene.trace(
                    best.position,
                    best.frame.to_world(direction),
                    settings.ray_bias,
                    options.box_distance
                );
                const float bound = camera[a] + sign * std::max(.01f, hit.ray.tfar);
                (sign < 0 ? best.projection_minimum[a] : best.projection_maximum[a]) = bound;
            }
        }
        probes[i] = best;
    }
    return failed;
}

bool remove_environment_capture(
    const Scene& scene,
    std::span<const EnvironmentFloor> floors,
    const BakeSettings& settings,
    const EnvironmentBakeSettings& options,
    unsigned removed,
    std::vector<EnvironmentProbe>& probes,
    std::vector<unsigned>& origins,
    EnvironmentAssignments& assignments,
    EnvironmentNeighbourhoods& neighbourhoods
) {
    std::vector<unsigned> counts(floors.size() * kEnvironmentReceiverHeights.size());
    for (unsigned i = 0; i < assignments.size(); ++i) {
        if (i != removed) {
            for (auto receiver : assignments[i]) {
                ++counts[receiver];
            }
        }
    }
    std::vector<unsigned> orphaned;
    std::vector<float> inherited_margins(counts.size());
    for (auto receiver : assignments[removed]) {
        if (!counts[receiver]) {
            orphaned.push_back(receiver);
            const unsigned f = receiver / unsigned(kEnvironmentReceiverHeights.size());
            const unsigned h = receiver % unsigned(kEnvironmentReceiverHeights.size());
            inherited_margins[receiver] =
                boundary_distance(probes[removed], environment_receiver_position(floors[f], h));
            ++counts[receiver];
        }
    }
    if (std::find(counts.begin(), counts.end(), 0u) != counts.end()) {
        return false;
    }
    const auto eligible = [&](unsigned i, unsigned receiver) {
        const unsigned f = receiver / unsigned(kEnvironmentReceiverHeights.size());
        return i != removed && environment_spaces_compatible(floors[origins[i]], floors[f])
               && neighbourhoods.contains(origins[i], f, true);
    };
    const auto attempt = [&](std::span<const unsigned> replacements) {
        auto reassigned = assignments;
        std::vector<bool> affected(probes.size());
        for (unsigned r = 0; r < orphaned.size(); ++r) {
            reassigned[replacements[r]].push_back(orphaned[r]);
            affected[replacements[r]] = true;
        }
        std::vector<EnvironmentProbe> fitted;
        std::vector<unsigned> fitted_origins, indices;
        EnvironmentAssignments required;
        for (unsigned i = 0; i < probes.size(); ++i) {
            if (affected[i]) {
                std::sort(reassigned[i].begin(), reassigned[i].end());
                indices.push_back(i);
                fitted.push_back(probes[i]);
                fitted_origins.push_back(origins[i]);
                required.push_back(reassigned[i]);
            }
        }
        if (!fit_environment_capture_boxes(
                 scene,
                 floors,
                 settings,
                 options,
                 fitted,
                 fitted_origins,
                 &required,
                 &neighbourhoods,
                 inherited_margins
            )
                 .empty()) {
            return false;
        }
        for (unsigned i = 0; i < indices.size(); ++i) {
            probes[indices[i]] = fitted[i];
        }
        probes.erase(probes.begin() + removed);
        origins.erase(origins.begin() + removed);
        reassigned.erase(reassigned.begin() + removed);
        assignments = std::move(reassigned);
        return true;
    };
    std::vector<unsigned> replacements;
    for (auto receiver : orphaned) {
        const unsigned f = receiver / unsigned(kEnvironmentReceiverHeights.size());
        unsigned replacement = ~0u;
        float nearest = INFINITY;
        for (unsigned i = 0; i < probes.size(); ++i) {
            if (eligible(i, receiver)) {
                const float distance = neighbourhoods.distance(origins[i], f);
                if (distance < nearest) {
                    nearest = distance;
                    replacement = i;
                }
            }
        }
        if (replacement == ~0u) {
            return false;
        }
        replacements.push_back(replacement);
    }
    if (attempt(replacements)) {
        return true;
    }
    // Nearest-first can choose a cramped boundary pocket. Also try transferring
    // the complete patch to a larger local domain before retaining another camera.
    std::vector<unsigned> alternatives;
    for (unsigned i = 0; i < probes.size(); ++i) {
        if (std::all_of(orphaned.begin(), orphaned.end(), [&](unsigned r) {
                return eligible(i, r);
            })) {
            alternatives.push_back(i);
        }
    }
    std::stable_sort(alternatives.begin(), alternatives.end(), [&](unsigned a, unsigned b) {
        return assignments[a].size() > assignments[b].size();
    });
    for (auto i : alternatives) {
        if (std::all_of(replacements.begin(), replacements.end(), [&](unsigned r) {
                return r == i;
            })) {
            continue;
        }
        std::fill(replacements.begin(), replacements.end(), i);
        if (attempt(replacements)) {
            return true;
        }
    }
    return false;
}
}  // namespace openrac::lighting::bake_detail
