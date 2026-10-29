// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Gameplay surfaces derived from authored swimmable collision, independently of water rendering.
#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting {
namespace {
using Key = std::array<int, 3>;
constexpr unsigned kMaximumWaterCells =
    32768;  // Bound planning memory before creating either medium.

// Winding-independent vertical intersection with a nondegenerate collision triangle.
std::optional<float> height(const GameplayTriangle& t, Float2 p) {
    const auto& a = t.vertices[0];
    const auto& b = t.vertices[1];
    const auto& c = t.vertices[2];
    const float determinant = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);

    if (std::abs(determinant) < .0001f) {
        return {};
    }

    const float u = ((b[1] - c[1]) * (p[0] - c[0]) + (c[0] - b[0]) * (p[1] - c[1])) / determinant;
    const float v = ((c[1] - a[1]) * (p[0] - c[0]) + (a[0] - c[0]) * (p[1] - c[1])) / determinant;

    if (u < -.0001f || v < -.0001f || u + v > 1.0001f) {
        return {};
    }

    return u * a[2] + v * b[2] + (1 - u - v) * c[2];
}

// Only nearly horizontal authored collision faces define a swimming plane; waterfalls do not.
std::map<Key, Float3> water_cells(std::span<const GameplayTriangle> triangles, float spacing) {
    std::map<Key, Float3> result;

    for (const auto& t : triangles) {
        if (!t.swimmable) {
            continue;
        }

        Float3 low = t.vertices[0], high = low;

        for (auto p : t.vertices) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = std::min(low[axis], p[axis]);
                high[axis] = std::max(high[axis], p[axis]);
            }
        }

        if (high[2] - low[2] > .25f) {
            continue;
        }

        for (int x = int(std::ceil(low[0] / spacing)); x <= int(std::floor(high[0] / spacing));
             ++x) {
            for (int y = int(std::ceil(low[1] / spacing)); y <= int(std::floor(high[1] / spacing));
                 ++y) {
                const Float2 p{float(x) * spacing, float(y) * spacing};

                if (const auto z = height(t, p)) {
                    result[{x, y, int(std::round(*z * 4))}] = {p[0], p[1], *z};

                    if (result.size() > kMaximumWaterCells) {
                        throw std::runtime_error("swimming gameplay surface exceeds 32768 cells");
                    }
                }
            }
        }
    }

    return result;
}

// A nearby reachable bank anchors the whole connected swimming surface, not unrelated background water.
unsigned bank(std::span<const EnvironmentFloorSample> floors, Float3 p) {
    unsigned result = ~0u;
    float best = 24 * 24;  // Gameplay-scale shoreline search, not a whole-level proximity test.

    for (unsigned i = 0; i < floors.size(); ++i) {
        const auto& q = floors[i].position;
        const float distance = (p[0] - q[0]) * (p[0] - q[0]) + (p[1] - q[1]) * (p[1] - q[1]);

        if (distance < best && q[2] >= p[2] - .5f && q[2] <= p[2] + 4) {
            best = distance;
            result = i;
        }
    }

    return result;
}

// Find the nearest actual basin floor; missing bathymetry must not invent underwater receivers.
std::optional<float> bottom(std::span<const GameplayTriangle> triangles, Float3 p) {
    std::optional<float> result;

    for (const auto& t : triangles) {
        if (t.swimmable) {
            continue;
        }

        const auto z = height(t, {p[0], p[1]});

        if (z && *z < p[2] - .1f && *z >= p[2] - 64 && (!result || *z > *result)) {
            result = z;
        }
    }

    return result;
}

// One authored water sample and its nearest solid basin floor, when collision provides one.
struct WaterPoint {
    Float3 surface{};
    std::optional<float> bottom;
};

// Sampling policy shared by both sides of a swimming waterline.
struct WaterGrid {
    float spacing = 2;
    float minimum_swim_depth = 1;
};

// Creates one floating or submerged receiver, leaving shallow water to the walking-floor path.
std::optional<EnvironmentFloorSample> swimming_sample(
    const WaterPoint& point, const EnvironmentFloorSample& shore, WaterGrid grid, bool underwater
) {
    auto p = point.surface;
    const float water = p[2];

    // Shallow water is traversed on its bed, without a second reflection medium.
    if (point.bottom && water - *point.bottom < grid.minimum_swim_depth) {
        return {};
    }

    if (underwater) {
        // Unknown bathymetry permits swimming at the surface, but cannot invent a basin floor.
        if (!point.bottom) {
            return {};
        }

        p[2] = *point.bottom;
    }

    EnvironmentFloorSample sample;
    sample.position = p;
    sample.anchor = true;
    sample.sky = underwater ? 0.f : shore.sky;
    sample.lighting_labels = shore.lighting_labels;
    sample.water_height = water;
    // Both layers are swimming domains, not walking-floor fragments.
    sample.swimming_surface = true;
    sample.underwater = underwater;
    sample.height_bounds = Float2{p[2] - .8f, underwater ? water - .25f : water + 3.8f};
    return sample;
}

// Top and submerged grids keep distinct region identities and never share walking edges.
void append_pool(
    std::span<const WaterPoint> points,
    const EnvironmentFloorSample& shore,
    unsigned& region,
    WaterGrid grid,
    std::vector<EnvironmentFloorSample>& floors
) {
    for (bool underwater : {false, true}) {
        std::map<std::array<int, 2>, unsigned> cells;

        for (const auto& point : points) {
            auto sample = swimming_sample(point, shore, grid, underwater);

            // One grid contains only receivers that belong to its swimming medium.
            if (!sample) {
                continue;
            }

            const auto p = sample->position;
            sample->region = region;
            cells[{int(std::round(p[0] / grid.spacing)), int(std::round(p[1] / grid.spacing))}] =
                unsigned(floors.size());
            floors.push_back(std::move(*sample));
        }

        for (const auto& [cell, id] : cells) {
            for (auto shift : {std::array<int, 2>{1, 0}, std::array<int, 2>{0, 1}}) {
                const auto found = cells.find({cell[0] + shift[0], cell[1] + shift[1]});

                if (found != cells.end()
                    && (underwater
                        || std::abs(floors[id].position[2] - floors[found->second].position[2])
                               <= 2)) {
                    floors[id].neighbours.push_back(found->second);
                    floors[found->second].neighbours.push_back(id);
                }
            }
        }

        ++region;
    }
}

// Wading receivers use real support and reconnect through the ordinary collision-checked floor pass.
void append_wading(
    std::span<const WaterPoint> points,
    const EnvironmentFloorSample& shore,
    unsigned& region,
    float minimum_swim_depth,
    std::vector<EnvironmentFloorSample>& floors
) {
    for (const auto& point : points) {
        // Only known shallow beds qualify; deep or unknown water keeps its swimming domain.
        if (!point.bottom || point.surface[2] - *point.bottom >= minimum_swim_depth) {
            continue;
        }

        EnvironmentFloorSample sample;
        sample.position = {point.surface[0], point.surface[1], *point.bottom};
        sample.region = region++;
        sample.anchor = true;
        sample.sky = shore.sky;
        sample.lighting_labels = shore.lighting_labels;
        // A wading floor shares air-side reflections with dry land, so it has no medium boundary.
        sample.height_bounds = Float2{sample.position[2] - .8f, sample.position[2] + 3.8f};
        floors.push_back(std::move(sample));
    }
}

// Keep sealed rooms beneath a basin dry even when their XY footprints overlap water collision.
void classify_land(
    std::span<const GameplayTriangle> triangles,
    std::vector<EnvironmentFloorSample>& floors,
    const std::function<bool(Float3)>& wet_mask,
    float minimum_swim_depth
) {
    // Classify existing reachable basin floors before attempting any spatial merges.
    for (auto& floor : floors) {
        for (const auto& t : triangles) {
            if (!t.swimmable) {
                continue;
            }

            const auto z = height(t, {floor.position[0], floor.position[1]});

            if (z && floor.position[2] <= *z - minimum_swim_depth) {
                if (wet_mask && !wet_mask({floor.position[0], floor.position[1], *z})) {
                    continue;
                }

                const auto basin = bottom(triangles, {floor.position[0], floor.position[1], *z});

                if (!basin || floor.position[2] < *basin - .25f) {
                    continue;  // A sealed room beneath a basin is not automatically underwater.
                }

                floor.water_height = *z;
                floor.underwater = true;
                break;
            }
        }
    }
}

struct Pool {
    std::vector<Float3> points;
    unsigned shore = ~0u;
};

// Flood only adjacent cells at approximately the same waterline.
Pool take_pool(std::map<Key, Float3>& cells, std::span<const EnvironmentFloorSample> land) {
    std::vector<Key> pending{cells.begin()->first};
    std::vector<Float3> pool;
    unsigned shore = ~0u;

    for (unsigned i = 0; i < pending.size(); ++i) {
        const auto key = pending[i];
        const auto found = cells.find(key);

        if (found == cells.end()) {
            continue;
        }

        const auto p = found->second;
        cells.erase(found);
        pool.push_back(p);

        if (shore == ~0u) {
            shore = bank(land, p);
        }

        for (auto shift :
             {std::array<int, 2>{1, 0},
              std::array<int, 2>{-1, 0},
              std::array<int, 2>{0, 1},
              std::array<int, 2>{0, -1}}) {
            // Small authored height differences must not turn one swimming surface into tiny pools.
            for (int dz = -1; dz <= 1; ++dz) {
                const Key next{key[0] + shift[0], key[1] + shift[1], key[2] + dz};
                const auto neighbour = cells.find(next);

                if (neighbour != cells.end() && std::abs(neighbour->second[2] - p[2]) <= .25f) {
                    pending.push_back(next);
                }
            }
        }
    }

    return {std::move(pool), shore};
}

}  // namespace

void append_swimming_surfaces(
    std::span<const GameplayTriangle> triangles,
    std::vector<EnvironmentFloorSample>& floors,
    float spacing,
    const std::function<bool(Float3)>& wet_mask,
    float minimum_swim_depth
) {
    if (!std::isfinite(spacing) || spacing < .5f || spacing > 8) {
        throw std::runtime_error("invalid swimming surface sample spacing");
    }

    // The threshold is a level policy, not a property of the water renderer or material.
    if (!std::isfinite(minimum_swim_depth) || minimum_swim_depth < .25f || minimum_swim_depth > 8) {
        throw std::runtime_error("invalid minimum swimming depth");
    }

    auto cells = water_cells(triangles, spacing);

    if (wet_mask) {
        std::erase_if(cells, [&](const auto& cell) { return !wet_mask(cell.second); });
    }

    const auto land = floors;
    unsigned region = 0;

    for (const auto& floor : floors) {
        region = std::max(region, floor.region + 1);
    }

    classify_land(triangles, floors, wet_mask, minimum_swim_depth);

    while (!cells.empty()) {
        const auto pool = take_pool(cells, land);

        if (pool.shore != ~0u) {
            std::vector<WaterPoint> points;

            // Reuse bathymetry across wading classification and both swimming layers.
            for (auto surface : pool.points) {
                points.push_back({surface, bottom(triangles, surface)});
            }

            append_wading(points, land[pool.shore], region, minimum_swim_depth, floors);
            append_pool(points, land[pool.shore], region, {spacing, minimum_swim_depth}, floors);
        }
    }
}
}  // namespace openrac::lighting
