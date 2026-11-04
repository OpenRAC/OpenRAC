// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Extracts surveyed coastal inputs for the Pokitaru wave-response prototype.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "assets/bytes.h"
#include "assets/version.h"
#include "assets/world/gameplay.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/water/water_bake.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
namespace {
// Reads the ocean object's authored rest height, independent of its placement and moving pools.
double ocean_height(const std::filesystem::path& level) {
    const auto manifest = bake_json::document(level / "manifest.json");
    // The reviewed pvar layout applies to the NTSC-U Pokitaru export.
    if (manifest["game"].string() != "rac1" || manifest["version"].string() != "ntsc"
        || manifest["level"].number(-1) != 11) {
        throw std::runtime_error("expected an NTSC-U Pokitaru export");
    }
    const auto placements = bake_json::document(level / "placements.json");
    std::optional<int> index;
    for (const auto& moby : placements["mobys"].items()) {
        // Class 1111 draws the outdoor ocean independently of the level's water grids.
        if (moby["class"].number(-1) != 1111) {
            continue;
        }
        const double value = moby["fields"]["pvar_index"].number(-1);
        // A unique, valid pvar index is required before opening the gameplay block.
        if (index || !std::isfinite(value) || value < 0 || value > std::numeric_limits<int>::max()
            || std::trunc(value) != value) {
            throw std::runtime_error("invalid Pokitaru ocean placement");
        }
        index = static_cast<int>(value);
    }
    // An export missing the ocean cannot supply its mean waterline.
    if (!index) {
        throw std::runtime_error("missing Pokitaru ocean placement");
    }
    const auto bytes = bake_json::source_bytes(level / "bake-inputs/gameplay.bin");
    const assets::GameplayFile gameplay(assets::Game::Rac1, bytes);
    const auto block = assets::read_pvar_block(gameplay, *index);
    constexpr std::size_t kLevelOffset = 0x14;  // OceanVars::level in the original update.
    // Read only the reviewed float within a complete pvar block.
    if (!block || block->size() < kLevelOffset + sizeof(float)) {
        throw std::runtime_error("incomplete Pokitaru ocean pvar block");
    }
    const double height = assets::ByteView(*block).f32_at(kLevelOffset);
    // Invalid rest heights cannot participate in the collision raster.
    if (!std::isfinite(height)) {
        throw std::runtime_error("invalid Pokitaru ocean height");
    }
    return height;
}

// Rasterizes the intersection with the water plane, including sloped and vertical walls.
void mark_waterline(Raster& raster, const Triangle& triangle, double height) {
    std::array<Point, 3> crossings{};
    unsigned count = 0;
    for (unsigned i = 0; i < triangle.size(); ++i) {
        const auto& a = triangle[i];
        const auto& b = triangle[(i + 1) % triangle.size()];
        // A half-open edge test keeps shared vertices consistent and skips coplanar faces.
        if ((a[2] < height) != (b[2] < height)) {
            const double t = (height - a[2]) / (b[2] - a[2]);
            crossings[count++] = {std::lerp(a[0], b[0], t), std::lerp(a[1], b[1], t), height};
        }
    }
    // Only a triangle crossing the plane contributes a contour segment.
    if (count != 2) {
        return;
    }
    const auto& a = crossings[0];
    const auto& b = crossings[1];
    const double dx = b[0] - a[0], dy = b[1] - a[1];
    const double length2 = dx * dx + dy * dy;
    // A cell's circumscribed circle conservatively covers diagonal and corner crossings.
    const double radius2 = .5 * raster.grid.cell * raster.grid.cell;
    visit(raster.grid, {a, b, b}, {-1.5, .5}, [&](std::size_t index, Point point) {
        const double x = point[0] - a[0], y = point[1] - a[1];
        const double t = length2 > 0 ? std::clamp((x * dx + y * dy) / length2, 0., 1.) : 0.;
        const double rx = x - t * dx, ry = y - t * dy;
        // Include every cell touched by the contour, even if its centre misses the face.
        if (rx * rx + ry * ry <= radius2) {
            raster.blocked[index] = 1;
        }
    });
}

// Fill closed contours, keeping only water that connects to the patch's open perimeter.
void fill_obstacles(Raster& raster) {
    const auto width = raster.grid.width, height = raster.grid.height;
    std::vector<std::uint8_t> connected(raster.wet.size());
    std::vector<std::size_t> queue;
    queue.reserve(connected.size());
    const auto add = [&](std::size_t index) {
        // The wave stencil uses face neighbours, so corner contact cannot leak through a bank.
        if (!connected[index] && !raster.blocked[index]) {
            connected[index] = 1;
            queue.push_back(index);
        }
    };
    for (unsigned x = 0; x < width; ++x) {
        add(x);
        add(std::size_t(height - 1) * width + x);
    }
    for (unsigned y = 0; y < height; ++y) {
        add(std::size_t(y) * width);
        add(std::size_t(y) * width + width - 1);
    }
    for (std::size_t i = 0; i < queue.size(); ++i) {
        const auto index = queue[i];
        const auto x = index % width, y = index / width;
        // Bounds keep traversal on the current grid row or column.
        if (x > 0) {
            add(index - 1);
        }
        if (x + 1 < width) {
            add(index + 1);
        }
        if (y > 0) {
            add(index - width);
        }
        if (y + 1 < height) {
            add(index + width);
        }
    }
    for (std::size_t i = 0; i < connected.size(); ++i) {
        raster.wet[i] = connected[i];
        raster.blocked[i] = !connected[i];
    }
}

// Surveys the highest submerged collision floor per cell and leaves unobserved depth unknown.
std::vector<double> depths(const Raster& raster, const Input& input, double height) {
    std::vector<double> floors(raster.wet.size(), -std::numeric_limits<double>::infinity());
    for (const auto& triangle : input.solids) {
        visit(raster.grid, triangle, {-.5, -.5}, [&](std::size_t index, Point point) {
            // Land cells do not have a water column.
            if (!raster.wet[index]) {
                return;
            }
            const auto floor = height_at(point, triangle);
            // Use the same surface clearance as the water hydraulics survey.
            if (floor && *floor < height - .1) {
                floors[index] = std::max(floors[index], *floor);
            }
        });
    }
    for (auto& floor : floors) {
        floor = height - floor;
    }
    return floors;
}

// Serializes row-major banks and depths. Null records mean no surveyed water column.
std::string patch_json(const Raster& raster, const std::vector<double>& depth, double height) {
    const auto& grid = raster.grid;
    std::ostringstream out;
    out << std::setprecision(17) << "{\"origin\":[" << grid.origin[0] << ',' << grid.origin[1]
        << "],\"size\":" << grid.width << ",\"cell\":" << grid.cell << ",\"mean_height\":" << height
        << ",\"bank\":[";
    for (std::size_t index = 0; index < depth.size(); ++index) {
        out << (index ? "," : "") << (raster.blocked[index] ? "true" : "false");
    }
    out << "],\"depth\":[";
    for (std::size_t index = 0; index < depth.size(); ++index) {
        out << (index ? "," : "");
        // JSON null preserves gaps without inventing a seabed outside surveyed collision.
        if (std::isfinite(depth[index])) {
            out << depth[index];
        } else {
            out << "null";
        }
    }
    out << "]}";
    return out.str();
}
}  // namespace

Result<std::string> pokitaru_coast(
    const std::filesystem::path& level, std::array<double, 2> origin, unsigned size, double cell
) {
    try {
        // Bound the patch to the prototype's allocation budget.
        if (size == 0 || size > 256 || !std::isfinite(cell) || cell <= 0
            || !std::isfinite(origin[0]) || !std::isfinite(origin[1])) {
            throw std::runtime_error("invalid Pokitaru coastal grid");
        }
        const auto height = ocean_height(level);
        Input input;
        load_collision(input, level / "collision.glb");
        Raster raster;
        raster.grid = {{origin[0], origin[1], 0}, size, size, cell};
        const auto count = std::size_t(size) * size;
        raster.wet.assign(count, 1);
        raster.blocked.resize(count);
        raster.heights.assign(count, height);
        clip_solid_banks(raster, input);
        for (const auto& triangle : input.solids) {
            mark_waterline(raster, triangle, height);
        }
        fill_obstacles(raster);
        return patch_json(raster, depths(raster, input, height), height);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting::water
