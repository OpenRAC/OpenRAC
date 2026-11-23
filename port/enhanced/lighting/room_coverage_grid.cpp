// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Fit final padded GI allocations, splitting only when the probe budget requires it.
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/probe_fit.h"
#include "enhanced/lighting/room_layout.h"

namespace openrac::lighting::bake_detail {
namespace {
struct Column {
    Float3 low, high;
    unsigned island;
};

struct Allocation {
    ProbeGrid grid;
    std::vector<Column> columns;
};

ProbeGrid bounds(
    const ProbeGrid& prototype,
    std::span<const Column> columns,
    ProbeFrame frame,
    Float3 spacing,
    double* padded_volume = nullptr
) {
    auto grid = prototype;
    grid.frame = frame;
    grid.spacing = spacing;
    // Fit relative to the domain, rather than quantising against the world origin.
    const auto anchor = columns.front().low;
    float coordinate_scale = 1;
    Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
    for (const auto& column : columns) {
        for (auto p : {column.low, column.high}) {
            for (auto value : p) {
                coordinate_scale = std::max(coordinate_scale, std::abs(value));
            }
            p = frame.to_local(add(p, mul(anchor, -1)));
            for (unsigned a = 0; a < 3; ++a) {
                lo[a] = std::min(lo[a], p[a]);
                hi[a] = std::max(hi[a], p[a]);
            }
        }
    }
    Float3 origin{};
    if (padded_volume) {
        *padded_volume = 1;
    }
    for (unsigned a = 0; a < 3; ++a) {
        // Float source coordinates carry less precision far from zero. Do not
        // allocate a whole extra cell for sub-ULP noise at an integer extent;
        // the one-cell interpolation apron still strictly contains the support.
        const double tolerance = std::max(
            1e-5, double(coordinate_scale) * std::numeric_limits<float>::epsilon() * 4 / spacing[a]
        );
        const double cells = std::ceil(double(hi[a] - lo[a]) / spacing[a] - tolerance);
        if (cells > 1'000'000) {
            throw std::runtime_error("GI receiver envelope exceeds grid limits");
        }
        grid.dimensions[a] = std::max(3u, unsigned(std::max(0., cells)) + 3);
        if (padded_volume) {
            *padded_volume *= double(hi[a] - lo[a]) + 2 * spacing[a];
        }
        // One-cell apron plus evenly distributed rounding slack determines the
        // translation analytically for every candidate orientation.
        origin[a] = (lo[a] + hi[a] - float(grid.dimensions[a] - 1) * spacing[a]) * .5f;
    }
    grid.origin = add(anchor, frame.to_world(origin));
    grid.max_distance = 1.5f * std::sqrt(dot(spacing, spacing));
    return grid;
}

double volume(const ProbeGrid& grid) {
    double result = 1;
    for (unsigned a = 0; a < 3; ++a) {
        result *= double(grid.dimensions[a] - 1) * grid.spacing[a];
    }
    return result;
}

ProbeGrid fit(
    const ProbeGrid& prototype,
    std::span<const Column> columns,
    Float3 spacing,
    std::span<const ProbeFrame> seeds = {},
    std::size_t maximum_probes = SIZE_MAX,
    double maximum_volume = INFINITY
) {
    std::vector<Float3> points;
    points.reserve(columns.size() * 2);
    for (const auto& column : columns) {
        points.push_back(column.low);
        points.push_back(column.high);
    }
    ProbeGrid best;
    double best_score = INFINITY;
    search_probe_frame(
        points,
        [&](ProbeFrame frame) {
            double padded_volume = 0;
            const auto grid = bounds(prototype, columns, frame, spacing, &padded_volume);
            if (grid.probe_count() > maximum_probes || volume(grid) > maximum_volume) {
                return double(INFINITY);
            }
            // Count is primary; padded physical volume breaks ties without spending
            // additional probes on a prettier continuous OBB.
            double cell_volume = 1;
            for (auto step : spacing) {
                cell_volume *= step;
            }
            const double score =
                double(grid.probe_count()) + .001 * std::log1p(padded_volume / cell_volume);
            if (score < best_score - 1e-9) {
                best_score = score;
                best = grid;
            }
            return score;
        },
        seeds
    );
    return best;
}

bool touches(const ProbeGrid& a, const ProbeGrid& b) {
    const auto box = [](const ProbeGrid& g) {
        std::array<Float3, 2> result{
            {{INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY}}
        };
        for (unsigned corner = 0; corner < 8; ++corner) {
            Float3 p{};
            for (unsigned k = 0; k < 3; ++k) {
                if (corner & (1u << k)) {
                    p[k] = g.spacing[k] * float(g.dimensions[k] - 1);
                }
            }
            p = add(g.origin, g.frame.to_world(p));
            for (unsigned k = 0; k < 3; ++k) {
                result[0][k] = std::min(result[0][k], p[k]);
                result[1][k] = std::max(result[1][k], p[k]);
            }
        }
        return result;
    };
    const auto x = box(a), y = box(b);
    for (unsigned k = 0; k < 3; ++k) {
        if (x[1][k] < y[0][k] - .001f || y[1][k] < x[0][k] - .001f) {
            return false;
        }
    }
    return true;
}
}  // namespace

std::vector<CoverageProbeGrid> coverage_probe_grids(
    const ProbeGrid& prototype,
    std::span<const EnvironmentFloorSample> floors,
    const RoomLayout& layout,
    Float3 spacing,
    unsigned maximum_probes,
    unsigned maximum_grids
) {
    if (maximum_probes < 64 || maximum_grids == 0 || maximum_grids > 1024
        || floors.size() != layout.owners.size() || floors.size() != layout.height_bounds.size()) {
        throw std::runtime_error("invalid floor coverage allocation limits");
    }
    for (auto step : spacing) {
        if (!std::isfinite(step) || step <= 0) {
            throw std::runtime_error("invalid GI spacing");
        }
    }
    std::map<unsigned, std::vector<Column>> domains;
    for (unsigned i = 0; i < floors.size(); ++i) {
        auto low = floors[i].position, high = low;
        low[2] = std::max(layout.height_bounds[i][0], low[2] + .15f);
        high[2] = layout.height_bounds[i][1];
        for (unsigned a = 0; a < 3; ++a) {
            if (!std::isfinite(low[a]) || !std::isfinite(high[a])) {
                throw std::runtime_error("invalid GI floor height envelope");
            }
        }
        if (low[2] > high[2]) {
            throw std::runtime_error("invalid GI floor height envelope");
        }
        domains[layout.owners[i]].push_back({low, high, layout.owners[i]});
    }
    std::vector<Allocation> allocations;
    const auto allocate =
        [&](auto&& self, std::vector<Column> columns, unsigned depth, ProbeFrame parent) -> void {
        if (depth > 32 || allocations.size() >= maximum_grids * 4) {
            throw std::runtime_error("GI coverage exceeds tile budget");
        }
        const auto grid = fit(prototype, columns, spacing, std::span(&parent, 1));
        if (grid.probe_count() <= maximum_probes) {
            grid.validate();
            allocations.push_back({grid, std::move(columns)});
            return;
        }
        unsigned axis = 0;
        for (unsigned a = 1; a < 3; ++a) {
            if (grid.dimensions[a] > grid.dimensions[axis]) {
                axis = a;
            }
        }
        float fraction = .5f;
        std::size_t cross_section = 1;
        for (unsigned a = 0; a < 3; ++a) {
            if (a != axis) {
                cross_section *= grid.dimensions[a];
            }
        }
        const auto capacity = maximum_probes / cross_section;
        if (capacity > 3 && grid.dimensions[axis] > capacity) {
            const auto parts = (grid.dimensions[axis] - 3 + capacity - 4) / (capacity - 3);
            fraction = float(parts / 2) / float(parts);
        }
        // Balance the estimated number of budget-sized leaves, not just the
        // distance: a five-box corridor should split into two plus three.
        const float plane = grid.frame.to_local(grid.origin)[axis] + spacing[axis]
                            + float(grid.dimensions[axis] - 3) * spacing[axis] * fraction;
        std::vector<Column> left, right;
        for (const auto& column : columns) {
            const float a = grid.frame.to_local(column.low)[axis],
                        b = grid.frame.to_local(column.high)[axis];
            if (std::max(a, b) <= plane) {
                left.push_back(column);
            } else if (std::min(a, b) >= plane) {
                right.push_back(column);
            } else {
                const float t = (plane - a) / (b - a);
                const auto cut = add(column.low, mul(add(column.high, mul(column.low, -1)), t));
                const Column first{column.low, cut, column.island},
                    second{cut, column.high, column.island};
                if (a < b) {
                    left.push_back(first);
                    right.push_back(second);
                } else {
                    right.push_back(first);
                    left.push_back(second);
                }
            }
        }
        if (left.empty() || right.empty()) {
            throw std::runtime_error("cannot split GI receiver envelope within probe budget");
        }
        self(self, std::move(left), depth + 1, grid.frame);
        self(self, std::move(right), depth + 1, grid.frame);
    };
    for (auto& [island, columns] : domains) {
        allocate(allocate, std::move(columns), 0, {});
    }
    // Share storage only when nearby allocations become cheaper and no larger
    // in combined padded volume. Their island identities remain independent.
    for (unsigned i = 0; i < allocations.size(); ++i) {
        for (unsigned j = i + 1; j < allocations.size();) {
            auto& a = allocations[i];
            const auto& b = allocations[j];
            if (!touches(a.grid, b.grid)) {
                ++j;
                continue;
            }
            std::vector<Column> combined = a.columns;
            combined.insert(combined.end(), b.columns.begin(), b.columns.end());
            const auto count = a.grid.probe_count() + b.grid.probe_count();
            const double old_volume = volume(a.grid) + volume(b.grid);
            const std::array<ProbeFrame, 2> seeds{a.grid.frame, b.grid.frame};
            bool promising = false;
            for (auto frame : seeds) {
                const auto trial = bounds(prototype, combined, frame, spacing);
                promising |= trial.probe_count() < count && trial.probe_count() <= maximum_probes
                             && volume(trial) <= old_volume;
            }
            if (!promising) {
                ++j;
                continue;
            }
            const auto grid = fit(prototype, combined, spacing, seeds, maximum_probes, old_volume);
            if (grid.probe_count() < count && grid.probe_count() <= maximum_probes
                && volume(grid) <= old_volume) {
                a = {grid, std::move(combined)};
                allocations.erase(allocations.begin() + j);
            } else {
                ++j;
            }
        }
    }
    if (allocations.size() > maximum_grids) {
        throw std::runtime_error("GI coverage exceeds tile budget");
    }
    std::vector<CoverageProbeGrid> result;
    for (auto& allocation : allocations) {
        std::set<unsigned> islands;
        for (const auto& column : allocation.columns) {
            islands.insert(column.island);
        }
        result.push_back({{islands.begin(), islands.end()}, allocation.grid});
    }
    return result;
}
}  // namespace openrac::lighting::bake_detail
