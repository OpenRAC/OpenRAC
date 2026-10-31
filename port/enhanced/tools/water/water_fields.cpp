// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Stable shore distances, absorbing outlets and floor-derived hydraulic fields for water_internal.h.
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>

#include "common/profiler.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
namespace {
// Stable priority ordering, with linear cell index breaking equal distances.
using Queue = std::priority_queue<
    std::pair<double, std::size_t>,
    std::vector<std::pair<double, std::size_t>>,
    std::greater<>>;

// Relaxes one popped frontier cell without changing neighbour or equal-distance ordering.
void propagate(
    const Grid& grid, std::vector<double>& field, Queue& queue, std::size_t at, double distance
) {
    const int x = static_cast<int>(at % grid.width);
    const int y = static_cast<int>(at / grid.width);

    // Neighbour traversal preserves the reference row order.
    for (int dy = -1; dy <= 1; ++dy) {
        // Each surrounding cell is considered in reference column order.
        for (int dx = -1; dx <= 1; ++dx) {
            const int nx = x + dx;
            const int ny = y + dy;

            // Only actual in-bounds neighbouring cells participate in propagation.
            if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= static_cast<int>(grid.width)
                || ny >= static_cast<int>(grid.height)) {
                continue;
            }

            const auto target =
                static_cast<std::size_t>(ny) * grid.width + static_cast<unsigned>(nx);
            const double value = distance + grid.cell * std::hypot(dx, dy);

            // Strict improvements retain stable equal-distance ties.
            if (value < field[target]) {
                field[target] = value;
                queue.emplace(value, target);
            }
        }
    }
}
}  // namespace

std::vector<double> distances(std::span<const std::uint8_t> wet, const Grid& grid) {
    const profiling::Zone timing("Water shore distances");
    std::vector<double> field(wet.size(), INFINITY);
    Queue queue;

    // Every dry cell is a zero-distance seed, including solid banks and the padded outer border.
    for (std::size_t at = 0; at < wet.size(); ++at) {
        // Only dry cells initialise the shore frontier.
        if (!wet[at]) {
            field[at] = 0;
            queue.emplace(0, at);
        }
    }

    // Distance then linear-index ties match Python's heap ordering.
    while (!queue.empty()) {
        const auto [distance, at] = queue.top();
        queue.pop();

        // A shorter route may have superseded this queued candidate.
        if (distance != field[at]) {
            continue;
        }

        propagate(grid, field, queue, at, distance);
    }

    return field;
}

namespace {
// Preserves the reference's three-component accumulation order for outlet distances.
double dot(Point first, Point second) {
    return (first[0] * second[0] + first[1] * second[1]) + first[2] * second[2];
}

// Finds first-use extrema without allowing equivalent art layers to change the chosen endpoints.
std::pair<Point, Point> extrema(const Surface& surface) {
    Point highest = surface.triangles.front()[0];
    Point lowest = highest;

    // Strict comparisons retain the first point at tied heights.
    for (const auto& triangle : surface.triangles) {
        // Point order resolves equal-height extrema deterministically.
        for (const auto& p : triangle) {
            // Strict comparison preserves the first highest authored endpoint.
            if (p[2] > highest[2]) {
                highest = p;
            }

            // Strict comparison preserves the first lowest authored endpoint.
            if (p[2] < lowest[2]) {
                lowest = p;
            }
        }
    }

    return {highest, lowest};
}

// Returns the authored direction or the inferred high-to-low flow for sloped strips.
std::array<double, 2> tangent(const Surface& surface) {
    Point flow = surface.flow;

    // Only flowing surfaces infer a downhill or endpoint direction.
    if (surface.flowing) {
        auto [highest, lowest] = extrema(surface);

        // Flat strips use the original last-to-first endpoint direction.
        if (highest[2] - lowest[2] <= .5) {
            highest = surface.triangles.front()[0];
            lowest = surface.triangles.back()[2];
        }

        // The inferred flow remains a full world-space vector.
        for (unsigned axis = 0; axis < 3; ++axis) {
            flow[axis] = lowest[axis] - highest[axis];
        }
    }

    const double length = std::hypot(flow[0], flow[1]);

    // Near-zero horizontal flow must not amplify into an arbitrary current.
    if (length <= .01) {
        return {};
    }

    return {flow[0] / length, flow[1] / length};
}

// Explicit outlets absorb according to full world-space segment distance.
double outlet_absorption(Point p, std::span<const Outlet> outlets) {
    double absorb = 0;

    // Each authored outlet can independently increase absorption.
    for (const auto& edge : outlets) {
        Point ab{};
        Point offset{};
        Point distance{};

        // All three coordinates contribute to the segment projection.
        for (unsigned axis = 0; axis < 3; ++axis) {
            ab[axis] = edge.second[axis] - edge.first[axis];
            offset[axis] = p[axis] - edge.first[axis];
        }

        const double t = std::clamp(dot(offset, ab) / std::max(dot(ab, ab), 1e-8), 0., 1.);

        // Full spatial distance prevents another storey from sharing this outlet.
        for (unsigned axis = 0; axis < 3; ++axis) {
            distance[axis] = p[axis] - edge.first[axis] - t * ab[axis];
        }

        absorb =
            std::max(absorb, std::max(0., 1 - std::sqrt(dot(distance, distance)) / edge.width));
    }

    return absorb;
}

// Writes one final cell without using invalid heights outside the wet mask.
Texel cell(
    const Raster& raster,
    const Surface& surface,
    const Input& input,
    const std::array<double, 2>& shore,
    std::size_t at
) {
    const auto& grid = raster.grid;
    const unsigned x = static_cast<unsigned>(at % grid.width);
    const unsigned y = static_cast<unsigned>(at / grid.width);
    Point p{grid.origin[0] + (x + .5) * grid.cell, grid.origin[1] + (y + .5) * grid.cell, 0};
    double absorb = 0;
    double encoded = 0;

    // Dry covered art retains its negated rest height; absent art uses zero.
    if (std::isfinite(raster.heights[at])) {
        encoded = -(512 + raster.heights[at]);
    }

    // Only wet receivers have an actual surface height.
    if (raster.wet[at]) {
        p[2] = raster.heights[at];
        encoded = 512 + raster.heights[at];

        // Unobstructed flowing ends absorb within two units, unlike solid banks.
        if (surface.flowing && shore[0] < 2 && shore[1] > 2) {
            absorb = std::max(0., 1 - shore[0] / 2);
        }
    }

    absorb = std::max(absorb, outlet_absorption(p, input.outlets));
    return {encoded, shore[0], absorb, 0};
}

// Stores the highest collision floor sufficiently below a wet receiver's rest height.
void floor_sample(
    const Body& body, const Triangle& triangle, std::vector<double>& floors, std::size_t at, Point p
) {
    // Dry receivers do not query a hydraulic floor.
    if (body.cells[at][0] < .5) {
        return;
    }

    const auto height = height_at(p, triangle);

    // The highest sufficiently submerged floor determines optical depth.
    if (height && *height < body.cells[at][0] - 512 - .1 && *height > floors[at]) {
        floors[at] = *height;
    }
}
}  // namespace

Body bake_body(const Surface& surface, const Input& input) {
    const profiling::Zone timing("Bake water body");
    auto raster = rasterise(surface, input);
    const auto shore = distances(raster.wet, raster.grid);
    std::vector<double> bank(raster.wet.size(), INFINITY);

    // Without solid banks, bank distance remains infinite everywhere.
    if (std::any_of(raster.blocked.begin(), raster.blocked.end(), [](auto b) { return b != 0; })) {
        auto mask = raster.blocked;

        // Bank cells become the dry seeds of this second distance field.
        for (auto& value : mask) {
            value = !value;
        }

        bank = distances(mask, raster.grid);
    }

    Body body;
    const auto direction = tangent(surface);
    const auto& origin = raster.grid.origin;
    body.metadata =
        {Texel{origin[0], origin[1], origin[2], static_cast<double>(surface.source)},
         Texel{1, 0, 0, static_cast<double>(raster.grid.width)},
         Texel{0, 1, 0, static_cast<double>(raster.grid.height)},
         Texel{0, 0, 1, 0},
         Texel{direction[0], direction[1], input.cell, 0}};
    body.cells.reserve(raster.wet.size());

    // Cell order determines the binary atlas and body offsets.
    for (std::size_t at = 0; at < raster.wet.size(); ++at) {
        body.cells.push_back(cell(raster, surface, input, {shore[at], bank[at]}, at));
    }

    hydraulics(body, surface, input);
    return body;
}

void hydraulics(Body& body, const Surface& surface, const Input& input) {
    const profiling::Zone timing("Water hydraulics");
    const auto& meta = body.metadata;
    const Grid grid{
        {meta[0][0], meta[0][1], meta[0][2]},
        static_cast<unsigned>(meta[1][3]),
        static_cast<unsigned>(meta[2][3]),
        input.cell
    };
    std::vector<double> floors(body.cells.size(), -INFINITY);

    // Existing projected collision faces supply optical depth, independently of solver clamps.
    for (const auto& triangle : input.solids) {
        visit(grid, triangle, {-.5, -.5}, [&](std::size_t at, Point p) {
            floor_sample(body, triangle, floors, at, p);
        });
    }

    const double fallback = surface.flowing ? .45 : .9;
    const double x = surface.pond ? 0 : meta[4][0] * .8;
    const double y = surface.pond ? 0 : meta[4][1] * .8;
    body.hydraulics.reserve(body.cells.size());

    // Solver depth clamps independently of the stored optical depth.
    for (std::size_t at = 0; at < body.cells.size(); ++at) {
        const double optical = std::isfinite(floors[at])
                                   ? std::max(0., body.cells[at][0] - 512 - floors[at])
                                   : fallback;
        body.hydraulics.push_back({std::clamp(optical, .25, 1.5), x, y, optical});
    }
}
}  // namespace openrac::lighting::water
