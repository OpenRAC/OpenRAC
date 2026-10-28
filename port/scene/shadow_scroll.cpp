// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "scene/shadow_scroll.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace openrac::scene {
namespace {
// Split one logical rectangle wherever its physical image coordinates wrap.
void split_region(
    std::vector<ShadowScrollRegion>& regions,
    std::array<int, 4> rect,
    std::array<std::int64_t, 2> origin,
    int size
) {
    for (int y = rect[1]; y < rect[1] + rect[3];) {
        const int py = shadow_wrap(origin[1] + y, size);
        const int height = std::min(rect[1] + rect[3] - y, size - py);
        for (int x = rect[0]; x < rect[0] + rect[2];) {
            const int px = shadow_wrap(origin[0] + x, size);
            const int width = std::min(rect[0] + rect[2] - x, size - px);
            regions.push_back({{x, y, width, height}, {px, py, width, height}});
            x += width;
        }
        y += height;
    }
}

// Project all scene bounds onto the light's depth axis, independent of the camera.
std::array<float, 2> depth_bounds(
    const renderer::Mat4& basis, renderer::Vec3 low, renderer::Vec3 high
) {
    if (low[0] > high[0]) {
        return {-1024, 1024};
    }
    float near = 3e38f, far = -3e38f;
    for (unsigned c = 0; c < 8; ++c) {
        renderer::Vec3 point;
        for (unsigned k = 0; k < 3; ++k) {
            point[k] = c & (1u << k) ? high[k] : low[k];
        }
        const float z = renderer::transform_point(basis, point)[2];
        near = std::min(near, z);
        far = std::max(far, z);
    }
    return {near, far};
}
}  // namespace

int shadow_wrap(std::int64_t coordinate, int resolution) {
    if (resolution <= 0) {
        throw std::invalid_argument("invalid shadow scroll resolution");
    }
    const auto remainder = coordinate % resolution;
    return static_cast<int>(remainder < 0 ? remainder + resolution : remainder);
}

std::vector<ShadowScrollRegion> shadow_scroll_regions(
    std::array<std::int64_t, 2> previous, std::array<std::int64_t, 2> next, int size, bool full
) {
    if (size <= 0) {
        throw std::invalid_argument("invalid shadow scroll resolution");
    }
    const auto dx = next[0] - previous[0], dy = next[1] - previous[1];
    std::vector<ShadowScrollRegion> regions;
    if (full || std::abs(dx) >= size || std::abs(dy) >= size) {
        split_region(regions, {0, 0, size, size}, next, size);
        return regions;
    }
    const int width = static_cast<int>(std::abs(dx)), height = static_cast<int>(std::abs(dy));
    if (width) {
        split_region(regions, {dx > 0 ? size - width : 0, 0, width, size}, next, size);
    }
    if (height && width < size) {
        split_region(
            regions,
            {dx < 0 ? width : 0, dy > 0 ? size - height : 0, size - width, height},
            next,
            size
        );
    }
    return regions;
}

renderer::Mat4 shadow_scroll_matrix(
    const ShadowScrollUpdate& update, const ShadowScrollRegion& region, int resolution
) {
    auto matrix = update.matrix;
    // Each region belongs to one world-aligned image-sized tile. Integer tile translations avoid
    // cancellation between a changing camera translation and its physical wrap offset.
    for (unsigned k = 0; k < 2; ++k) {
        const auto tile = (update.origin[k] + region.logical[k] - region.physical[k]) / resolution;
        matrix[12 + k] = static_cast<float>(-1 - 2 * tile);
    }
    return matrix;
}

renderer::Mat4 shadow_region_clip(
    const renderer::Mat4& matrix, const std::array<int, 4>& rect, int resolution
) {
    auto clipped = matrix;
    for (unsigned k = 0; k < 2; ++k) {
        const float scale = static_cast<float>(resolution) / static_cast<float>(rect[2 + k]);
        const float centre = (2.f * static_cast<float>(rect[k]) + static_cast<float>(rect[2 + k]))
                                 / static_cast<float>(resolution)
                             - 1;
        for (unsigned c = 0; c < 4; ++c) {
            clipped[c * 4 + k] = (matrix[c * 4 + k] - centre * matrix[c * 4 + 3]) * scale;
        }
    }
    return clipped;
}

ShadowScrollUpdate ShadowScrollGrid::update(
    const renderer::Mat4& requested,
    renderer::Vec3 direction,
    renderer::Vec3 low,
    renderer::Vec3 high,
    int resolution,
    std::uint64_t generation,
    bool force
) {
    const float length = std::sqrt(
        requested[0] * requested[0] + requested[4] * requested[4] + requested[8] * requested[8]
    );
    if (!(length > 0) || !std::isfinite(length) || resolution <= 0 || !std::isfinite(direction[0])
        || !std::isfinite(direction[1]) || !std::isfinite(direction[2])
        || (direction[0] == 0 && direction[1] == 0 && direction[2] == 0)) {
        throw std::invalid_argument("invalid scrolling shadow projection");
    }
    const float requested_radius = 1 / length;
    const auto basis = renderer::look_along(
        {0, 0, 0},
        direction,
        std::abs(renderer::normalize(direction)[2]) > .95f ? renderer::Vec3{0, 1, 0}
                                                           : renderer::Vec3{0, 0, 1}
    );
    auto bounds = depth_bounds(basis, low, high);
    const float zscale = std::sqrt(
        requested[2] * requested[2] + requested[6] * requested[6] + requested[10] * requested[10]
    );
    if (!(zscale > 0) || !std::isfinite(zscale) || !std::isfinite(requested[14])) {
        throw std::invalid_argument("invalid scrolling shadow depth");
    }
    const float zcentre = -requested[14] / zscale;
    bounds[0] = std::min(bounds[0], zcentre - 1 / zscale);
    bounds[1] = std::max(bounds[1], zcentre + 1 / zscale);
    const bool reset = !m_valid || direction != m_direction || generation != m_generation
                       || resolution != m_resolution || requested_radius > m_radius
                       || bounds[0] < m_zlow || bounds[1] > m_zhigh;
    if (reset) {
        m_radius = std::ceil(requested_radius * 1.05f);
        m_zlow = bounds[0] - 128;
        m_zhigh = bounds[1] + 128;
        m_basis = basis;
        m_direction = direction;
        m_generation = generation;
        m_resolution = resolution;
    }
    ShadowScrollUpdate result;
    result.full = reset || force;
    const double texel = 2.0 * m_radius / resolution;
    for (unsigned k = 0; k < 2; ++k) {
        const double centre = -requested[12 + k] * requested_radius;
        if (!std::isfinite(centre) || std::abs(centre / texel) > 1e12) {
            throw std::invalid_argument("invalid scrolling shadow origin");
        }
        const double requested_cell = centre / texel;
        const auto previous_cell = m_origin[k] + resolution / 2;
        // Retain a cell through tiny camera/snap fluctuations. The frozen projection's existing
        // coverage margin bounds this deadband, so it cannot expose uncovered receivers.
        const double hold = std::min(1.0, (m_radius - requested_radius) / texel);
        const auto cell =
            !reset && std::abs(requested_cell - static_cast<double>(previous_cell)) <= hold
                ? previous_cell
                : static_cast<std::int64_t>(std::llround(requested_cell));
        result.origin[k] = cell - resolution / 2;
        result.offset[k] = shadow_wrap(result.origin[k], resolution);
    }
    result.full = result.full || std::abs(result.origin[0] - m_origin[0]) >= resolution
                  || std::abs(result.origin[1] - m_origin[1]) >= resolution;
    renderer::Mat4 projection = renderer::identity();
    projection[0] = projection[5] = 1 / m_radius;
    projection[12] =
        static_cast<float>(-2.0 * static_cast<double>(result.origin[0]) / resolution - 1);
    projection[13] =
        static_cast<float>(-2.0 * static_cast<double>(result.origin[1]) / resolution - 1);
    projection[10] = 2 / (m_zhigh - m_zlow);
    projection[14] = -(m_zhigh + m_zlow) / (m_zhigh - m_zlow);
    result.matrix = renderer::multiply(projection, m_basis);
    result.regions = shadow_scroll_regions(m_origin, result.origin, resolution, result.full);
    m_origin = result.origin;
    m_valid = true;
    return result;
}
}  // namespace openrac::scene
