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
#include "renderer/render_opt.h"

namespace openrac::enhanced {
namespace {
using Barycentric = std::array<double, 3>;

// The bake's dyadic longest-edge split (tools/water/water_mesh.cpp) of a template's own shape,
// stopped at `limit` units: the same midpoints, so a coarse leaf covers whole fine leaves.
void subdivide_shape(
    const std::array<std::array<double, 2>, 3>& corners,
    const std::array<Barycentric, 3>& weights,
    double limit,
    std::vector<std::array<float, 2>>& out,
    unsigned depth
) {
    std::array<std::array<double, 2>, 3> points{};
    for (unsigned vertex = 0; vertex < 3; ++vertex) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            for (unsigned component = 0; component < 3; ++component) {
                points[vertex][axis] += corners[component][axis] * weights[vertex][component];
            }
        }
    }
    std::array<double, 3> lengths{};
    for (unsigned edge = 0; edge < 3; ++edge) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            const double delta = points[edge][axis] - points[(edge + 1) % 3][axis];
            lengths[edge] += delta * delta;
        }
    }
    const auto edge =
        static_cast<unsigned>(std::max_element(lengths.begin(), lengths.end()) - lengths.begin());
    if (depth > 64 || lengths[edge] <= limit * limit + 1e-6) {
        for (const auto& weight : weights) {
            out.push_back({float(weight[1]), float(weight[2])});
        }
        return;
    }
    const auto& a = weights[edge];
    const auto& b = weights[(edge + 1) % 3];
    const auto& c = weights[(edge + 2) % 3];
    Barycentric middle{};
    for (unsigned component = 0; component < 3; ++component) {
        middle[component] = (a[component] + b[component]) * .5;
    }
    subdivide_shape(corners, {a, middle, c}, limit, out, depth + 1);
    subdivide_shape(corners, {middle, b, c}, limit, out, depth + 1);
}
}  // namespace

WaterMesh::WaterMesh() {
    constexpr int kSteps = 32;  // Subdivide the original 50-unit ocean grid.
    const auto point = [](int x, int y) {
        return std::array{float(x) / kSteps, float(y) / kSteps};
    };
    for (int y = 0; y < kSteps; ++y) {
        for (int x = 0; x + y < kSteps; ++x) {
            m_weights.insert(m_weights.end(), {point(x, y), point(x + 1, y), point(x, y + 1)});
            // The last diagonal row has no second triangle.
            if (x + y + 1 < kSteps) {
                m_weights.insert(
                    m_weights.end(), {point(x + 1, y), point(x + 1, y + 1), point(x, y + 1)}
                );
            }
        }
    }
    m_ocean_shape = {0, static_cast<unsigned>(m_weights.size())};
    m_ocean_linear = {static_cast<unsigned>(m_weights.size()), 3};
    m_weights.insert(m_weights.end(), {{0, 0}, {1, 0}, {0, 1}});
}

renderer::EffectMesh WaterMesh::prepare_ocean(
    const renderer::Vec3& eye, float height, float near_radius, float horizon
) {
    if (m_used == m_plans.size()) {
        m_plans.emplace_back();
    }
    auto& plan = m_plans[m_used];
    const auto first = static_cast<unsigned>(m_sources.size());
    constexpr int kSegments = 32;
    const float spacing = 2.f * near_radius / kSegments;
    const float x = std::floor(eye[0] / spacing) * spacing;
    const float y = std::floor(eye[1] / spacing) * spacing;
    const auto triangle = [&](auto a, auto b, auto c) {
        Source source{};
        const std::array points{a, b, c};
        for (unsigned i = 0; i < points.size(); ++i) {
            source[2 * i] =
                {std::bit_cast<std::uint32_t>(x + points[i][0]),
                 std::bit_cast<std::uint32_t>(y + points[i][1]),
                 std::bit_cast<std::uint32_t>(height),
                 0x80808080u};
        }
        m_sources.push_back(source);
    };
    using Point = std::array<float, 2>;
    const auto quad = [&](Point a, Point b, Point c, Point d) {
        triangle(a, b, c);
        triangle(b, d, c);
    };
    for (int row = 0; row < kSegments; ++row) {
        for (int column = 0; column < kSegments; ++column) {
            const float a = -near_radius + column * spacing;
            const float b = -near_radius + row * spacing;
            quad({a, b}, {a + spacing, b}, {a, b + spacing}, {a + spacing, b + spacing});
        }
    }
    // Both sides of each ring boundary have the same vertices and wave samples.
    for (float inner = near_radius; inner < horizon;) {
        const float outer = std::min(2 * inner, horizon);
        for (int segment = 0; segment < kSegments; ++segment) {
            const float a = -1.f + 2.f * segment / kSegments;
            const float b = -1.f + 2.f * (segment + 1) / kSegments;
            quad(
                {inner * a, -inner}, {inner * b, -inner}, {outer * a, -outer}, {outer * b, -outer}
            );
            quad({inner, inner * a}, {inner, inner * b}, {outer, outer * a}, {outer, outer * b});
            quad(
                {-inner * a, inner}, {-inner * b, inner}, {-outer * a, outer}, {-outer * b, outer}
            );
            quad(
                {-inner, -inner * a},
                {-inner, -inner * b},
                {-outer, -outer * a},
                {-outer, -outer * b}
            );
        }
        inner = outer;
    }
    const auto count = static_cast<unsigned>(m_sources.size()) - first;
    const auto weights = std::span(m_weights).subspan(m_ocean_linear.first, m_ocean_linear.count);
    const auto vertex_count = std::size_t{count} * weights.size();
    if (!plan.ocean || plan.vertices.size() != vertex_count) {
        plan.vertices.clear();
        plan.vertices.reserve(vertex_count);
        for (unsigned index = 0; index < count; ++index) {
            for (const auto& weight : weights) {
                plan.vertices.push_back({weight, index});
            }
        }
        plan.upload_needed = true;
    }
    plan.ocean = true;
    plan.topology_valid = false;
    plan.first_triangle = first;
    plan.last_use = ++m_use_serial;
    return {m_used++, vertex_count, 1};
}

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
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::WaterMeshCoarse)) {
        // Each shape again from its quantised edges, split to one unit instead of half a unit.
        std::vector<std::array<float, 2>> coarse = {{{0, 0}}, {{1, 0}}, {{0, 1}}};
        for (auto& [key, shape] : m_templates) {
            const std::array<std::array<double, 2>, 3> corners{
                {{0, 0}, {key[0] / 256., key[1] / 256.}, {key[2] / 256., key[3] / 256.}}
            };
            const auto first = static_cast<unsigned>(coarse.size());
            subdivide_shape(corners, {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}, 1., coarse, 0);
            shape = {first, static_cast<unsigned>(coarse.size()) - first};
        }
        log::info(
            "water mesh: {} baked vertices, {} at one unit (water-mesh-coarse)",
            m_weights.size(),
            coarse.size()
        );
        m_weights = std::move(coarse);
    }
}

float WaterMesh::rest_height(
    const renderer::EffectVertex& p, unsigned surface, std::vector<float>* heights
) const {
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
        if (heights) {
            heights->push_back(z);
        }
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
    m_cache_counts = {};
}

void WaterMesh::finish_pass() const {
    stats::gauge("water rest lookups", m_cache_counts[0]);
    stats::gauge("water rest cache hits", m_cache_counts[1]);
    stats::gauge("water topology builds", m_cache_counts[2]);
    stats::gauge("water topology cache hits", m_cache_counts[3]);
}

// Dense geometry depends on ordered XY positions. Rest height still follows the live layer.
renderer::EffectMesh WaterMesh::prepare(
    const renderer::EffectStrip& strip, std::span<const renderer::EffectVertex> vertices
) {
    const profiling::Zone trace("Water coarse instances");
    const bool reuse = renderer::render_opt::enabled(renderer::render_opt::Opt::WaterMeshReuse);
    const auto key = [surface = strip.surface](const renderer::EffectVertex& vertex) {
        return std::array{
            std::bit_cast<std::uint32_t>(vertex.x), std::bit_cast<std::uint32_t>(vertex.y), surface
        };
    };
    const auto unused = m_plans.begin() + static_cast<std::ptrdiff_t>(m_used);
    const auto match =
        reuse ? std::find_if(
                    unused,
                    m_plans.end(),
                    [&](const Plan& plan) {
                        return plan.topology_valid
                               && std::equal(
                                   plan.rest.begin(),
                                   plan.rest.end(),
                                   vertices.begin(),
                                   vertices.end(),
                                   [&](const Plan::RestLookup& lookup,
                                       const renderer::EffectVertex& vertex) {
                                       return lookup.valid && lookup.key == key(vertex);
                                   }
                               );
                    }
                )
              : m_plans.end();
    const bool topology_hit = match != m_plans.end();
    constexpr std::size_t kRetainedPlans = 256;
    if (topology_hit) {
        // Only unclaimed slots can move. Draw handles issued earlier in the pass remain valid.
        if (unused != match) {
            std::iter_swap(unused, match);
        }
    } else if (m_used == m_plans.size() || (reuse && m_plans.size() < kRetainedPlans)) {
        m_plans.emplace_back();
        if (m_used + 1 != m_plans.size()) {
            std::swap(m_plans[m_used], m_plans.back());
        }
    } else if (reuse) {
        const auto oldest =
            std::min_element(unused, m_plans.end(), [](const Plan& a, const Plan& b) {
                return a.last_use < b.last_use;
            });
        std::iter_swap(unused, oldest);
    }
    auto& plan = m_plans[m_used];
    plan.first_triangle = static_cast<unsigned>(m_sources.size());
    plan.last_use = ++m_use_serial;
    std::vector<renderer::EffectVertex> reference_rest;
    auto& rest = reuse ? m_rest_vertices : reference_rest;
    rest.assign(vertices.begin(), vertices.end());
    if (reuse) {
        plan.rest.resize(rest.size());
    }
    for (std::size_t i = 0; i < rest.size(); ++i) {
        auto& p = rest[i];
        if (reuse && std::isfinite(p.x) && std::isfinite(p.y)) {
            auto& lookup = plan.rest[i];
            const auto position = key(p);
            if (!lookup.valid || lookup.key != position) {
                lookup.key = position;
                lookup.heights.clear();
                p.z = rest_height(p, strip.surface, &lookup.heights);
                lookup.valid = true;
                ++m_cache_counts[0];
            } else {
                float best = std::numeric_limits<float>::max(), result = p.z;
                for (const auto z : lookup.heights) {
                    if (std::abs(z - p.z) < best) {
                        best = std::abs(z - p.z);
                        result = z;
                    }
                }
                p.z = result;
                ++m_cache_counts[1];
            }
        } else {
            if (reuse) {
                plan.rest[i].valid = false;
            }
            p.z = rest_height(p, strip.surface);
            ++m_cache_counts[0];
        }
    }
    constexpr unsigned orders[6][3] =
        {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    if (!topology_hit) {
        const profiling::Zone rebuild("Rebuild water strip mesh");
        ++m_cache_counts[2];
        plan.topology.clear();
        plan.vertices.clear();
        plan.ocean = false;
        plan.upload_needed = true;
        for (std::size_t k = 0; k + 2 < rest.size(); ++k) {
            const auto& p = rest[k];
            const auto& q = rest[k + 1];
            const auto& r = rest[k + 2];
            // Strip restart connectors have no visible triangle.
            if (std::abs((q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)) < 1e-8f) {
                continue;
            }
            Shape shape = m_ocean_shape.count ? m_ocean_shape : Shape{0, 3};
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
                    quantize(b.x - a.x),
                    quantize(b.y - a.y),
                    quantize(c.x - a.x),
                    quantize(c.y - a.y)
                };
                auto found = m_templates.find(key);
                if (found != m_templates.end()) {
                    shape = found->second;
                    order = candidate;
                    matched = true;
                    break;
                }
            }
            if (!matched && !m_missing_template && !m_ocean_shape.count) {
                log::warn(
                    "Water bake missing mesh shape for surface {}; using coarse triangle",
                    strip.surface
                );
                m_missing_template = true;
            }
            const auto triangle = static_cast<unsigned>(plan.topology.size());
            if (plan.vertices.size() + shape.count > 4 * 1024 * 1024) {
                throw std::runtime_error("water strip exceeds the vertex budget");
            }
            for (const auto& weights : std::span(m_weights).subspan(shape.first, shape.count)) {
                plan.vertices.push_back({weights, triangle});
            }
            plan.topology.push_back({k, {order[0], order[1], order[2]}});
        }
        plan.topology_valid = reuse;
    } else {
        ++m_cache_counts[3];
    }
    for (const auto& triangle : plan.topology) {
        const auto k = triangle.first;
        const auto& order = triangle.order;
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
    }
    const renderer::EffectMesh result{
        m_used++, plan.vertices.size(), plan.vertices.empty() ? 0u : 1u
    };
    stats::gauge("water coarse triangles", double(m_sources.size()));
    return result;
}
}  // namespace openrac::enhanced
