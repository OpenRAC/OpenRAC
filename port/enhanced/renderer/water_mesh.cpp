// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/renderer/water_mesh.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "common/log.h"
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
        auto& vertices = m_templates[shape];
        if (!vertices.empty()) {
            throw std::runtime_error("duplicate water mesh shape");
        }
        vertices.resize(count);
        mesh_file.read(reinterpret_cast<char*>(vertices.data()), std::streamsize(count * 8));
        for (const auto& w : vertices) {
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

bool WaterMesh::build(
    const renderer::EffectStrip& strip,
    std::span<const renderer::EffectVertex> vertices,
    std::vector<renderer::EffectVertex>& output
) {
    if (vertices.size() < 3 || strip.surface == 0 || strip.surface > 7) {
        return false;
    }
    std::vector<renderer::EffectVertex> rest(vertices.begin(), vertices.end());
    for (auto& p : rest) {
        p.z = rest_height(p, strip.surface);
    }
    std::uint64_t key = 14695981039346656037ull;
    const auto hash = [&](std::uint32_t word) {
        key = (key ^ word) * 1099511628211ull;
    };
    hash(strip.surface);
    hash(unsigned(rest.size()));
    for (const auto& p : rest) {
        hash(std::bit_cast<std::uint32_t>(p.x));
        hash(std::bit_cast<std::uint32_t>(p.y));
        hash(std::bit_cast<std::uint32_t>(p.z));
    }
    const auto map_baked_vertices = [&] {
        output.clear();
        static constexpr unsigned orders[6][3] =
            {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
        for (std::size_t k = 0; k + 2 < rest.size(); ++k) {
            // Strip restart connectors have no area and are not part of the water mesh.
            const auto& p = rest[k];
            const auto& q = rest[k + 1];
            const auto& r = rest[k + 2];
            if (std::abs((q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)) < 1e-8f) {
                continue;
            }
            bool matched = false;
            for (const auto& order : orders) {
                const auto& a = rest[k + order[0]];
                const auto& b = rest[k + order[1]];
                const auto& c = rest[k + order[2]];
                const auto quantize = [](float x) {
                    return int(std::floor(x * 256 + .5f));
                };
                std::array<int, 4> shape{
                    quantize(b.x - a.x),
                    quantize(b.y - a.y),
                    quantize(c.x - a.x),
                    quantize(c.y - a.y)
                };
                auto found = m_templates.find(shape);
                if (found == m_templates.end()) {
                    continue;
                }
                for (const auto& weights : found->second) {
                    float v = weights[0], w = weights[1], u = 1 - v - w;
                    renderer::EffectVertex p{};
                    p.x = a.x * u + b.x * v + c.x * w;
                    p.y = a.y * u + b.y * v + c.y * w;
                    p.z = a.z * u + b.z * v + c.z * w;
                    p.s = a.s * u + b.s * v + c.s * w;
                    p.t = a.t * u + b.t * v + c.t * w;
                    for (unsigned shift = 0; shift < 32; shift += 8) {
                        auto value = unsigned(std::lround(
                            float((a.rgba >> shift) & 255) * u + float((b.rgba >> shift) & 255) * v
                            + float((c.rgba >> shift) & 255) * w
                        ));
                        p.rgba |= std::min(value, 255u) << shift;
                    }
                    output.push_back(p);
                }
                matched = true;
                break;
            }
            if (!matched) {
                // Preserve a flat rest triangle for an unsupported draw layout, never tessellate at runtime.
                if (!m_missing_template) {
                    log::warn(
                        "water bake missing mesh shape: surface {}, edges ({}, {}) ({}, {})",
                        strip.surface,
                        rest[k + 1].x - rest[k].x,
                        rest[k + 1].y - rest[k].y,
                        rest[k + 2].x - rest[k].x,
                        rest[k + 2].y - rest[k].y
                    );
                    m_missing_template = true;
                }
                output.insert(output.end(), rest.begin() + k, rest.begin() + k + 3);
            }
        }
    };
    auto found = m_mesh_cache.find(key);
    bool reused = false;
    if (found != m_mesh_cache.end() && found->second.source.size() == rest.size()) {
        const auto& cached = found->second;
        float ds = rest[0].s - cached.source[0].s, dt = rest[0].t - cached.source[0].t;
        bool translation = true, uniform_colour = true;
        for (std::size_t i = 0; i < rest.size(); ++i) {
            const auto& a = rest[i];
            const auto& b = cached.source[i];
            translation = translation && a.x == b.x && a.y == b.y && a.z == b.z
                          && std::abs(a.s - b.s - ds) < 1e-5f && std::abs(a.t - b.t - dt) < 1e-5f;
            uniform_colour = uniform_colour && a.rgba == rest[0].rgba;
        }
        bool same_colour = true;
        for (std::size_t i = 0; i < rest.size(); ++i) {
            same_colour = same_colour && rest[i].rgba == cached.source[i].rgba;
        }
        if (translation && (same_colour || uniform_colour)) {
            output = cached.triangles;
            for (auto& p : output) {
                p.s += ds;
                p.t += dt;
                if (!same_colour) {
                    p.rgba = rest[0].rgba;
                }
            }
            reused = true;
        }
    }
    if (!reused) {
        map_baked_vertices();
        // Bound optional CPU storage; displacement history always stays on the GPU.
        const auto old_size = found != m_mesh_cache.end() ? found->second.triangles.size() : 0;
        if (m_cached_vertices - old_size + output.size() <= 4 * 1024 * 1024) {
            if (found != m_mesh_cache.end()) {
                m_cached_vertices -= found->second.triangles.size();
            }
            m_cached_vertices += output.size();
            m_mesh_cache[key] = {rest, output};
        }
    }
    stats::gauge("water mesh reused", reused ? 1. : 0.);
    stats::gauge("water mesh vertices", double(output.size()));
    return true;
}

void WaterMesh::clear_cache() {
    m_mesh_cache.clear();
    m_cached_vertices = 0;
}
}  // namespace openrac::enhanced
