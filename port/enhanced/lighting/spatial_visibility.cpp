// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <bitset>
#include <cmath>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/spatial_watershed.h"

namespace openrac::lighting::bake_detail {
namespace {
constexpr unsigned sample_limit = 32, landmark_limit = 128;
using View = std::bitset<landmark_limit>;

float distance(Float3 a, Float3 b) {
    const auto d = add(a, mul(b, -1));
    return std::sqrt(dot(d, d));
}

float difference(const View& a, const View& b) {
    const auto common = (a | b).count();
    return common ? float((a ^ b).count()) / float(common) : 0;
}

// Deterministic farthest-point coverage, rather than density-weighted sampling.
std::vector<unsigned> spread(std::span<const Float3> points, unsigned limit) {
    if (points.empty()) {
        return {};
    }
    std::vector<unsigned> result{0};
    std::vector<float> nearest(points.size(), INFINITY);
    while (result.size() < std::min<std::size_t>(limit, points.size())) {
        const auto p = points[result.back()];
        for (unsigned i = 0; i < points.size(); ++i) {
            nearest[i] = std::min(nearest[i], distance(p, points[i]));
        }
        const auto next =
            unsigned(std::max_element(nearest.begin(), nearest.end()) - nearest.begin());
        if (nearest[next] < 1e-4f) {
            break;
        }
        result.push_back(next);
    }
    return result;
}

struct Representative {
    unsigned cell;
    float weight;
    View view;
};

std::vector<Representative> representatives(
    const SpatialWatershed& field, std::span<const unsigned> members, float player_height
) {
    std::map<unsigned, std::vector<unsigned>> columns;
    for (const auto i : members) {
        columns[field.cells[i].span].push_back(i);
    }
    std::vector<Representative> candidates;
    std::vector<Float3> positions;
    for (const auto& [span_id, cells] : columns) {
        const auto& span = field.spans[span_id];
        const float near_floor = std::min(span.top, span.bottom + player_height * .5f);
        unsigned previous = ~0u;
        // Receiver-height views carry twice the weight of the volume midpoint.
        // Both remain genuinely 3D; this does not extrude a floor partition.
        for (const auto [height, weight] :
             {std::pair{near_floor, 2.f}, std::pair{(span.bottom + span.top) * .5f, 1.f}}) {
            const auto best =
                *std::min_element(cells.begin(), cells.end(), [&](unsigned a, unsigned b) {
                    return std::abs(field.cells[a].position[2] - height)
                           < std::abs(field.cells[b].position[2] - height);
                });
            if (best == previous) {
                continue;
            }
            previous = best;
            candidates.push_back({best, weight, {}});
            positions.push_back(field.cells[best].position);
        }
    }
    std::vector<Representative> result;
    for (const auto i : spread(positions, sample_limit)) {
        result.push_back(candidates[i]);
    }
    return result;
}

std::vector<Float3> sample_views(
    const Scene& scene,
    const SpatialWatershed& field,
    std::vector<Representative>& reps,
    const BakeSettings& settings,
    float radius,
    std::vector<Float3>& normals
) {
    std::vector<Float3> candidates, candidate_normals;
    std::set<std::array<int, 4>> bins;
    for (const auto& rep : reps) {
        const auto p = field.cells[rep.cell].position;
        for (unsigned r = 0; r < 32; ++r) {
            const float z = 1.f - 2.f * (float(r) + .5f) / 32.f;
            const float angle = float(r) * 2.39996323f, xy = std::sqrt(1 - z * z);
            const Float3 dir{xy * std::cos(angle), xy * std::sin(angle), z};
            const auto hit = scene.trace(p, dir, settings.ray_bias, radius);
            // Compare actual shared surface landmarks. Synthetic miss endpoints
            // would flood outdoor signatures with identical empty-sky witnesses.
            if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                continue;
            }
            const float d = hit.ray.tfar;
            if (d < 1) {
                continue;
            }
            const auto point = add(p, mul(dir, d));
            Float3 normal{hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z};
            const float normal_length = std::sqrt(dot(normal, normal));
            if (normal_length <= 0) {
                continue;
            }
            normal = mul(normal, (dot(normal, dir) > 0 ? -1.f : 1.f) / normal_length);
            unsigned axis = 0;
            for (unsigned k = 1; k < 3; ++k) {
                if (std::abs(normal[k]) > std::abs(normal[axis])) {
                    axis = k;
                }
            }
            std::array<int, 4> bin{};
            bin[3] = int(axis) * 2 + (normal[axis] > 0 ? 1 : 0);
            for (unsigned k = 0; k < 3; ++k) {
                bin[k] = int(std::floor(point[k] / 2));
            }
            if (bins.insert(bin).second) {
                candidates.push_back(point);
                candidate_normals.push_back(normal);
            }
        }
    }
    std::vector<Float3> landmarks;
    for (const auto i : spread(candidates, landmark_limit)) {
        landmarks.push_back(candidates[i]);
        normals.push_back(candidate_normals[i]);
    }
    for (auto& rep : reps) {
        const auto p = field.cells[rep.cell].position;
        for (unsigned l = 0; l < landmarks.size(); ++l) {
            const auto delta = add(landmarks[l], mul(p, -1));
            const float d = std::sqrt(dot(delta, delta));
            const float bias = std::max(.05f, settings.ray_bias * 2);
            // Opposite faces of a thin wall are different surroundings.
            if (dot(normals[l], mul(delta, -1)) <= 0) {
                continue;
            }
            if (d <= bias * 2
                || !scene.occluded(p, mul(delta, 1 / d), settings.ray_bias, d - bias)) {
                rep.view.set(l);
            }
        }
    }
    return landmarks;
}

bool substantial(
    const SpatialWatershed& field,
    const std::vector<Representative>& reps,
    const std::vector<unsigned>& groups
) {
    float weights[2]{}, total = 0;
    std::set<unsigned> spans[2];
    for (unsigned i = 0; i < reps.size(); ++i) {
        weights[groups[i]] += reps[i].weight;
        total += reps[i].weight;
        spans[groups[i]].insert(field.cells[reps[i].cell].span);
    }
    return spans[0].size() >= 4 && spans[1].size() >= 4 && weights[0] >= total * .2f
           && weights[1] >= total * .2f;
}
}  // namespace

void refine_spatial_visibility(
    const Scene& scene,
    SpatialWatershed& field,
    const BakeSettings& settings,
    const SpatialWatershedSettings& options
) {
    if (!std::isfinite(options.visibility_radius) || options.visibility_radius <= 0
        || options.visibility_radius > 1000000 || !std::isfinite(options.visibility_split_gain)
        || options.visibility_split_gain <= 0 || options.visibility_split_gain > 1
        || !std::isfinite(settings.ray_bias) || settings.ray_bias <= 0
        || !std::isfinite(settings.traversal.player_height)
        || settings.traversal.player_height <= 0) {
        throw std::runtime_error("invalid spatial visibility settings");
    }
    field.visibility_splits.clear();
    field.visibility_assessments.clear();
    const auto n = unsigned(field.cells.size());
    std::vector<std::vector<unsigned>> members(field.basins.size());
    for (unsigned i = 0; i < n; ++i) {
        members[field.cells[i].basin].push_back(i);
    }
    std::vector<unsigned> depth(members.size()), origins(members.size()), tasks(members.size());
    std::iota(origins.begin(), origins.end(), 0u);
    std::iota(tasks.begin(), tasks.end(), 0u);
    for (unsigned task = 0; task < tasks.size(); ++task) {
        const auto basin = tasks[task];
        // Bounded refinement: at most four visibility regions per watershed basin.
        if (depth[basin] >= 2) {
            continue;
        }
        auto reps = representatives(field, members[basin], settings.traversal.player_height);
        if (reps.size() < 8) {
            continue;
        }
        std::vector<Float3> landmark_normals;
        auto landmarks =
            sample_views(scene, field, reps, settings, options.visibility_radius, landmark_normals);
        if (landmarks.size() < 8) {
            continue;
        }
        const auto count = unsigned(reps.size());
        std::vector<std::vector<float>> differences(count, std::vector<float>(count));
        float total = 0, before = INFINITY;
        for (const auto& r : reps) {
            total += r.weight;
        }
        for (unsigned i = 0; i < count; ++i) {
            float cost = 0;
            for (unsigned j = 0; j < count; ++j) {
                differences[i][j] = difference(reps[i].view, reps[j].view);
                cost += reps[j].weight * differences[i][j];
            }
            before = std::min(before, cost / total);
        }
        float after = before;
        unsigned first = 0, second = 0;
        std::vector<unsigned> groups(count);
        // Exhaustive two-medoid proposals are cheap at 32 representatives.
        for (unsigned a = 0; a < count; ++a) {
            for (unsigned b = a + 1; b < count; ++b) {
                float cost = 0;
                for (unsigned i = 0; i < count; ++i) {
                    groups[i] = differences[i][a] <= differences[i][b] ? 0 : 1;
                    cost += reps[i].weight * std::min(differences[i][a], differences[i][b]);
                }
                cost /= total;
                if (cost < after && substantial(field, reps, groups)) {
                    after = cost;
                    first = a;
                    second = b;
                }
            }
        }
        field.visibility_assessments.push_back({basin, before, after, -1});
        if (before - after < options.visibility_split_gain) {
            continue;
        }

        using Visit = std::tuple<float, unsigned, unsigned>;
        std::priority_queue<Visit, std::vector<Visit>, std::greater<Visit>> pending;
        std::vector<unsigned> nearest(n, ~0u), owner(n, ~0u);
        // Interpolate the sampled view evidence along the existing free-space graph.
        for (unsigned r = 0; r < count; ++r) {
            pending.push({0, r, reps[r].cell});
        }
        while (!pending.empty()) {
            const auto [cost, r, i] = pending.top();
            pending.pop();
            if (nearest[i] != ~0u) {
                continue;
            }
            nearest[i] = r;
            for (const auto j : field.neighbours(i)) {
                if (field.cells[j].basin == basin && nearest[j] == ~0u) {
                    pending.push(
                        {cost + distance(field.cells[i].position, field.cells[j].position), r, j}
                    );
                }
            }
        }
        pending.push({0, 0, reps[first].cell});
        pending.push({0, 1, reps[second].cell});
        while (!pending.empty()) {
            const auto [cost, g, i] = pending.top();
            pending.pop();
            if (owner[i] != ~0u || (i == reps[first].cell && g != 0)
                || (i == reps[second].cell && g != 1)) {
                continue;
            }
            owner[i] = g;
            for (const auto j : field.neighbours(i)) {
                if (field.cells[j].basin == basin && owner[j] == ~0u) {
                    const float mismatch = differences[nearest[j]][g ? second : first];
                    const float step = distance(field.cells[i].position, field.cells[j].position);
                    pending.push({cost + step * (.1f + mismatch * mismatch), g, j});
                }
            }
        }
        if (std::any_of(members[basin].begin(), members[basin].end(), [&](unsigned i) {
                return owner[i] == ~0u;
            })) {
            continue;
        }
        float verified = 0;
        for (unsigned r = 0; r < count; ++r) {
            groups[r] = owner[reps[r].cell];
            verified += reps[r].weight * differences[r][groups[r] ? second : first];
        }
        verified /= total;
        field.visibility_assessments.back().after = verified;
        if (before - verified < options.visibility_split_gain
            || !substantial(field, reps, groups)) {
            continue;
        }
        const auto child = unsigned(field.basins.size());
        SpatialVisibilitySplit split;
        split.parent = origins[basin];
        split.a = basin;
        split.b = child;
        split.before = before;
        split.after = verified;
        split.landmarks = std::move(landmarks);
        split.landmark_normals = std::move(landmark_normals);
        for (unsigned r = 0; r < count; ++r) {
            SpatialVisibilitySample sample;
            sample.cell = reps[r].cell;
            sample.basin = groups[r] ? child : basin;
            for (unsigned l = 0; l < split.landmarks.size(); ++l) {
                if (reps[r].view[l]) {
                    sample.visible.push_back(l);
                }
            }
            split.samples.push_back(std::move(sample));
        }
        std::vector<unsigned> a, b;
        for (const auto i : members[basin]) {
            (owner[i] ? b : a).push_back(i);
            if (owner[i]) {
                field.cells[i].basin = child;
            }
        }
        members[basin] = std::move(a);
        members.push_back(std::move(b));
        field.basins.push_back(field.basins[basin]);
        ++depth[basin];
        depth.push_back(depth[basin]);
        origins.push_back(origins[basin]);
        field.visibility_splits.push_back(std::move(split));
        // Test both children, keeping the refinement hierarchy bounded.
        if (depth[basin] < 2) {
            tasks.push_back(basin);
            tasks.push_back(child);
        }
    }
    for (auto& split : field.visibility_splits) {
        for (auto& sample : split.samples) {
            sample.basin = field.cells[sample.cell].basin;
        }
    }
    if (!field.visibility_splits.empty()) {
        rebuild_spatial_basins(field);
    }
    scene.check_error();
}
}  // namespace openrac::lighting::bake_detail
