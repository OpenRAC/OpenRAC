// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned surface, raster and binary-output records shared by the native water stages.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <ostream>
#include <span>
#include <vector>

namespace openrac::lighting::water {
using Point = std::array<double, 3>;
using Triangle = std::array<Point, 3>;
using Texel = std::array<double, 4>;
// Limits match the existing runtime water atlas and per-body allocation budgets.
inline constexpr std::size_t kAtlasCells = 4ull * 1024 * 1024;
inline constexpr std::size_t kBodyCells = 1024ull * 1024;

// One authored outlet segment, applied without changing reflective solid banks.
struct Outlet {
    Point first{};     // World-space first endpoint.
    Point second{};    // World-space second endpoint.
    double width = 2;  // Absorption radius in world units.
};

// One physical water body, retaining original triangle and art-layer order.
struct Surface {
    unsigned source = 0;   // Authored runtime surface identifier.
    bool pond = false;     // Restrict to collision water support and keep the current still.
    bool flowing = false;  // Infer the current and absorb unobstructed ends.
    Point flow{};          // Authored initial flow vector.
    std::vector<Triangle> triangles;  // World-space water surface triangles.
};

// All decoded inputs are owned by one controller until output is complete.
struct Input {
    std::vector<Surface> surfaces;         // Stable physical-body order.
    std::vector<Triangle> solids;          // Two-sided collision, independent of mesh normals.
    std::vector<Triangle> water;           // Authored collision waterline support.
    std::vector<Outlet> outlets;           // Explicit absorbing boundaries.
    std::array<double, 2> pool_spacing{};  // Rest-grid subdivision dimensions.
    std::array<std::uint8_t, 32> fingerprint{};  // Shared source identity supplied by preparation.
    double cell = .5;                            // World-space field and mesh subdivision size.
};

// One bounded XY grid; pixel centres lie half a cell from its origin.
struct Grid {
    Point origin{};       // World or surface-relative origin, depending on the visiting stage.
    unsigned width = 0;   // Columns.
    unsigned height = 0;  // Rows.
    double cell = .5;     // World-space cell size.
};

// Intermediate occupancy shared by raster, bank and hydraulic passes for one body.
struct Raster {
    Grid grid;                          // World-space grid.
    Point anchor{};                     // First surface XY vertex, with Z zero.
    Point local_origin{};               // Exact snapped surface-relative origin.
    std::vector<std::uint8_t> wet;      // Supported cells, then clipped water mask.
    std::vector<std::uint8_t> blocked;  // Solid banks, separate from open strip ends.
    std::vector<double> heights;  // Highest rest height, or negative infinity without art coverage.
};

// Final per-body doubles are narrowed only when writing the existing float32 binary format.
struct Body {
    std::array<Texel, 5> metadata{};  // Existing origin/axes/current layout.
    std::vector<Texel> cells;         // Rest heights, shore distance and absorption.
    std::vector<Texel> hydraulics;    // Solver depth, current and optical depth.
};

// Reads finite, bounded decoded surface inputs and collision geometry.
Input read_input(const std::filesystem::path& path);
// Returns a signed XY edge test; face winding is irrelevant.
bool contains(Point p, const Triangle& triangle);
// Projects onto a nonvertical triangle, returning no height outside its footprint.
std::optional<double> height_at(Point point, const Triangle& triangle);
// Visits a conservative pixel bounding rectangle; the synchronous callback borrows the current triangle.
void visit(
    const Grid& grid,
    const Triangle& triangle,
    std::array<double, 2> padding,
    const std::function<void(std::size_t, Point)>& sample
);
// Rasterises art, constrains pond support and cuts solid banks.
Raster rasterise(const Surface& surface, const Input& input);
// Computes eight-neighbour shore distances in stable distance/index order.
std::vector<double> distances(std::span<const std::uint8_t> wet, const Grid& grid);
// Builds runtime cells and metadata before hydraulic floor queries.
Body bake_body(const Surface& surface, const Input& input);
// Appends floor-derived hydraulic depths and currents to one finished body.
void hydraulics(Body& body, const Surface& surface, const Input& input);
// Writes field and rest-mesh records in the unchanged little-endian formats.
void write_fields(
    const std::filesystem::path& path, std::span<const Body> bodies, const Input& input
);
// Writes original art layers and shared rest-mesh subdivision templates.
void write_mesh(const std::filesystem::path& path, const Input& input);
// Writes an unsigned little-endian word to an open output stream.
void write_word(std::ostream& output, std::uint32_t value);
// Narrows and writes one float32 without aliasing host memory.
void write_float(std::ostream& output, double value);
}  // namespace openrac::lighting::water
