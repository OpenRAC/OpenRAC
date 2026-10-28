// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/water_mesh.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "common/log.h"
#include "common/profiler.h"
#include "common/stats.h"

namespace openrac::enhanced {
WaterMesh::WaterMesh(const std::filesystem::path& path, const std::array<char, 32>& fingerprint) {
    // Keep all authored layers: the simulation shares a field, not a rest elevation.
    std::ifstream mesh_file(path.parent_path() / "water.mesh", std::ios::binary);
    std::array<char, 8> mesh_magic{};
    unsigned triangle_count = 0, template_count = 0;
    mesh_file.read(mesh_magic.data(), 8);
    mesh_file.read(reinterpret_cast<char*>(&triangle_count), 4);
    mesh_file.read(reinterpret_cast<char*>(&template_count), 4);
    std::array<char, 32> mesh_fingerprint{};
    mesh_file.read(mesh_fingerprint.data(), 32);
    if (mesh_fingerprint != fingerprint) {
        throw std::runtime_error("water mesh and coastline bakes do not match");
    }
    if (!mesh_file || std::string_view(mesh_magic.data(), 8) != "ORWMESH2" || triangle_count == 0
        || triangle_count > 100000 || template_count == 0 || template_count > 4096) {
        throw std::runtime_error("missing or invalid water rest mesh; rebake water");
    }
    m_rest.resize(triangle_count);
    std::size_t tile_references = 0;
    for (unsigned i = 0; i < triangle_count; ++i) {
        auto& t = m_rest[i];
        mesh_file.read(reinterpret_cast<char*>(&t.surface), 4);
        mesh_file.read(reinterpret_cast<char*>(t.p.data()), 36);
        if (t.surface < 1 || t.surface > 7) {
            throw std::runtime_error("invalid water mesh surface");
        }
        for (const auto& point : t.p) {
            for (float x : point) {
                if (!std::isfinite(x) || std::abs(x) > 100000) {
                    throw std::runtime_error("invalid water rest vertex");
                }
            }
        }
        int x0 = int(std::floor(std::min({t.p[0][0], t.p[1][0], t.p[2][0]})));
        int x1 = int(std::floor(std::max({t.p[0][0], t.p[1][0], t.p[2][0]})));
        int y0 = int(std::floor(std::min({t.p[0][1], t.p[1][1], t.p[2][1]})));
        int y1 = int(std::floor(std::max({t.p[0][1], t.p[1][1], t.p[2][1]})));
        const auto references = std::size_t(x1 - x0 + 1) * std::size_t(y1 - y0 + 1);
        tile_references += references;
        if (references > 100000 || tile_references > 4 * 1024 * 1024) {
            throw std::runtime_error("water rest triangle exceeds spatial budget");
        }
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                m_rest_tiles[{x, y}].push_back(i);
            }
        }
    }
    m_weights = {{{0, 0}}, {{1, 0}}, {{0, 1}}};
    std::size_t dense_vertices = 0;
    for (unsigned i = 0; i < template_count; ++i) {
        std::array<int, 4> shape{};
        unsigned count = 0;
        mesh_file.read(reinterpret_cast<char*>(shape.data()), 16);
        mesh_file.read(reinterpret_cast<char*>(&count), 4);
        dense_vertices += count;
        if (count == 0 || count % 3 != 0 || dense_vertices > 4 * 1024 * 1024) {
            throw std::runtime_error("invalid baked water mesh vertex count");
        }
        const auto first = static_cast<unsigned>(m_weights.size());
        if (!m_templates.emplace(shape, Shape{first, count}).second) {
            throw std::runtime_error("duplicate water mesh shape");
        }
        m_weights.resize(first + count);
        mesh_file
            .read(reinterpret_cast<char*>(m_weights.data() + first), std::streamsize(count * 8));
        for (const auto& w : std::span(m_weights).subspan(first, count)) {
            if (!std::isfinite(w[0]) || !std::isfinite(w[1]) || w[0] < 0 || w[1] < 0
                || w[0] + w[1] > 1.000001f) {
                throw std::runtime_error("invalid baked water mesh mapping");
            }
        }
    }
    if (!mesh_file || mesh_file.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("invalid water rest mesh size");
    }
}

float WaterMesh::rest_height(const renderer::EffectVertex& p, unsigned surface) const {
    auto found = m_rest_tiles.find({int(std::floor(p.x)), int(std::floor(p.y))});
    float best = std::numeric_limits<float>::max(), result = p.z;
    if (found == m_rest_tiles.end()) {
        return result;
    }
    for (unsigned index : found->second) {
        const auto& t = m_rest[index];
        if (t.surface != surface) {
            continue;
        }
        const auto& a = t.p[0];
        const auto& b = t.p[1];
        const auto& c = t.p[2];
        float det = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
        if (std::abs(det) < 1e-8f) {
            continue;
        }
        float u = ((b[1] - c[1]) * (p.x - c[0]) + (c[0] - b[0]) * (p.y - c[1])) / det;
        float v = ((c[1] - a[1]) * (p.x - c[0]) + (a[0] - c[0]) * (p.y - c[1])) / det;
        if (std::min({u, v, 1 - u - v}) < -.001f) {
            continue;
        }
        float z = u * a[2] + v * b[2] + (1 - u - v) * c[2];
        if (std::abs(z - p.z) < best) {
            best = std::abs(z - p.z);
            result = z;
        }
    }
    return result;
}

// Starts a pass without freeing the vectors reused by subsequent frames.
void WaterMesh::begin_pass() {
    m_sources.clear();
    m_used = 0;
}

// Each instance references the original coarse triangle, never an expanded CPU vertex stream.
renderer::EffectMesh WaterMesh::prepare(
    const renderer::EffectStrip& strip, std::span<const renderer::EffectVertex> vertices
) {
    const profiling::Zone trace("Water coarse instances");
    if (m_used == m_plans.size()) {
        m_plans.emplace_back();
    }
    auto& plan = m_plans[m_used];
    plan.batches.clear();
    plan.vertices = 0;
    std::vector<renderer::EffectVertex> rest(vertices.begin(), vertices.end());
    for (auto& p : rest) {
        p.z = rest_height(p, strip.surface);
    }
    constexpr unsigned orders[6][3] =
        {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (std::size_t k = 0; k + 2 < rest.size(); ++k) {
        const auto& p = rest[k];
        const auto& q = rest[k + 1];
        const auto& r = rest[k + 2];
        // Strip restart connectors have no visible triangle.
        if (std::abs((q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)) < 1e-8f) {
            continue;
        }
        Shape shape{0, 3};  // The first resident template is a flat, untessellated fallback.
        const unsigned* order = orders[0];
        bool matched = false;
        for (const auto& candidate : orders) {
            const auto& a = rest[k + candidate[0]];
            const auto& b = rest[k + candidate[1]];
            const auto& c = rest[k + candidate[2]];
            const auto quantize = [](float x) {
                return int(std::floor(x * 256 + .5f));
            };
            const std::array<int, 4> key{
                quantize(b.x - a.x), quantize(b.y - a.y), quantize(c.x - a.x), quantize(c.y - a.y)
            };
            auto found = m_templates.find(key);
            if (found != m_templates.end()) {
                shape = found->second;
                order = candidate;
                matched = true;
                break;
            }
        }
        if (!matched && !m_missing_template) {
            log::warn(
                "Water bake missing mesh shape for surface {}; using coarse triangle", strip.surface
            );
            m_missing_template = true;
        }
        const auto first = static_cast<unsigned>(m_sources.size());
        Source source{};
        for (unsigned i = 0; i < 3; ++i) {
            const auto& a = rest[k + order[i]];
            source[i * 2] =
                {std::bit_cast<std::uint32_t>(a.x),
                 std::bit_cast<std::uint32_t>(a.y),
                 std::bit_cast<std::uint32_t>(a.z),
                 a.rgba};
            source[i * 2 + 1] =
                {std::bit_cast<std::uint32_t>(a.s), std::bit_cast<std::uint32_t>(a.t), 0, 0};
        }
        source[6] =
            {std::bit_cast<std::uint32_t>(strip.uv_motion.offset[0]),
             std::bit_cast<std::uint32_t>(strip.uv_motion.offset[1]),
             std::bit_cast<std::uint32_t>(strip.uv_motion.amplitude),
             (strip.uv_motion.phase & 511u) | (strip.uv_motion.enabled ? 0x80000000u : 0u)};
        m_sources.push_back(source);
        if (!plan.batches.empty() && plan.batches.back().shape == shape) {
            ++plan.batches.back().count;
        } else {
            plan.batches.push_back({shape, first, 1});
        }
        plan.vertices += shape.count;
    }
    const renderer::EffectMesh result{
        m_used++, plan.vertices, static_cast<std::uint32_t>(plan.batches.size())
    };
    stats::gauge("water coarse triangles", double(m_sources.size()));
    stats::gauge("water CPU expanded vertices", 0.);
    return result;
}
}  // namespace openrac::enhanced
