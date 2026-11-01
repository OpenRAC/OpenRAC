// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Pure wireframe geometry for camera and shadow diagnostics, in GL clip coordinates [-1, 1].
#include <array>
#include <cmath>
#include <optional>

#include "renderer/debug_lines.h"

namespace openrac::renderer {
namespace {
// Intersects three normalised inward planes without requiring a general matrix inverse.
std::optional<Vec3> intersection(const std::array<Vec3, 3>& n, const Vec3& d) {
    const auto yz = cross(n[1], n[2]);
    const auto zx = cross(n[2], n[0]);
    const auto xy = cross(n[0], n[1]);
    const float determinant = dot(n[0], yz);
    // Parallel planes cannot define a finite diagnostic corner.
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-7f) {
        return std::nullopt;
    }
    Vec3 point;
    for (unsigned axis = 0; axis < 3; ++axis) {
        point[axis] = -(d[0] * yz[axis] + d[1] * zx[axis] + d[2] * xy[axis]) / determinant;
        if (!std::isfinite(point[axis])) {
            return std::nullopt;
        }
    }
    return point;
}

// Only the low endpoint emits each undirected edge of these eight bit-ordered corners.
void append_edges(
    std::vector<DebugLineVertex>& vertices, const std::array<Vec3, 8>& corners, Vec3 colour
) {
    for (unsigned corner = 0; corner < corners.size(); ++corner) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            const unsigned bit = 1u << axis;
            if ((corner & bit) == 0) {
                vertices.push_back({corners[corner], colour});
                vertices.push_back({corners[corner | bit], colour});
            }
        }
    }
}
}  // namespace

// Validates every corner before publishing any lines, so singular inputs leave no partial box.
bool append_debug_frustum(std::vector<DebugLineVertex>& vertices, const Mat4& clip, Vec3 colour) {
    std::array<Vec3, 8> corners;
    for (unsigned corner = 0; corner < corners.size(); ++corner) {
        std::array<Vec3, 3> normals;
        Vec3 distances;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float sign = (corner & (1u << axis)) ? -1.0f : 1.0f;
            Vec3 normal;
            for (unsigned coordinate = 0; coordinate < 3; ++coordinate) {
                normal[coordinate] = clip[coordinate * 4 + 3] + sign * clip[coordinate * 4 + axis];
            }
            const float length = std::sqrt(dot(normal, normal));
            // Infinite or degenerate projections must not add partial geometry.
            if (!std::isfinite(length) || length < 1e-12f) {
                return false;
            }
            for (unsigned coordinate = 0; coordinate < 3; ++coordinate) {
                normals[axis][coordinate] = normal[coordinate] / length;
            }
            distances[axis] = (clip[15] + sign * clip[12 + axis]) / length;
        }
        const auto point = intersection(normals, distances);
        if (!point) {
            return false;
        }
        corners[corner] = *point;
    }
    append_edges(vertices, corners, colour);
    return true;
}

// Retains the lighting overlays' bit-ordered box edges.
void append_debug_box(
    std::vector<DebugLineVertex>& vertices,
    renderer::Vec3 minimum,
    renderer::Vec3 maximum,
    renderer::Vec3 colour
) {
    constexpr unsigned kCorners = 8;
    std::array<renderer::Vec3, kCorners> corners{};
    for (unsigned corner = 0; corner < kCorners; ++corner) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            corners[corner][axis] = (corner & (1u << axis)) ? maximum[axis] : minimum[axis];
        }
    }
    append_edges(vertices, corners, colour);
}

}  // namespace openrac::renderer
