// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/spatial_volume_fit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <tuple>

#include "common/log.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/probe_fit.h"

namespace openrac::lighting::bake_detail {
SpatialVolumeFitData::SpatialVolumeFitData(const SpatialWatershed& field) : m_field(field) {
    if (field.uses_tiled_links()) {
        // Reuse the allowance for the two radix buffers, which have already been released.
        // Scatter from global link order to reproduce the incidence order at both endpoints.
        m_neighbours.resize(field.incident.size());
        auto cursor = field.offsets;
        const auto count = field.link_count();
        for (unsigned i = 0; i < count; ++i) {
            const auto edge = field.link(i);
            m_neighbours[cursor[edge.a]++] = edge.b;
            m_neighbours[cursor[edge.b]++] = edge.a;
        }
    }
    m_sorted.resize(field.cells.size());
    std::iota(m_sorted.begin(), m_sorted.end(), 0u);
    std::stable_sort(m_sorted.begin(), m_sorted.end(), [&](unsigned a, unsigned b) {
        return field.cells[a].position[0] < field.cells[b].position[0];
    });
    m_weights.resize(field.cells.size());
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        const auto& cell = field.cells[i];
        const auto& span = field.spans[cell.span];
        float step = 2;
        for (auto j : neighbours(i)) {
            const auto p = field.cells[j].position;
            const float d = std::hypot(p[0] - cell.position[0], p[1] - cell.position[1]);
            if (d > .001f) {
                step = std::min(step, d);
            }
        }
        const float height = float(
            (span.top - span.bottom) / std::max(1., std::ceil(double(span.top - span.bottom)))
        );
        m_weights[i] = step * step * height;
    }
}

namespace {
double product(Float3 p) {
    return double(p[0]) * p[1] * p[2];
}

class Fitter {
    const SpatialVolumeFitData& data;
    const SpatialWatershed& field;
    const SpatialVolumeFitSettings& options;
    std::span<const unsigned> sorted;
    std::span<const float> weights;
    double unit_volume;

public:
    Fitter(const SpatialVolumeFitData& input, const SpatialVolumeFitSettings& o)
        : data(input),
          field(input.field()),
          options(o),
          sorted(input.sorted_cells()),
          weights(input.cell_weights()),
          unit_volume(product(o.spacing)) {}

    SpatialFittedVolume bounds(
        std::span<const Float3> points, ProbeFrame frame, unsigned basin
    ) const {
        SpatialFittedVolume box;
        box.basin = basin;
        box.frame = frame;
        const auto anchor = points.front();
        Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
        float coordinate_scale = 1;
        for (auto point : points) {
            for (auto value : point) {
                coordinate_scale = std::max(coordinate_scale, std::abs(value));
            }
            const auto p = frame.to_local(add(point, mul(anchor, -1)));
            for (unsigned k = 0; k < 3; ++k) {
                lo[k] = std::min(lo[k], p[k] - options.padding);
                hi[k] = std::max(hi[k], p[k] + options.padding);
            }
        }
        box.feasible = true;
        Float3 origin = lo;
        for (unsigned k = 0; k < 3; ++k) {
            box.extent[k] = hi[k] - lo[k];
            if (options.kind == SpatialVolumeKind::gi) {
                const double tolerance = std::max(
                    1e-5,
                    double(coordinate_scale) * std::numeric_limits<float>::epsilon() * 4
                        / options.spacing[k]
                );
                const double count =
                    std::ceil(double(box.extent[k]) / options.spacing[k] - tolerance) + 3;
                if (count > 1000000) {
                    throw std::runtime_error("spatial fitted grid is too large");
                }
                box.dimensions[k] = std::max(3u, unsigned(count));
                box.extent[k] = float(box.dimensions[k] - 1) * options.spacing[k];
                origin[k] = (lo[k] + hi[k] - box.extent[k]) * .5f;
                box.feasible &= box.dimensions[k] <= options.maximum_dimension;
            } else {
                box.feasible &= box.extent[k] <= options.maximum_extent;
            }
        }
        box.origin = add(anchor, frame.to_world(origin));
        box.volume = product(box.extent);
        double resource = box.volume / unit_volume, overflow = 0;
        if (options.kind == SpatialVolumeKind::gi) {
            resource = double(box.dimensions[0]) * box.dimensions[1] * box.dimensions[2];
        }
        for (unsigned k = 0; k < 3; ++k) {
            overflow += std::max(
                0.,
                options.kind == SpatialVolumeKind::gi
                    ? double(box.dimensions[k]) / options.maximum_dimension - 1
                    : double(box.extent[k]) / options.maximum_extent - 1
            );
        }
        box.score = resource + options.volume_cost + resource * overflow * 10;
        return box;
    }

    void measure(SpatialFittedVolume& box) const {
        Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
        for (unsigned i = 0; i < 8; ++i) {
            auto p = box.origin;
            Float3 q{};
            for (unsigned k = 0; k < 3; ++k) {
                if (i & (1 << k)) {
                    q[k] = box.extent[k];
                }
            }
            p = add(p, box.frame.to_world(q));
            for (unsigned k = 0; k < 3; ++k) {
                lo[k] = std::min(lo[k], p[k]);
                hi[k] = std::max(hi[k], p[k]);
            }
        }
        const auto start =
            std::lower_bound(sorted.begin(), sorted.end(), lo[0], [&](unsigned i, float x) {
                return field.cells[i].position[0] < x;
            });
        for (auto it = start; it != sorted.end() && field.cells[*it].position[0] <= hi[0]; ++it) {
            const auto& c = field.cells[*it];
            if (c.position[1] < lo[1] || c.position[1] > hi[1] || c.position[2] < lo[2]
                || c.position[2] > hi[2]) {
                continue;
            }
            const auto p = box.frame.to_local(add(c.position, mul(box.origin, -1)));
            bool inside = true;
            for (unsigned k = 0; k < 3; ++k) {
                inside &= p[k] >= 0 && p[k] <= box.extent[k];
            }
            if (inside) {
                if (c.basin == box.basin) {
                    box.occupied_volume += weights[*it];
                } else {
                    box.foreign_volume += weights[*it];
                }
            }
        }
        box.score += 2 * box.foreign_volume / unit_volume;
    }

    SpatialFittedVolume fit(
        std::span<const unsigned> cells,
        unsigned basin,
        std::span<const ProbeFrame> seeds = {},
        bool refine = true
    ) const {
        std::vector<Float3> support;
        std::vector<bool> member(field.cells.size());
        for (auto i : cells) {
            member[i] = true;
        }
        for (auto i : cells) {
            support.push_back(field.cells[i].position);
            // Shared graph-edge midpoints plus the blend margin prevent seams
            // between independently rotated boxes and adjacent basin volumes.
            for (auto j : data.neighbours(i)) {
                if (!member[j]) {
                    support.push_back(
                        mul(add(field.cells[i].position, field.cells[j].position), .5f)
                    );
                }
            }
        }
        std::vector<Float3> points;
        if (refine) {
            for (int x = -1; x <= 1; ++x) {
                for (int y = -1; y <= 1; ++y) {
                    for (int z = -1; z <= 1; ++z) {
                        if (!x && !y && !z) {
                            continue;
                        }
                        const Float3 direction{float(x), float(y), float(z)};
                        auto best = support.front();
                        float value = -INFINITY;
                        for (auto p : support) {
                            const auto score = dot(add(p, mul(support.front(), -1)), direction);
                            if (score > value) {
                                value = score;
                                best = p;
                            }
                        }
                        points.push_back(best);
                    }
                }
            }
        }
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
        SpatialFittedVolume best = bounds(support, {}, basin);
        measure(best);
        const auto consider = [&](ProbeFrame frame) {
            auto box = bounds(support, frame, basin);
            measure(box);
            const auto complexity = [](ProbeFrame f) {
                return std::abs(f.roll[1]) > .001f                                   ? 2
                       : std::abs(f.pitch[1]) > .001f || std::abs(f.axis[1]) > .001f ? 1
                                                                                     : 0;
            };
            const int a = complexity(frame), b = complexity(best.frame);
            const double tolerance = .005 * std::max(1., best.score - options.volume_cost);
            if ((box.feasible && !best.feasible)
                || (box.feasible == best.feasible
                    && ((a > b && box.score < best.score - tolerance)
                        || (a == b && box.score < best.score - 1e-6)
                        || (a < b && box.score <= best.score + tolerance)))) {
                best = std::move(box);
            }
        };
        for (const auto frame : seeds) {
            consider(frame);
        }
        if (refine && points.size() > 1) {
            // Search against the coreset; the objective includes real padding
            // and GI grid quantisation, rather than an unpadded continuous OBB.
            const auto cost = [&](ProbeFrame frame) {
                return bounds(points, frame, basin).score;
            };
            const auto simple = search_probe_frame(points, cost, seeds, true, false);
            consider(simple);
            const auto baseline = best;
            const std::array<ProbeFrame, 1> starting{simple};
            const auto full = search_probe_frame(points, cost, starting, true, true);
            auto rolled = bounds(support, full, basin);
            measure(rolled);
            // Near-equivalent fits retain the simpler frame. This also avoids
            // spending probes on insignificant roll or lattice-rounding noise.
            if ((rolled.feasible && !baseline.feasible)
                || (rolled.feasible == baseline.feasible
                    && rolled.score
                           < baseline.score
                                 - .005 * std::max(1., baseline.score - options.volume_cost))) {
                best = std::move(rolled);
            }
        }
        best.cells.assign(cells.begin(), cells.end());
        const auto centre = add(best.origin, best.frame.to_world(mul(best.extent, .5f)));
        unsigned representative = cells.front();
        float closest = INFINITY;
        for (auto i : cells) {
            const auto d = add(field.cells[i].position, mul(centre, -1));
            const auto distance = dot(d, d);
            if (distance < closest) {
                closest = distance;
                representative = i;
            }
        }
        best.representative = field.cells[representative].position;
        return best;
    }

    using ProjectedCell = std::pair<float, unsigned>;

    std::vector<ProjectedCell> project_cut_axis(
        const SpatialFittedVolume& box, unsigned axis
    ) const {
        std::vector<ProjectedCell> ordered;
        ordered.reserve(box.cells.size());
        for (auto i : box.cells) {
            ordered.push_back(
                {box.frame.to_local(add(field.cells[i].position, mul(box.origin, -1)))[axis], i}
            );
        }
        std::stable_sort(ordered.begin(), ordered.end());
        return ordered;
    }

    std::vector<std::vector<unsigned>> cut(
        std::span<const unsigned> cells, std::span<const ProjectedCell> ordered, float fraction
    ) const {
        const float plane =
            ordered[std::min(ordered.size() - 1, std::size_t(double(ordered.size()) * fraction))]
                .first;
        std::vector<int> side(field.cells.size(), -1);
        for (const auto [p, i] : ordered) {
            side[i] = p < plane ? 0 : 1;
        }
        std::vector<std::vector<unsigned>> groups;
        for (auto i : cells) {
            if (side[i] < 0) {
                continue;
            }
            const auto group = side[i];
            groups.push_back({i});
            side[i] = -1;
            for (unsigned head = 0; head < groups.back().size(); ++head) {
                for (auto j : data.neighbours(groups.back()[head])) {
                    if (side[j] == group) {
                        side[j] = -1;
                        groups.back().push_back(j);
                    }
                }
            }
        }
        if (groups.size() < 2) {
            return {};
        }
        if (groups.size() > 2 && ordered.front().first < ordered.back().first) {
            // A plane can leave many little islands around holes or bends.
            // Grow two connected pieces from its extrema instead of consuming
            // the whole volume budget with those disconnected fragments.
            std::vector<int> desired(field.cells.size(), -1), owner(field.cells.size(), -1);
            for (const auto [p, i] : ordered) {
                desired[i] = p < plane ? 0 : 1;
            }
            using Visit = std::tuple<float, unsigned, unsigned>;
            std::priority_queue<Visit, std::vector<Visit>, std::greater<Visit>> pending;
            std::vector<std::pair<float, unsigned>> best_visit(field.cells.size(), {INFINITY, ~0u});
            const auto enqueue = [&](float cost, unsigned group, unsigned cell) {
                const auto visit = std::pair{cost, group};
                // Any worse visit would be popped after this cell has already been claimed.
                if (visit < best_visit[cell]) {
                    best_visit[cell] = visit;
                    pending.emplace(cost, group, cell);
                }
            };
            enqueue(0, 0, ordered.front().second);
            enqueue(0, 1, ordered.back().second);
            std::vector<std::vector<unsigned>> connected(2);
            while (!pending.empty()) {
                const auto [cost, g, i] = pending.top();
                pending.pop();
                if (owner[i] >= 0) {
                    continue;
                }
                owner[i] = int(g);
                connected[g].push_back(i);
                for (auto j : data.neighbours(i)) {
                    if (desired[j] >= 0 && owner[j] < 0) {
                        const auto d =
                            add(field.cells[j].position, mul(field.cells[i].position, -1));
                        enqueue(
                            cost + std::sqrt(dot(d, d)) * (desired[j] == int(g) ? .25f : 1.f), g, j
                        );
                    }
                }
            }
            if (connected[0].size() + connected[1].size() == cells.size() && !connected[0].empty()
                && !connected[1].empty()) {
                return connected;
            }
        }
        return groups;
    }

    bool adjacent(const SpatialFittedVolume& a, const SpatialFittedVolume& b) const {
        std::vector<bool> marked(field.cells.size());
        for (auto i : a.cells) {
            marked[i] = true;
        }
        for (auto i : b.cells) {
            for (auto j : data.neighbours(i)) {
                if (marked[j]) {
                    return true;
                }
            }
        }
        return false;
    }
};
}  // namespace

SpatialVolumePlan fit_spatial_volumes(
    const SpatialWatershed& field,
    const SpatialVolumeFitSettings& options,
    const BakeExecution& execution
) {
    return fit_spatial_volumes(SpatialVolumeFitData(field), options, execution);
}

SpatialVolumePlan fit_spatial_volumes(
    const SpatialVolumeFitData& data,
    const SpatialVolumeFitSettings& options,
    const BakeExecution& execution
) {
    const auto& field = data.field();
    const bool gi = options.kind == SpatialVolumeKind::gi;
    const unsigned format_limit = gi ? 1024 : 512;
    const unsigned volume_limit = options.maximum_volumes ? options.maximum_volumes : format_limit;
    if (!std::isfinite(options.padding) || options.padding <= 0
        || !std::isfinite(options.maximum_extent) || options.maximum_extent <= 0
        || options.maximum_dimension < 3 || (!gi && options.maximum_parts == 0)
        || options.maximum_parts > format_limit || volume_limit > format_limit
        || !std::isfinite(options.volume_cost) || options.volume_cost < 0) {
        throw std::runtime_error("invalid spatial volume fit settings");
    }
    for (auto step : options.spacing) {
        if (!std::isfinite(step) || step <= 0) {
            throw std::runtime_error("invalid spatial volume spacing");
        }
    }
    SpatialVolumePlan plan;
    plan.settings = options;
    Fitter fitter(data, options);
    std::vector<std::vector<unsigned>> basins(field.basins.size());
    for (unsigned i = 0; i < field.cells.size(); ++i) {
        basins.at(field.cells[i].basin).push_back(i);
    }
    const auto nonempty =
        unsigned(std::count_if(basins.begin(), basins.end(), [](const auto& cells) {
            return !cells.empty();
        }));
    // Independent fits can borrow the spare capacity, while the assembled plan must
    // still fit the shared budget. Keep one slot for each other nonempty basin.
    const auto gi_allowance = 1 + volume_limit - std::min(volume_limit, nonempty);
    const auto part_limit =
        gi ? (options.maximum_parts ? std::min(options.maximum_parts, gi_allowance) : gi_allowance)
           : options.maximum_parts;
    const auto overflow = [&](const SpatialFittedVolume& part) {
        return gi && !part.feasible
                   ? *std::max_element(part.dimensions.begin(), part.dimensions.end())
                         - options.maximum_dimension
                   : 0u;
    };

    struct BasinFit {
        double single_box_volume = 0;
        std::vector<SpatialFittedVolume> parts;
    };

    std::vector<BasinFit> fitted(basins.size());
    parallel_bake_indices(basins.size(), execution, [&](std::size_t index) {
        const auto basin = unsigned(index);
        if (basins[basin].empty()) {
            return;
        }
        auto root = fitter.fit(basins[basin], basin);
        auto& [single_box_volume, parts] = fitted[basin];
        single_box_volume = root.volume;
        parts.push_back(std::move(root));
        bool changed = true;
        while (changed && parts.size() < part_limit) {
            changed = false;
            std::vector<unsigned> pending;
            for (unsigned part = 0; part < parts.size(); ++part) {
                if (!parts[part].feasible) {
                    pending.push_back(part);
                }
            }
            if (gi) {
                std::stable_sort(pending.begin(), pending.end(), [&](auto a, auto b) {
                    return overflow(parts[a]) > overflow(parts[b]);
                });
            }
            for (const auto part : pending) {
                const auto& parent = parts[part];
                struct Proposal {
                    std::pair<unsigned, double> rank;
                    std::vector<std::vector<unsigned>> groups;
                };

                std::vector<Proposal> proposals;
                const std::array<ProbeFrame, 1> seed{parent.frame};
                for (unsigned axis = 0; axis < 3; ++axis) {
                    const auto ordered = fitter.project_cut_axis(parent, axis);
                    for (float fraction : {.2f, .35f, .5f, .65f, .8f}) {
                        auto groups = fitter.cut(parent.cells, ordered, fraction);
                        if (groups.empty() || parts.size() + groups.size() - 1 > part_limit) {
                            continue;
                        }
                        std::pair<unsigned, double> rank{};
                        for (const auto& group : groups) {
                            const auto child = fitter.fit(group, basin, seed, false);
                            rank.first = std::max(rank.first, overflow(child));
                            rank.second += child.score;
                        }
                        proposals.push_back({rank, std::move(groups)});
                    }
                }
                std::stable_sort(
                    proposals.begin(), proposals.end(), [](const auto& a, const auto& b) {
                        return a.rank < b.rank;
                    }
                );
                std::pair<unsigned, double> best{~0u, INFINITY};
                std::vector<SpatialFittedVolume> replacement;
                for (unsigned p = 0; p < std::min<std::size_t>(2, proposals.size()); ++p) {
                    std::vector<SpatialFittedVolume> children;
                    std::pair<unsigned, double> rank{};
                    for (const auto& group : proposals[p].groups) {
                        children.push_back(fitter.fit(group, basin, seed));
                        rank.first = std::max(rank.first, overflow(children.back()));
                        rank.second += children.back().score;
                    }
                    if (rank < best) {
                        best = rank;
                        replacement = std::move(children);
                    }
                }
                if (!replacement.empty()) {
                    parts[part] = std::move(replacement.front());
                    for (unsigned i = 1; i < replacement.size(); ++i) {
                        parts.push_back(std::move(replacement[i]));
                    }
                    changed = true;
                    break;
                }
            }
        }
        if (gi && parts.size() == part_limit) {
            const auto oversized = std::count_if(parts.begin(), parts.end(), [](const auto& part) {
                return !part.feasible;
            });
            if (oversized) {
                log::info(
                    "layered GI split: basin {} ({} cells) reached {} parts with {} oversized "
                    "grids",
                    basin,
                    basins[basin].size(),
                    parts.size(),
                    oversized
                );
            }
        }
        // Adjacent parts can become redundant after independent rotations.
        bool merged = true;
        while (merged) {
            merged = false;
            for (unsigned a = 0; a < parts.size() && !merged; ++a) {
                for (unsigned b = a + 1; b < parts.size(); ++b) {
                    if (!fitter.adjacent(parts[a], parts[b])) {
                        continue;
                    }
                    auto cells = parts[a].cells;
                    cells.insert(cells.end(), parts[b].cells.begin(), parts[b].cells.end());
                    const std::array<ProbeFrame, 2> seeds{parts[a].frame, parts[b].frame};
                    auto combined = fitter.fit(cells, basin, seeds);
                    if (combined.feasible) {
                        parts[a] = std::move(combined);
                        parts.erase(parts.begin() + b);
                        merged = true;
                        break;
                    }
                }
            }
        }
    });

    // Preserve basin order, including the order of floating-point totals and volume IDs.
    for (auto& [single_box_volume, parts] : fitted) {
        plan.single_box_volume += single_box_volume;
        for (auto& box : parts) {
            if (options.kind == SpatialVolumeKind::gi) {
                plan.estimated_probes +=
                    std::uint64_t(box.dimensions[0]) * box.dimensions[1] * box.dimensions[2];
            }
            plan.fitted_volume += box.volume;
            plan.over_budget += !box.feasible;
            plan.volumes.push_back(std::move(box));
        }
    }
    std::vector<bool> covered(field.cells.size());
    for (const auto& box : plan.volumes) {
        for (auto i : box.cells) {
            const auto p = box.frame.to_local(add(field.cells[i].position, mul(box.origin, -1)));
            bool inside = true;
            for (unsigned k = 0; k < 3; ++k) {
                inside &= p[k] >= -.001f && p[k] <= box.extent[k] + .001f;
            }
            covered[i] = inside;
        }
    }
    plan.covered = unsigned(std::count(covered.begin(), covered.end(), true));
    plan.uncovered = unsigned(field.cells.size()) - plan.covered;
    plan.volume_limit_exceeded = plan.volumes.size() > volume_limit;
    return plan;
}

std::vector<float> spatial_reflection_coverage_tops(
    const BakeScene& geometry, const SpatialWatershed& field, float outdoor_height, float ray_bias
) {
    if (!std::isfinite(outdoor_height) || outdoor_height <= 0 || !std::isfinite(ray_bias)
        || ray_bias <= 0) {
        throw std::runtime_error("invalid reflection coverage query");
    }
    const Scene scene(geometry);
    float maximum_z = -INFINITY;
    for (const auto& triangle : geometry.triangles) {
        for (const auto& p : triangle.vertices) {
            maximum_z = std::max(maximum_z, p[2]);
        }
    }
    std::vector<float> tops;
    for (const auto& span : field.spans) {
        auto start = span.floor;
        start[2] += std::max(.05f, ray_bias * 2);
        // Search beyond the analysis cap to avoid treating tall rooms as sky.
        const float far = std::max(outdoor_height, maximum_z - start[2] + 1);
        const auto roof = scene.trace(start, {0, 0, 1}, ray_bias, far);
        tops.push_back(
            span.underwater                              ? span.top
            : roof.hit.geomID != RTC_INVALID_GEOMETRY_ID ? start[2] + roof.ray.tfar
                                                         : span.floor[2] + outdoor_height
        );
    }
    return tops;
}

void extend_spatial_reflection_coverage(
    const SpatialWatershed& field,
    SpatialVolumePlan& plan,
    std::span<const float> span_tops,
    const BakeExecution& execution
) {
    if (plan.settings.kind == SpatialVolumeKind::reflection) {
        extend_spatial_reflection_coverage(SpatialVolumeFitData(field), plan, span_tops, execution);
    }
}

void extend_spatial_reflection_coverage(
    const SpatialVolumeFitData& data,
    SpatialVolumePlan& plan,
    std::span<const float> span_tops,
    const BakeExecution& execution
) {
    const auto& field = data.field();
    if (plan.settings.kind != SpatialVolumeKind::reflection) {
        return;
    }
    if (span_tops.size() != field.spans.size()
        || std::any_of(span_tops.begin(), span_tops.end(), [](float z) {
               return !std::isfinite(z);
           })) {
        throw std::runtime_error("invalid reflection coverage heights");
    }
    Fitter fitter(data, plan.settings);
    parallel_bake_indices(plan.volumes.size(), execution, [&](std::size_t index) {
        auto& box = plan.volumes[index];
        const auto& original = box;
        const auto contains = [](const SpatialFittedVolume& v, Float3 p) {
            p = v.frame.to_local(add(p, mul(v.origin, -1)));
            return p[0] >= -.001f && p[1] >= -.001f && p[2] >= -.001f && p[0] <= v.extent[0] + .001f
                   && p[1] <= v.extent[1] + .001f && p[2] <= v.extent[2] + .001f;
        };
        std::vector<bool> owned(field.cells.size());
        std::vector<Float3> targets;
        for (auto i : box.cells) {
            owned[i] = true;
            const auto& c = field.cells[i];
            const auto& s = field.spans[c.span];
            // Only the part owning a column's highest band grows upward. Lower
            // stacked parts must not expand through the part above them.
            const float band = (s.top - s.bottom) / std::max(1.f, std::ceil(s.top - s.bottom));
            if (!s.underwater && c.position[2] >= s.top - band) {
                auto p = c.position;
                p[2] = std::max(p[2], span_tops[c.span]);
                targets.push_back(p);
            }
        }
        const auto candidate = [&](float fraction) {
            auto v = original;
            Float3 lo{}, hi = original.extent;
            for (auto p : targets) {
                // Grow in world Z, then project into the existing fitted frame.
                // Do not stretch local Z on tilted pipes and sloping rooms.
                auto q = original.frame.to_local(add(p, mul(original.origin, -1)));
                for (unsigned k = 0; k < 3; ++k) {
                    lo[k] = std::min(lo[k], (q[k] - plan.settings.padding) * fraction);
                    hi[k] = std::max(
                        hi[k],
                        original.extent[k]
                            + (q[k] + plan.settings.padding - original.extent[k]) * fraction
                    );
                }
            }
            v.origin = add(original.origin, original.frame.to_world(lo));
            v.extent = add(hi, mul(lo, -1));
            return v;
        };
        std::vector<unsigned> outside;
        for (unsigned i = 0; i < field.cells.size(); ++i) {
            if (!owned[i] && !contains(original, field.cells[i].position)) {
                outside.push_back(i);
            }
        }
        // Bound newly engulfed receiver support, including other parts of this
        // basin. Existing blend overlaps do not consume the growth allowance.
        const unsigned allowance = unsigned(box.cells.size() / 20);
        const auto acceptable = [&](const SpatialFittedVolume& v) {
            for (unsigned k = 0; k < 3; ++k) {
                if (v.extent[k]
                    > std::max(original.extent[k], plan.settings.maximum_extent) + .001f) {
                    return false;
                }
            }
            unsigned extra = 0;
            for (auto i : outside) {
                if (contains(v, field.cells[i].position) && ++extra > allowance) {
                    return false;
                }
            }
            return true;
        };
        float growth = 1;
        auto expanded = candidate(growth);
        if (!acceptable(expanded)) {
            float lo = 0, hi = 1;
            for (unsigned iteration = 0; iteration < 14; ++iteration) {
                const float mid = (lo + hi) * .5f;
                if (acceptable(candidate(mid))) {
                    lo = mid;
                } else {
                    hi = mid;
                }
            }
            growth = lo;
            expanded = candidate(growth);
        }
        box = std::move(expanded);
        box.coverage_growth = growth;
        box.volume = product(box.extent);
        box.score = box.volume / product(plan.settings.spacing) + plan.settings.volume_cost;
        box.foreign_volume = box.occupied_volume = 0;
        fitter.measure(box);
    });
    plan.fitted_volume = 0;
    for (const auto& box : plan.volumes) {
        plan.fitted_volume += box.volume;
    }
}
}  // namespace openrac::lighting::bake_detail
