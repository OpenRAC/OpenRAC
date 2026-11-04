// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Surface coverage queries use geometric winding-independent support and two-sided Embree traversal.
#include <algorithm>
#include <cmath>
#include <numbers>

#include "enhanced/lighting/surface_coverage_internal.h"

namespace openrac::lighting {
namespace {
// Existing planner enclosure dimensions are independent of authored mesh normals.
constexpr unsigned kMinimumWalls = 5;
constexpr double kWallDistance = 32;
constexpr double kRayNear = 0.05;
constexpr double kEyeHeight = 1.5;

// Converts only the ray presented to Embree; all lattice and final plane arithmetic stays double.
interiors::Point ray_point(CoveragePoint point) {
    return {
        static_cast<float>(point[0]), static_cast<float>(point[1]), static_cast<float>(point[2])
    };
}

// Derives the collision plane in double precision without reading any authored vertex normals.
CoveragePoint geometric_normal(const interiors::Triangle& triangle) {
    std::array<CoveragePoint, 2> edges{};

    // Double plane arithmetic retains the original planner lattice decisions.
    for (unsigned axis = 0; axis < triangle.vertices[0].size(); ++axis) {
        edges[0][axis] =
            static_cast<double>(triangle.vertices[1][axis]) - triangle.vertices[0][axis];
        edges[1][axis] =
            static_cast<double>(triangle.vertices[2][axis]) - triangle.vertices[0][axis];
    }

    const auto& u = edges[0];
    const auto& v = edges[1];
    return {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
}

// Uses geometric slope only; neither authored normals nor triangle winding affect support.
std::vector<interiors::Triangle> walkable_triangles(
    const std::vector<interiors::Triangle>& triangles
) {
    std::vector<interiors::Triangle> floor;

    // Floor support is selected from geometric slopes, independent of authored normals.
    for (const auto& triangle : triangles) {
        const auto normal = geometric_normal(triangle);
        const double squared =
            normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];

        // Match the reference slope cosine and reject degenerate support triangles.
        if (squared > 1e-16 && std::abs(normal[2]) >= 0.6 * std::sqrt(squared)) {
            floor.push_back(triangle);
        }
    }

    return floor;
}

// Recovers the plane distance in the Python reference's precision after Embree chooses a triangle.
double plane_distance(
    const interiors::Triangle& triangle, CoveragePoint origin, CoveragePoint direction
) {
    const auto normal = geometric_normal(triangle);
    double numerator = 0;
    double denominator = 0;

    // Distance refinement avoids float hit rounding changing snapped coverage bounds.
    for (unsigned axis = 0; axis < origin.size(); ++axis) {
        numerator += normal[axis] * (triangle.vertices[0][axis] - origin[axis]);
        denominator += normal[axis] * direction[axis];
    }

    return numerator / denominator;
}

}  // namespace

CoverageSurfaces::CoverageSurfaces(const std::vector<interiors::Triangle>& triangles)
    : m_triangles(triangles),
      m_floor(walkable_triangles(triangles)) {
    // Empty collision has no occluders and needs no Embree allocation.
    if (!m_triangles.empty()) {
        m_scene.emplace(m_triangles);
    }

    // Wall-only collision can block enclosure rays without providing any walkable support.
    if (!m_floor.empty()) {
        m_floor_scene.emplace(m_floor);
    }
}

std::optional<double> CoverageSurfaces::ray(
    CoveragePoint origin, CoveragePoint direction, double distance
) const {
    const bool down = direction[0] == 0 && direction[1] == 0 && direction[2] < 0;
    const auto& scene = down ? m_floor_scene : m_scene;
    const auto& triangles = down ? m_floor : m_triangles;

    // Empty support or occluder sets are ordinary misses, not BVH allocation errors.
    if (!scene) {
        return std::nullopt;
    }

    const auto hit = scene->hit(
        ray_point(origin),
        ray_point(direction),
        static_cast<float>(distance),
        static_cast<float>(kRayNear)
    );

    // Embree reports absence without a usable primitive index.
    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return std::nullopt;
    }

    const double refined = plane_distance(triangles.at(hit.hit.primID), origin, direction);

    // Float traversal must not turn a near-parallel or endpoint rounding artefact into invalid coverage.
    if (!std::isfinite(refined) || refined <= kRayNear || refined > distance) {
        return std::nullopt;
    }

    return refined;
}

std::optional<std::array<double, 2>> CoverageSurfaces::room(
    CoveragePoint position, const CoverageSettings& settings
) const {
    const auto down = ray(position, {0, 0, -1}, kWallDistance);

    // Without support, the sample provides no playable-room evidence.
    if (!down) {
        return std::nullopt;
    }

    const double floor = position[2] - *down;
    position[2] = floor + kEyeHeight;
    const double roof_distance = std::max(64.0, settings.max_rise + kWallDistance);
    const auto roof = ray(position, {0, 0, 1}, roof_distance);

    // Missing or very low ceilings cannot provide a usable room height.
    if (!roof || *roof < kEyeHeight) {
        return std::nullopt;
    }

    // A narrow roof edge must not classify an outdoor overhang as enclosed.
    if (!covered_roof(position, *roof, roof_distance, settings)) {
        return std::nullopt;
    }

    position[2] = floor + std::min(3.0, (*roof + kEyeHeight) * 0.5);

    // The enclosure test separates rooms from exposed roofed outdoor areas.
    if (!enclosed(position)) {
        return std::nullopt;
    }

    return std::array<double, 2>{floor, floor + kEyeHeight + *roof};
}

bool CoverageSurfaces::covered_roof(
    CoveragePoint position, double roof, double roof_distance, const CoverageSettings& settings
) const {
    const double cell = std::min(settings.spacing[0], settings.spacing[1]) * 2;
    unsigned covered = 1;

    // Four offset rays prevent a single roof edge from classifying an outdoor cell as a room.
    for (const auto offset :
         std::array<std::array<int, 2>, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}}) {
        const auto hit =
            ray({position[0] + offset[0] * cell / 4,
                 position[1] + offset[1] * cell / 4,
                 position[2]},
                {0, 0, 1},
                roof_distance);
        covered += hit && std::abs(*hit - roof) <= std::max(4.0, cell);
    }

    // Most offset rays must agree with the first ceiling height.
    if (covered < 4) {
        return false;
    }

    return true;
}

bool CoverageSurfaces::enclosed(CoveragePoint position) const {
    unsigned walls = 0;

    // Eight equally spaced horizontal rays retain the reference enclosure test.
    for (unsigned index = 0; index < 8; ++index) {
        const double angle = index * std::numbers::pi / 4;
        walls += ray(position, {std::cos(angle), std::sin(angle), 0}, kWallDistance).has_value();
    }

    // Five of eight wall directions preserve the established room heuristic.
    if (walls < kMinimumWalls) {
        return false;
    }

    return true;
}
}  // namespace openrac::lighting
