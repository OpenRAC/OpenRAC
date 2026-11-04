// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Two-sided XY waterline rasterisation and solid-bank clipping for water_internal.h.
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
bool contains(Point p, const Triangle& triangle) {
    std::array<double, 3> edges{};

    // Signed projected edges accept either triangle winding and the reference edge tolerance.
    for (unsigned index = 0; index < 3; ++index) {
        const auto& a = triangle[index];
        const auto& b = triangle[(index + 1) % 3];
        edges[index] = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
    }

    return *std::min_element(edges.begin(), edges.end()) >= -1e-7
           || *std::max_element(edges.begin(), edges.end()) <= 1e-7;
}

std::optional<double> height_at(Point p, const Triangle& triangle) {
    const auto& [a, b, c] = triangle;
    const double determinant = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);

    // Vertical or uncovered faces cannot provide a water or floor height.
    if (std::abs(determinant) < 1e-8 || !contains(p, triangle)) {
        return {};
    }

    const double wa = ((b[1] - c[1]) * (p[0] - c[0]) + (c[0] - b[0]) * (p[1] - c[1])) / determinant;
    const double wb = ((c[1] - a[1]) * (p[0] - c[0]) + (a[0] - c[0]) * (p[1] - c[1])) / determinant;
    return wa * a[2] + wb * b[2] + (1 - wa - wb) * c[2];
}

void visit(
    const Grid& grid,
    const Triangle& triangle,
    std::array<double, 2> padding,
    const std::function<void(std::size_t, Point)>& sample
) {
    std::array<int, 2> low{};
    std::array<int, 2> high{};
    const std::array dimensions{grid.width, grid.height};

    // Clamp floating extents before conversion, including far-away collision triangles.
    for (unsigned axis = 0; axis < 2; ++axis) {
        const double first = std::min({triangle[0][axis], triangle[1][axis], triangle[2][axis]});
        const double last = std::max({triangle[0][axis], triangle[1][axis], triangle[2][axis]});
        low[axis] = static_cast<int>(std::clamp(
            std::floor((first - grid.origin[axis]) / grid.cell + padding[0]),
            0.,
            static_cast<double>(dimensions[axis])
        ));
        high[axis] = static_cast<int>(std::clamp(
            std::ceil((last - grid.origin[axis]) / grid.cell + padding[1]),
            -1.,
            static_cast<double>(dimensions[axis]) - 1
        ));
    }

    // Row-major visits retain reference accumulation and highest-surface selection order.
    for (int y = low[1]; y <= high[1]; ++y) {
        // Column order retains the reference per-triangle accumulation order.
        for (int x = low[0]; x <= high[0]; ++x) {
            sample(
                static_cast<std::size_t>(y) * grid.width + static_cast<unsigned>(x),
                {grid.origin[0] + (x + .5) * grid.cell, grid.origin[1] + (y + .5) * grid.cell, 0}
            );
        }
    }
}

namespace {
// Allocates the same padded surface-relative lattice before any height queries.
Raster allocate(const Surface& surface, double cell) {
    Raster raster;
    raster.anchor = {surface.triangles.front()[0][0], surface.triangles.front()[0][1], 0};
    raster.grid.cell = cell;
    std::array<unsigned, 2> dimensions{};

    // One dry border cell surrounds the complete authored footprint.
    for (unsigned axis = 0; axis < 2; ++axis) {
        double low = INFINITY;
        double high = -INFINITY;

        // Every art layer contributes to the complete footprint bounds.
        for (const auto& triangle : surface.triangles) {
            // Bounds include all authored triangle corners.
            for (const auto& p : triangle) {
                low = std::min(low, p[axis] - raster.anchor[axis]);
                high = std::max(high, p[axis] - raster.anchor[axis]);
            }
        }

        low = std::floor(low / cell) * cell - cell;
        high = std::ceil(high / cell) * cell + cell;
        const double extent = std::round((high - low) / cell);

        // Reject excessive extents before narrowing or allocating.
        if (extent < 1 || extent > kBodyCells) {
            throw std::runtime_error("water grid exceeds cell budget");
        }

        dimensions[axis] = static_cast<unsigned>(extent);
        raster.local_origin[axis] = low;
        raster.grid.origin[axis] = raster.anchor[axis] + low;
    }

    const std::size_t count = std::size_t{dimensions[0]} * dimensions[1];

    // The product must fit the per-body raster budget.
    if (count > kBodyCells) {
        throw std::runtime_error("water grid exceeds cell budget");
    }

    raster.grid.width = dimensions[0];
    raster.grid.height = dimensions[1];
    raster.wet.resize(count);
    raster.blocked.resize(count);
    raster.heights.assign(count, -INFINITY);
    return raster;
}

// Updates the highest art layer at a pixel without changing authored traversal order.
void art_sample(Raster& raster, const Triangle& triangle, std::size_t at, Point point) {
    const auto height = height_at(point, triangle);

    // Only covered, nonvertical triangles contribute rest heights.
    if (height) {
        raster.wet[at] = 1;
        raster.heights[at] = std::max(raster.heights[at], *height);
    }
}

// Accepts collision water close to the actual surface, excluding stacked storeys.
void support_sample(
    const Raster& raster,
    const Triangle& triangle,
    std::vector<std::uint8_t>& supported,
    std::size_t at,
    Point point
) {
    // Unsupported art cells need no collision-water query.
    if (!raster.wet[at]) {
        return;
    }

    const auto height = height_at(point, triangle);

    // Separate storeys must not share waterline support.
    if (height && std::abs(*height - raster.heights[at]) < 1) {
        supported[at] = 1;
    }
}

// Cuts unsupported pond patches using the exported collision waterline.
void constrain(Raster& raster, const Input& input) {
    std::vector<std::uint8_t> supported(raster.wet.size());

    // Each query is synchronous and borrows the current collision triangle.
    for (const auto& triangle : input.water) {
        visit(raster.grid, triangle, {0, 0}, [&](std::size_t at, Point p) {
            support_sample(raster, triangle, supported, at, p);
        });
    }

    // Unmatched pond collision indicates incompatible or incomplete input.
    if (std::none_of(supported.begin(), supported.end(), [](auto value) { return value != 0; })) {
        throw std::runtime_error("pond waterline does not match collision export");
    }

    // Shared art seams do not create banks; only actual collision support controls occupancy.
    for (std::size_t at = 0; at < supported.size(); ++at) {
        raster.wet[at] = raster.wet[at] && supported[at];
    }
}

// Covers a projected vertical wall segment within slightly over half a cell.
bool near_wall(Point p, const Triangle& triangle, double cell) {
    // Three segments cover every possible vertical-face projection.
    for (unsigned edge = 0; edge < 3; ++edge) {
        const auto& q = triangle[edge];
        const auto& r = triangle[(edge + 1) % 3];
        const double x = r[0] - q[0];
        const double y = r[1] - q[1];
        const double t = std::clamp(
            ((p[0] - q[0]) * x + (p[1] - q[1]) * y) / std::max(x * x + y * y, 1e-8), 0., 1.
        );

        // A slightly wider half-cell band conservatively covers vertical banks.
        if (std::hypot(p[0] - q[0] - t * x, p[1] - q[1] - t * y) < cell * .55) {
            return true;
        }
    }

    return false;
}

// Separates solid waterline banks from bottom surfaces and overhead bridges.
void bank_sample(Raster& raster, const Triangle& triangle, std::size_t at, Point p) {
    // Only currently supported water cells can be blocked.
    if (!raster.wet[at]) {
        return;
    }

    const auto& [a, b, c] = triangle;
    const double determinant = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);

    // The reference treats faces with projected area above this threshold as height surfaces.
    if (std::abs(determinant) > 1e-8) {
        const auto height = height_at(p, triangle);

        // Height surfaces block only at the actual waterline.
        if (height && raster.heights[at] - .1 <= *height && *height <= raster.heights[at] + .5) {
            raster.blocked[at] = 1;
        }

        return;
    }

    const double low = std::min({a[2], b[2], c[2]});
    const double high = std::max({a[2], b[2], c[2]});

    // Vertical faces block only where their actual height crosses the waterline.
    if (low - .1 <= raster.heights[at] && raster.heights[at] <= high + .1
        && near_wall(p, triangle, raster.grid.cell)) {
        raster.blocked[at] = 1;
    }
}

// Restricts collision visits to the body's finite authored water-height band.
void banks(Raster& raster, const Input& input) {
    double low = INFINITY;
    double high = -INFINITY;

    // Only finite art heights contribute to vertical collision pruning.
    for (double height : raster.heights) {
        // Absent art must not widen the water-height band.
        if (std::isfinite(height)) {
            low = std::min(low, height);
            high = std::max(high, height);
        }
    }

    // A degenerate surface cannot produce a valid water field.
    if (!std::isfinite(low)) {
        throw std::runtime_error("water surface has no rasterised cells");
    }

    // Synchronous callbacks keep all state local to the current body.
    for (const auto& triangle : input.solids) {
        const double minimum = std::min({triangle[0][2], triangle[1][2], triangle[2][2]});
        const double maximum = std::max({triangle[0][2], triangle[1][2], triangle[2][2]});

        // Floors and overhead bridges outside the waterline band cannot block it.
        if (maximum < low - .1 || minimum > high + .5) {
            continue;
        }

        visit(raster.grid, triangle, {-.5, .5}, [&](std::size_t at, Point p) {
            bank_sample(raster, triangle, at, p);
        });
    }

    // Solid banks become dry cells before distance propagation.
    for (std::size_t at = 0; at < raster.wet.size(); ++at) {
        // Unblocked cells retain their supported occupancy.
        if (raster.blocked[at]) {
            raster.wet[at] = 0;
        }
    }
}
}  // namespace

Raster rasterise(const Surface& surface, const Input& input) {
    const profiling::Zone timing("Rasterise waterline");
    auto raster = allocate(surface, input.cell);
    Grid local = raster.grid;
    local.origin = raster.local_origin;

    // Relative arithmetic reproduces the original surface projection before world-space clipping.
    for (auto triangle : surface.triangles) {
        // Surface-relative arithmetic preserves the existing projection precision.
        for (auto& p : triangle) {
            p[0] -= raster.anchor[0];
            p[1] -= raster.anchor[1];
        }

        visit(local, triangle, {0, 0}, [&](std::size_t at, Point p) {
            art_sample(raster, triangle, at, p);
        });
    }

    // Only pond surfaces use authored collision-water support.
    if (surface.pond && !input.water.empty()) {
        constrain(raster, input);
    }

    banks(raster, input);
    return raster;
}
}  // namespace openrac::lighting::water
