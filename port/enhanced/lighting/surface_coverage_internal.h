// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private two-sided coverage queries; the immutable BVHs outlive every seed flood in one request.
#pragma once
#include <optional>

#include "enhanced/lighting/interior_inference_internal.h"
#include "enhanced/lighting/surface_coverage.h"

namespace openrac::lighting {
// Owns full and walkable-only collision BVHs; only the level controller calls these queries.
class CoverageSurfaces {
public:
    // Borrows triangles until the complete surface-coverage request returns.
    explicit CoverageSurfaces(const std::vector<interiors::Triangle>& triangles);
    // Finds the nearest two-sided hit; down rays only accept walkable geometric slopes.
    std::optional<double> ray(CoveragePoint origin, CoveragePoint direction, double distance) const;
    // Returns floor and first ceiling only for broadly roofed, sufficiently enclosed cells.
    std::optional<std::array<double, 2>> room(
        CoveragePoint position, const CoverageSettings& settings
    ) const;

private:
    // Tests the four roof-offset samples around the cell centre.
    bool covered_roof(
        CoveragePoint position, double roof, double roof_distance, const CoverageSettings& settings
    ) const;
    // Tests eight horizontal directions for sufficient nearby walls.
    bool enclosed(CoveragePoint position) const;
    const std::vector<interiors::Triangle>& m_triangles;  // Borrowed until all seed floods finish.
    std::vector<interiors::Triangle> m_floor;             // Owned slope-filtered support geometry.
    std::optional<interiors::detail::Scene> m_scene;  // Two-sided wall and ceiling acceleration.
    std::optional<interiors::detail::Scene>
        m_floor_scene;  // Two-sided walkable support acceleration.
};
}  // namespace openrac::lighting
