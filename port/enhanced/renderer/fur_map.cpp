// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Rasterises authored fur islands and pads their groom across unused atlas borders.
#include <algorithm>
#include <cmath>

#include "enhanced/renderer/fur.h"

namespace openrac::enhanced {
namespace {
using Point = std::array<float, 2>;
using Sample = std::array<float, 3>;  // Coverage followed by signed UV flow.

// Tests a pixel-space polygon without depending on texture colour or the game's image bytes.
bool contains(const FurRegion& region, Point p) {
    bool inside = false;
    if (region.polygon.size() < 3) {
        return false;
    }
    auto a = region.polygon.back();
    for (const auto& b : region.polygon) {
        if ((a[1] > p[1]) != (b[1] > p[1])
            && p[0] < (b[0] - a[0]) * (p[1] - a[1]) / (b[1] - a[1]) + a[0]) {
            inside = !inside;
        }
        a = b;
    }
    return inside;
}

// Applies direction edits and fine exclusions only to a selected triangle's footprint.
Sample groom(const FurProfile& profile, Point p, Sample sample) {
    if (sample[0] == 0) {
        return sample;
    }
    for (const auto& region : profile.regions) {
        if (!contains(region, p)) {
            continue;
        }
        if (region.exclude) {
            sample[0] = 0;
        } else {
            sample[1] = region.direction[0];
            sample[2] = region.direction[1];
        }
    }
    return sample;
}

// Twice-resolution coverage gives four subpixel samples per final map texel.
class FurRaster {
public:
    FurRaster(int width, int height)
        : m_width(width * 2),
          m_height(height * 2),
          m_samples(std::size_t(m_width * m_height)) {}

    // Returns false for an invalid authored triangle range; geometry indices were checked earlier.
    bool paint(
        const scene::LevelData& level, const scene::Primitive& primitive, const FurPatch& patch
    ) {
        for (const auto& range : patch.triangles) {
            if (range[0] > range[1] || range[1] >= primitive.index_count / 3) {
                return false;
            }
            for (unsigned triangle = range[0]; triangle <= range[1]; ++triangle) {
                std::array<Point, 3> uv{};
                for (unsigned corner = 0; corner < 3; ++corner) {
                    const auto index = level.indices[primitive.first_index + triangle * 3 + corner];
                    uv[corner] =
                        {level.vertices[index].uv[0] * float(m_width),
                         level.vertices[index].uv[1] * float(m_height)};
                }
                paint_triangle(uv, patch.direction);
            }
        }
        return true;
    }

    // One half-texel dilation step; vertex eligibility keeps padding off other UV islands.
    void pad() {
        auto padded = m_samples;
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                if (m_samples[index(x, y)][0] == 0) {
                    padded[index(x, y)] = neighbour(x, y);
                }
            }
        }
        m_samples.swap(padded);
    }

    // Encodes coverage and premultiplied flow, preserving a neutral direction outside fur.
    renderer::Rgba8Image encode(const FurProfile& profile) const {
        renderer::Rgba8Image map(profile.width, profile.height);
        for (int y = 0; y < profile.height; ++y) {
            for (int x = 0; x < profile.width; ++x) {
                map.set_texel(x, y, texel(profile, x, y));
            }
        }
        return map;
    }

private:
    std::size_t index(int x, int y) const {
        return std::size_t(y) * std::size_t(m_width) + std::size_t(x);
    }

    // Searches only the four adjacent subpixels, so padding cannot skip a gap in one step.
    Sample neighbour(int x, int y) const {
        constexpr std::array<Point, 4> kNeighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
        for (const auto delta : kNeighbours) {
            const int nx = x + int(delta[0]), ny = y + int(delta[1]);
            if (nx >= 0 && ny >= 0 && nx < m_width && ny < m_height
                && m_samples[index(nx, ny)][0] != 0) {
                return m_samples[index(nx, ny)];
            }
        }
        return {};
    }

    // Signed edge tests accept both triangle windings and discard degenerate UV triangles.
    void paint_triangle(const std::array<Point, 3>& uv, Point flow) {
        const auto edge = [](Point a, Point b, Point p) {
            return (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
        };
        const float area = edge(uv[0], uv[1], uv[2]);
        if (std::abs(area) < 1e-6f) {
            return;
        }
        const float sign = area > 0 ? 1.f : -1.f;
        const int x0 = int(std::clamp(
            std::floor(std::min({uv[0][0], uv[1][0], uv[2][0]})), 0.f, float(m_width - 1)
        ));
        const int y0 = int(std::clamp(
            std::floor(std::min({uv[0][1], uv[1][1], uv[2][1]})), 0.f, float(m_height - 1)
        ));
        const int x1 = int(
            std::clamp(std::ceil(std::max({uv[0][0], uv[1][0], uv[2][0]})), 0.f, float(m_width - 1))
        );
        const int y1 = int(std::clamp(
            std::ceil(std::max({uv[0][1], uv[1][1], uv[2][1]})), 0.f, float(m_height - 1)
        ));
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                const Point p{float(x) + .5f, float(y) + .5f};
                if (sign * edge(uv[0], uv[1], p) >= -1e-5f && sign * edge(uv[1], uv[2], p) >= -1e-5f
                    && sign * edge(uv[2], uv[0], p) >= -1e-5f) {
                    m_samples[index(x, y)] = {1, flow[0], flow[1]};
                }
            }
        }
    }

    // Opposing directions fall back to a selected sample rather than encoding a zero-length groom.
    std::uint32_t texel(const FurProfile& profile, int x, int y) const {
        float coverage = 0;
        Point flow{}, fallback{};
        for (int dy = 0; dy < 2; ++dy) {
            for (int dx = 0; dx < 2; ++dx) {
                const auto sample = groom(
                    profile,
                    {float(x) + (float(dx) + .5f) * .5f, float(y) + (float(dy) + .5f) * .5f},
                    m_samples[index(x * 2 + dx, y * 2 + dy)]
                );
                if (sample[0] != 0) {
                    coverage += .25f;
                    fallback = {sample[1], sample[2]};
                    flow[0] += .25f * sample[1];
                    flow[1] += .25f * sample[2];
                }
            }
        }
        const float length = std::hypot(flow[0], flow[1]);
        for (unsigned axis = 0; axis < 2; ++axis) {
            flow[axis] = coverage * (length > 1e-6f ? flow[axis] / length : fallback[axis]);
        }
        const auto byte = [](float value) {
            return unsigned(std::lround(std::clamp(value, 0.f, 1.f) * 255));
        };
        return renderer::rgba(
            byte(coverage), byte(.5f + .5f * flow[0]), byte(.5f + .5f * flow[1]), 255
        );
    }

    int m_width, m_height;
    std::vector<Sample> m_samples;
};
}  // namespace

renderer::Rgba8Image make_fur_map(
    const scene::LevelData& level, const scene::Primitive& primitive, const FurProfile& profile
) {
    // Authored maps stay bounded; malformed geometry or triangle labels produce no map.
    if (profile.width <= 0 || profile.height <= 0 || profile.width > 1024 || profile.height > 1024
        || !fur_uv_signature(level, primitive)) {
        return {};
    }
    FurRaster raster(profile.width, profile.height);
    for (const auto& patch : profile.patches) {
        if (!raster.paint(level, primitive, patch)) {
            return {};
        }
    }
    // Four half-texel steps extend two original texels across UV padding.
    for (int step = 0; step < 4; ++step) {
        raster.pad();
    }
    return raster.encode(profile);
}
}  // namespace openrac::enhanced
