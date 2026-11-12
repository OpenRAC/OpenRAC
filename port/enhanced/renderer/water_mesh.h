// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Water strip geometry and its live coarse attributes.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <vector>

#include "renderer/effects.h"

namespace openrac::enhanced {
// Builds one resident vertex stream per strip from the baked subdivision.
class WaterMesh {
public:
    WaterMesh(const std::filesystem::path& bake, const std::array<char, 32>& fingerprint);
    // Builds a regular subdivision template for the camera-following ocean grid.
    WaterMesh();
    ~WaterMesh();
    WaterMesh(const WaterMesh&) = delete;
    WaterMesh& operator=(const WaterMesh&) = delete;

    // A range of baked barycentric coordinates in the shared resident vertex stream.
    struct Shape {
        unsigned first = 0, count = 0;
        bool operator==(const Shape&) const = default;
    };

    struct Vertex {
        std::array<float, 2> weights;
        std::uint32_t triangle;
        bool operator==(const Vertex&) const = default;
    };

    // Triangle order is preserved, with one draw for the entire strip.
    struct Plan {
        std::vector<Vertex> vertices;
        unsigned first_triangle = 0;
        unsigned buffer = 0;
        bool upload_needed = true;
        bool ocean = false;
        bool coarse = false;  // Built from the one-unit templates (water-mesh-coarse).
        std::uint64_t last_use = 0;

        struct RestLookup {
            std::array<std::uint32_t, 3> key{};  // Bit-exact XY and surface; Z stays live.
            std::vector<float> heights;          // Original candidate order preserves ties.
            bool valid = false;
        };

        struct Triangle {
            std::size_t first;
            std::array<unsigned, 3> order;
        };

        std::vector<RestLookup> rest;
        std::vector<Triangle> topology;
        bool topology_valid = false;
    };

    // Two texels per coarse vertex plus one animation record; the GPU fills each UV texel's ZW.
    using Source = std::array<std::array<std::uint32_t, 4>, 7>;

    // Starts one effect pass; storage is reused without retaining pointers into the frame snapshot.
    void begin_pass();
    // Resolves rest heights and geometry; animated UVs and colours never invalidate topology.
    renderer::EffectMesh prepare(
        const renderer::EffectStrip&, std::span<const renderer::EffectVertex>
    );
    renderer::EffectMesh prepare_ocean(
        const renderer::Vec3& eye, float height, float near_radius, float horizon
    );
    // Creates the vertex array and a valid dummy source view.
    bool init(std::string& error);
    // Uploads changed geometry and the collected coarse triangles.
    void upload();
    // Binds a valid unsigned source view even for the shader's original-material branch.
    void bind(const renderer::Shader&) const;
    // Draws an earlier strip plan through the shared effect shader.
    void draw(const renderer::Shader&, const renderer::EffectMesh&) const;
    // Releases GPU resources; the immutable bake remains available for reinitialisation.
    void release();
    void finish_pass() const;

    const std::array<unsigned, 4>& cache_counts() const { return m_cache_counts; }

    // Immutable views for CPU/GPU equivalence tests and mesh diagnostics.
    const Plan& plan(std::size_t handle) const { return m_plans.at(handle); }

    std::span<const Source> sources() const { return m_sources; }

    std::span<const std::array<float, 2>> weights() const { return m_weights; }

private:
    // The nearest authored layer at XY supplies static height instead of the legacy moving endpoints.
    float rest_height(
        const renderer::EffectVertex&, unsigned surface, std::vector<float>* heights = nullptr
    ) const;
    std::vector<renderer::EffectVertex> m_rest_vertices;
    std::array<unsigned, 4> m_cache_counts{};         // Height lookups/hits, topology builds/hits.
    std::map<std::array<int, 4>, Shape> m_templates;  // Immutable shape-to-resident-range lookup.
    std::map<std::array<int, 4>, Shape> m_coarse_templates;  // The same shapes at one unit.
    std::vector<std::array<float, 2>> m_weights;      // Baked dense coordinates; never regenerated.
    Shape m_ocean_shape{};                            // Optional shared ocean triangle template.
    Shape m_ocean_linear{};
    std::vector<Source> m_sources;                    // Owned coarse instances for this pass.
    std::vector<Plan> m_plans;                        // Reused strip-plan storage.
    std::size_t m_used = 0;                           // Plans collected during the current pass.
    std::uint64_t m_use_serial = 0;
    bool m_missing_template = false;  // Emit at most one unsupported-shape warning.
    unsigned m_vao = 0;
    unsigned m_source_buffer = 0, m_source_texture = 0;  // Dynamic coarse data and unsigned view.
    renderer::Shader m_animation;  // Animates coarse UVs once, before dense vertex interpolation.

    // An authored rest triangle retained independently of the camera's visible strips.
    struct RestTriangle {
        unsigned surface;
        std::array<renderer::Vec3, 3> p;
    };

    std::vector<RestTriangle> m_rest;  // Immutable authored layers.
    std::map<std::pair<int, int>, std::vector<unsigned>> m_rest_tiles;  // XY lookup into m_rest.
};
}  // namespace openrac::enhanced
