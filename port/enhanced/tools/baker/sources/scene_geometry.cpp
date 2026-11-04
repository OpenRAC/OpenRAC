// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Converts exported meshes to two-sided transport, preserving authored colour and emission.

#include "enhanced/tools/baker/sources/scene_geometry.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "enhanced/tools/baker/sources/level_scene.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {

// Copies an exported RGB or position triple.
Float3 as_float3(const float* v) {
    return {v[0], v[1], v[2]};
}

// One scene conversion, owning texture remapping while borrowing the input and output for this call.
class GeometryExporter {
public:
    // Keeps first-use texture order stable across all layers and instances.
    GeometryExporter(const scene::LevelData& level, BakeSceneExport& out)
        : m_level(level),
          m_out(out),
          m_texture_indices(level.images.size(), -1) {}

    // Visits terrain, ties, shrubs, static mobys and authored bake surfaces in source order.
    void run() {
        // The final layer contains bake-only proxy surfaces rather than level instances.
        for (unsigned layer = 0; layer < 5; ++layer) {
            const auto id = static_cast<std::size_t>(scene::Layer::Terrain) + layer;
            const bool bake_surface = layer == 4;

            for (const auto& instance :
                 bake_surface ? m_level.bake_surfaces : m_level.instances[id]) {
                append_instance(instance, layer);
            }
        }
    }

private:
    // Copies a texture on first use; missing texture indices keep the white fallback.
    int texture_index(int image_index) {
        // Untextured primitives retain the existing white material fallback.
        if (image_index < 0) {
            return -1;
        }

        const auto image = static_cast<std::size_t>(image_index);
        auto& texture = m_texture_indices.at(image);

        // Shared images are copied once at their first encounter.
        if (texture < 0) {
            const auto& source = m_level.images.at(image);
            texture = static_cast<int>(m_out.scene.textures.size());
            m_out.scene.textures.push_back(
                {static_cast<std::uint32_t>(source.width),
                 static_cast<std::uint32_t>(source.height),
                 source.pixels}
            );
        }

        return texture;
    }

    // Resolves the original static light slot or unlit moby multiplier before authored tint.
    Float3 emission_colour(
        const scene::Instance& instance, const scene::Vertex& input, bool moby
    ) const {
        Float3 colour = as_float3(input.colour);

        // Static light-slot colours replace mesh colours in the original renderer.
        if (!moby && instance.lights >= 0 && input.light_slot >= 0) {
            const auto at = static_cast<std::size_t>(instance.lights)
                            + static_cast<std::size_t>(input.light_slot);
            const auto packed = m_level.light_colours.at(at);
            for (unsigned c = 0; c < 3; ++c) {
                colour[c] = static_cast<float>((packed >> (8 * c)) & 255) / 128;
            }
        }

        // Instance tint remains an encoded GS colour multiplier in runtime and transport.
        for (unsigned c = 0; c < 3; ++c) {
            colour[c] *= instance.tint[c];
        }
        return colour;
    }

    // Copies authored vertex attributes without consulting unreliable mesh normals.
    void vertex_attributes(
        BakeTriangle& triangle,
        const scene::Instance& instance,
        const scene::Vertex& input,
        unsigned k,
        bool moby
    ) {
        // Terrain vertex colour remains an authored albedo input.
        if (triangle.terrain_prelighting) {
            for (unsigned c = 0; c < 3; ++c) {
                triangle.authored_colours[k][c] = input.colour[c] * instance.tint[c];
            }
        }

        // Ignore input.normal, even if inverted or nonfinite.
        // Embree derives transport normals from these positions.
        triangle.vertices[k] =
            renderer::transform_point(instance.matrix, as_float3(input.position));
        triangle.uv[k] = {input.uv[0], input.uv[1]};

        // Some authored emission explicitly includes vertex colour.
        if (triangle.emission_vertex_colour) {
            triangle.emission_colours[k] = emission_colour(instance, input, moby);
        }

        // Moby packets carry per-vertex colour multipliers.
        if (moby) {
            for (unsigned c = 0; c < 4; ++c) {
                triangle.multipliers[k][c] = input.colour[c] * instance.tint[c];
            }
        }
    }

    // Rejects non-emissive glow and degenerate faces before extending receiver bounds.
    void append_triangle(BakeTriangle& triangle, bool glow, bool emits) {
        // Unconfigured additive glow has no static transport surface.
        if (glow && !emits) {
            ++m_out.skipped_glow;
            return;
        }

        // Emissive glow does not reflect incoming diffuse light.
        if (glow) {
            triangle.diffuse = {};
        }

        const auto& a = triangle.vertices[0];
        const auto& b = triangle.vertices[1];
        const auto& c = triangle.vertices[2];
        const auto n = renderer::cross(
            Float3{b[0] - a[0], b[1] - a[1], b[2] - a[2]},
            Float3{c[0] - a[0], c[1] - a[1], c[2] - a[2]}
        );

        // Squared cross-product area below this threshold is degenerate.
        if (renderer::dot(n, n) < 1e-16f) {
            ++m_out.skipped_degenerate;
            return;
        }

        for (const auto& p : triangle.vertices) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                // Invalid positions cannot enter bounds or Embree geometry.
                if (!std::isfinite(p[axis])) {
                    throw std::runtime_error("nonfinite static scene position");
                }

                m_out.bounds_min[axis] = std::min(m_out.bounds_min[axis], p[axis]);
                m_out.bounds_max[axis] = std::max(m_out.bounds_max[axis], p[axis]);
            }
        }

        m_out.scene.triangles.push_back(triangle);
        m_out.emissive_triangles += emits ? 1 : 0;
    }

    // Converts a primitive while preserving lazy emission-texture lookup and triangle order.
    void append_primitive(
        const scene::Instance& instance, const scene::Primitive& primitive, unsigned layer
    ) {
        const auto& material = m_level.materials.at(static_cast<std::size_t>(primitive.material));

        // Blended and reflective packets are excluded from static transport.
        if (material.metal || material.blend || material.colour[3] < 0.99f) {
            ++m_out.skipped_blended;
            return;
        }

        const int texture = texture_index(material.image);
        Float3 emission = material.emission;

        // The bake uses an absolute override, never a second multiplier on runtime intensity.
        if (material.bake_intensity) {
            emission.fill(*material.bake_intensity);
        }

        const bool emits = *std::max_element(emission.begin(), emission.end()) > 0;

        // Each primitive must contain complete index triples.
        if (primitive.index_count % 3) {
            throw std::runtime_error("non-triangle bake primitive");
        }

        const bool moby = layer == 3;

        for (std::size_t j = 0; j < primitive.index_count; j += 3) {
            BakeTriangle triangle;
            triangle.terrain_prelighting = layer == 0;
            triangle.texture = texture;
            triangle.emission = emission;
            triangle.visible_emission = material.emission;
            triangle.fixed_emission = material.bake_intensity.has_value();
            triangle.emission_texture = texture_index(material.emission_image);
            triangle.emission_vertex_colour = material.emission_vertex_colour;
            triangle.emission_contrast = material.emission_contrast;
            triangle.diffuse = as_float3(material.colour.data());
            triangle.diffuse_scale = material.diffuse_scale;
            // Masked packets use half alpha; other opaque packets discard near-zero alpha.
            triangle.alpha_cutoff = material.cutout ? 0.5f : 0.02f;
            bool glow = false;

            for (unsigned k = 0; k < 3; ++k) {
                const auto vertex = m_level.indices.at(primitive.first_index + j + k);
                const auto& input = m_level.vertices.at(vertex);
                vertex_attributes(triangle, instance, input, k, moby);

                // Negative light slots identify additive glow packets.
                if (moby) {
                    glow |= input.light_slot < -1.5f;
                }
            }

            append_triangle(triangle, glow, emits);
        }
    }

    // Includes only explicitly selected rigid mobys; every accepted instance retains its statistics.
    void append_instance(const scene::Instance& instance, unsigned layer) {
        const auto& model = m_level.models.at(instance.model);
        const bool moby = layer == 3;

        // Moving, skinned or fading objects cannot be fixed occluders for a static bake.
        if (moby
            && (!instance.bake_static || instance.stand_in || model.joints > 0
                || instance.palette >= 0 || instance.blend != 0 || instance.tint[3] < .99f)) {
            ++m_out.skipped_mobys;
            return;
        }

        // Bake-only surfaces have a separate counter from the four exported render layers.
        if (layer == 4) {
            ++m_out.surface_instances;
        } else {
            ++m_out.instances[layer];
        }

        for (const auto& primitive : model.primitives) {
            append_primitive(instance, primitive, layer);
        }
    }

    const scene::LevelData& m_level;     // Input assets outlive this conversion.
    BakeSceneExport& m_out;              // Caller-owned transport and statistics.
    std::vector<int> m_texture_indices;  // First-use image-to-transport remapping.
};

}  // namespace

void append_probe_geometry(const scene::LevelData& level, BakeSceneExport& out) {
    GeometryExporter(level, out).run();
}

}  // namespace openrac::lighting
