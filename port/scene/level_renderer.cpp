// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/level_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <stdexcept>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#include "scene/hero_shadow.h"
#include "scene/level_renderer_internal.h"
#include "scene/moby_passes.h"
#include "scene/shadow_atlas.h"
#include "scene_shaders.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::scene {

using namespace openrac::gl;
using renderer::Bucket;
using namespace render_detail;

namespace {

Bucket bucket_of(Layer layer) {
    switch (layer) {
        case Layer::Sky:
            return Bucket::Sky;
        case Layer::Terrain:
            return Bucket::Terrain;
        case Layer::Ties:
            return Bucket::Ties;
        case Layer::Shrubs:
            return Bucket::Shrubs;
        default:
            return Bucket::Mobys;
    }
}

// Returns the textures `materials` draw with, in their order: each one's image and emission image,
// `white` where it has none, as the layer and sky passes pick them (`textures` is by image).
std::vector<renderer::TextureHandle> material_textures(
    const std::vector<Material>& materials,
    const std::vector<renderer::TextureHandle>& textures,
    renderer::TextureHandle white
) {
    const auto of = [&](int image) {
        // A material without an image, or with one the level does not hold, draws white.
        return image >= 0 && static_cast<std::size_t>(image) < textures.size()
                   ? textures[static_cast<std::size_t>(image)]
                   : white;
    };
    std::vector<renderer::TextureHandle> used;
    used.reserve(materials.size() * 2 + 1);  // two a material, and white
    used.push_back(white);

    // Two a material: the colour image, then the emission image (white without one).
    for (const Material& material : materials) {
        used.push_back(of(material.image));
        used.push_back(of(material.emission_image));
    }

    return used;
}

}  // namespace

std::string LevelScene::deformation_vertex(std::string source) const {
    return m_lighting ? m_lighting->mesh_vertex(std::move(source)) : source;
}

bool LevelScene::deformation_active() const {
    return m_lighting && m_lighting->deformation_active();
}

// ---- Scene upload ----

bool LevelScene::upload(
    const LevelData& level, renderer::TexturePool& textures, std::string& error
) {
    // mesh.frag again with OPENRAC_SOLID, after its version line (which has to stay the first).
    // The registered lighting puts its GLSL into the mesh shader; without it the scene's own.
    const std::string mesh_vertex =
        m_lighting ? m_lighting->mesh_vertex(shaders::mesh_vert) : shaders::mesh_vert;
    const std::string fragment = m_lighting ? m_lighting->mesh_fragment(shaders::mesh_frag)
                                            : std::string(shaders::mesh_frag);
    m_early_capable = m_lighting && m_lighting->early_supported();
    std::string solid(fragment);
    solid.insert(solid.find('\n') + 1, "#define OPENRAC_SOLID 1\n");
    if (!m_mesh.build("mesh", mesh_vertex, fragment, error)
        || !m_solid.build("mesh (solid)", mesh_vertex, solid, error)
        || !m_sky.build("sky", shaders::sky_vert, shaders::sky_frag, error)) {
        return false;
    }
    if (m_early_capable) {
        std::string early = m_lighting->early_fragment(shaders::mesh_frag), early_solid(early);
        early_solid.insert(early_solid.find('\n') + 1, "#define OPENRAC_SOLID 1\n");
        if (!m_early_mesh.build("early lit mesh", mesh_vertex, early, error)
            || !m_early_solid.build("early lit solid", mesh_vertex, early_solid, error)) {
            return false;
        }
    }
#if defined(OPENRAC_VULKAN)
    if (vkgl::gpu_materials_active()) {
        const auto opaque_source = [](std::string source) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_OPAQUE_COLOUR 1\n");
            const auto effect = source.find("uniform int probe_effect;");
            if (effect != std::string::npos) {
                source.replace(
                    effect, std::strlen("uniform int probe_effect;"), "const int probe_effect = 0;"
                );
            }
            source.insert(source.find("void main() {"), "uniform int opaque_solid;\n");
            return source;
        };
        if (!m_colour_opaque
                 .build("opaque colour batch", mesh_vertex, opaque_source(fragment), error)
            || (m_early_capable
                && !m_early_colour_opaque.build(
                    "early opaque colour batch",
                    mesh_vertex,
                    opaque_source(m_lighting->early_fragment(shaders::mesh_frag)),
                    error
                ))) {
            return false;
        }
        // Keep opaque features constant per native program. In particular,
        // static geometry never needs the joint-palette path, and solid
        // geometry must not inherit the alpha shader's discard instructions.
        const auto fixed_int = [](std::string source, const char* name, int value) {
            const auto declaration = std::string("uniform int ") + name + ";";
            const auto at = source.find(declaration);
            if (at != std::string::npos) {
                source.replace(
                    at,
                    declaration.size(),
                    std::string("const int ") + name + " = " + std::to_string(value) + ";"
                );
            }
            return source;
        };
        for (unsigned variant = 0; variant < m_colour_specialised.size(); ++variant) {
            std::string vertex(mesh_vertex);
            vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
            vertex = fixed_int(vertex, "metal_pass", 0);
            if (variant < 2) {
                vertex = fixed_int(vertex, "skinning", 0);
            }
            const auto specialise = [&](std::string source) {
                source.insert(source.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
                if (variant == 0) {
                    source.insert(source.find('\n') + 1, "#define OPENRAC_SOLID 1\n");
                }
                for (const auto* name :
                     {"metal_pass",
                      "glow_pass",
                      "translucent_pass",
                      "effect_pass",
                      "probe_effect"}) {
                    source = fixed_int(std::move(source), name, 0);
                }
                return source;
            };
            const char* names[] =
                {"solid static colour", "alpha static colour", "opaque moby colour"};
            if (!m_colour_specialised[variant]
                     .build(names[variant], vertex, specialise(fragment), error)
                || (m_early_capable
                    && !m_early_colour_specialised[variant].build(
                        std::string("early ") + names[variant],
                        vertex,
                        specialise(m_lighting->early_fragment(shaders::mesh_frag)),
                        error
                    ))) {
                return false;
            }
        }
        std::string static_vertex(mesh_vertex), static_translucent(fragment);
        static_vertex
            .insert(static_vertex.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
        static_translucent
            .insert(static_translucent.find('\n') + 1, "#define OPENRAC_COLOUR_SPECIALISED 1\n");
        static_vertex = fixed_int(std::move(static_vertex), "skinning", 0);
        static_vertex = fixed_int(std::move(static_vertex), "metal_pass", 0);
        for (const auto* name : {"metal_pass", "glow_pass", "effect_pass", "probe_effect"}) {
            static_translucent = fixed_int(std::move(static_translucent), name, 0);
        }
        static_translucent = fixed_int(std::move(static_translucent), "translucent_pass", 1);
        if (!m_static_translucent
                 .build("static translucent colour", static_vertex, static_translucent, error)) {
            return false;
        }
    }
#endif
    const std::string depth_source = deformation_vertex(shaders::mesh_depth_vert);
    std::string depth_vertex(depth_source), depth_fragment(shaders::mesh_depth_frag);
    depth_vertex.insert(depth_vertex.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
    depth_fragment.insert(depth_fragment.find('\n') + 1, "#define OPENRAC_DEPTH_SOLID 1\n");
    if (!m_depth.build("mesh depth", depth_source, shaders::mesh_depth_frag, error)
        || !m_depth_solid.build("mesh depth solid", depth_vertex, depth_fragment, error)) {
        return false;
    }
    // Compatibility rendering retains the minimal depth-only shaders.
    if (gl::enhanced_rendering_available()) {
        const std::string define = "#define OPENRAC_DEPTH_NORMALS 1\n";
        std::string vertex(depth_source);
        std::string fragment(shaders::mesh_depth_frag);
        fragment.insert(fragment.find('\n') + 1, renderer::shaders::shading_normal_glsl);
        depth_fragment
            .insert(depth_fragment.find('\n') + 1, renderer::shaders::shading_normal_glsl);
        vertex.insert(vertex.find('\n') + 1, define);
        fragment.insert(fragment.find('\n') + 1, define);
        depth_vertex.insert(depth_vertex.find('\n') + 1, define);
        depth_fragment.insert(depth_fragment.find('\n') + 1, define);
        if (!m_surface_depth.build("mesh surface depth", vertex, fragment, error)
            || !m_surface_depth_solid
                    .build("mesh solid surface depth", depth_vertex, depth_fragment, error)) {
            return false;
        }
    }
    m_models = level.models;
    for (std::size_t k = 0; k < m_scenery.size(); ++k) {
        Scenery& scenery = m_scenery[k];
        const auto layer = static_cast<std::size_t>(kSceneryLayers[k]);
        const auto& distances = level.draw_distances[layer];
        const bool sized = distances.size() == level.instances[layer].size();
        const bool chosen = sized && !distances.empty();
        scenery.source = chosen ? level.instances[layer] : std::vector<Instance>{};
        scenery.distances = chosen ? distances : std::vector<float>{};
        scenery.centres.clear();
        for (const Instance& instance : scenery.source) {
            scenery.centres.push_back(
                instance.model < level.models.size()
                    ? bounds_centre(level.models[instance.model], instance.matrix)
                    : Vec3{instance.matrix[12], instance.matrix[13], instance.matrix[14]}
            );
        }
        scenery.drawn.clear();
    }
    // A glow packet's vertex (light slot -2, mesh.vert); an index past the vertices counts as one,
    // so that a broken primitive is still drawn.
    const auto glow_vertex = [&level](std::uint32_t i) {
        return i >= level.vertices.size() || level.vertices[i].light_slot < -1.5f;
    };
    // The primitives without a glow packet's vertex, which the mobys' glow pass would draw only to
    // discard every fragment (mesh.frag: v_glow != glow_pass): left out of it (draw_layer).
    for (Model& model : m_models) {
        for (Primitive& p : model.primitives) {
            if (static_cast<std::size_t>(p.first_index) + p.index_count > level.indices.size()) {
                continue;  // kept as drawn
            }
            p.glow_vertices = std::any_of(
                level.indices.begin() + p.first_index,
                level.indices.begin() + p.first_index + p.index_count,
                glow_vertex
            );
        }
    }
    m_materials = level.materials;
    m_light_sets = level.light_sets;
    renderer::Rgba8Image white(1, 1);
    white.set_texel(0, 0, 0xFFFFFFFFu);
    m_white = textures.add(std::move(white), renderer::AlphaScale::Full, "white");
    // The cutout materials' moby textures (the hologram's lettering, fences, foliage) with one
    // level, bilinear: the GS samples moby textures without mip levels (TEX1 MXL 0), and a mipmap
    // chain averages a cutout's alpha away at a distance (thin, broken letters).
    std::vector<bool> cutout_moby(level.images.size(), false);
    for (const Material& m : level.materials) {
        if (m.cutout && m.image >= 0 && static_cast<std::size_t>(m.image) < level.images.size()
            && level.image_paths[static_cast<std::size_t>(m.image)].find("/textures/moby_")
                   != std::string::npos) {
            cutout_moby[static_cast<std::size_t>(m.image)] = true;
        }
    }
    for (std::size_t i = 0; i < level.images.size(); ++i) {
        m_textures.push_back(
            level.images[i].width > 0 ? textures.add(
                                            level.images[i],
                                            renderer::AlphaScale::Full,
                                            level.image_paths[i],
                                            !cutout_moby[i],
                                            cutout_moby[i]
                                        )
                                      : m_white
        );
    }

    m_pvs = gl::is_vulkan() ? level.pvs : Pvs{};
    // The static layers' models again, their primitives in material order, for the opaque pass:
    // the level's terrain is one model of a primitive per fragment and material, two thousand
    // draws in a frame, and in material order a material's fragments are one run of indices,
    // one draw (draw_layer joins them). Depth decides what is in front in that pass, so the order
    // does not change the picture; the blended passes keep the stored order.
    std::vector<std::uint32_t> indices = level.indices;
    m_opaque.assign(m_models.size(), 0);
    std::vector<bool> fixed(m_models.size(), false);
    for (const Layer layer : {Layer::Terrain, Layer::Ties, Layer::Shrubs}) {
        for (const Instance& instance : level.instances[static_cast<std::size_t>(layer)]) {
            if (instance.model < fixed.size()) {
                fixed[instance.model] = true;
            }
        }
    }
    for (std::uint32_t m = 0; m < m_opaque.size(); ++m) {
        m_opaque[m] = m;
        const Model& model = m_models[m];
        if (!fixed[m] || model.primitives.size() < 2) {
            continue;
        }
        Model sorted = model;
        std::stable_sort(
            sorted.primitives.begin(),
            sorted.primitives.end(),
            [](const Primitive& a, const Primitive& b) { return a.material < b.material; }
        );
        for (Primitive& p : sorted.primitives) {
            const std::uint32_t first = static_cast<std::uint32_t>(indices.size());
            if (static_cast<std::size_t>(p.first_index) + p.index_count > level.indices.size()) {
                continue;
            }
            indices.insert(
                indices.end(),
                level.indices.begin() + p.first_index,
                level.indices.begin() + p.first_index + p.index_count
            );
            p.first_index = first;
        }
        m_opaque[m] = static_cast<std::uint32_t>(m_models.size());
        m_models.push_back(std::move(sorted));
    }

    // The glow primitives' triangles with a glow vertex, copied after the rest: a triangle without
    // one has v_glow 0 at its provoking vertex, whichever that is, so the glow pass discards all of
    // it (mesh.frag). The kept triangles keep their order, so what the pass draws is unchanged.
    for (Model& model : m_models) {
        for (Primitive& p : model.primitives) {
            const std::size_t end = static_cast<std::size_t>(p.first_index) + p.index_count;
            if (!p.glow_vertices || p.index_count % 3 != 0 || end > level.indices.size()) {
                continue;
            }
            const auto first = static_cast<std::uint32_t>(indices.size());
            for (std::size_t t = p.first_index; t < end; t += 3) {
                const auto triangle = level.indices.begin() + static_cast<std::ptrdiff_t>(t);
                if (std::any_of(triangle, triangle + 3, glow_vertex)) {
                    indices.insert(indices.end(), triangle, triangle + 3);
                }
            }
            const auto count = static_cast<std::uint32_t>(indices.size()) - first;
            // Every triangle glows: the primitive's own indices serve.
            if (count == p.index_count) {
                indices.resize(first);
                continue;
            }
            p.glow_first = first;
            p.glow_count = count;
        }
    }

    if (gl::is_vulkan()) {
        m_cpu_indices = indices;
        m_pvs_models.resize(m_models.size());
    }
    // A material's opaque flag alone does not guarantee coverage: vertex alpha
    // may still cut holes. Sorted copies use the same vertices and their new indices.
    m_shadow_solid_primitives.clear();
    for (const auto& model : m_models) {
        auto& eligible = m_shadow_solid_primitives.emplace_back();
        for (const auto& primitive : model.primitives) {
            bool solid =
                std::size_t(primitive.first_index) + primitive.index_count <= indices.size();
            for (std::size_t i = primitive.first_index;
                 solid && i < std::size_t(primitive.first_index) + primitive.index_count;
                 ++i) {
                const auto vertex = indices[i];
                solid = vertex < level.vertices.size()
                        && std::isfinite(level.vertices[vertex].colour[3])
                        && level.vertices[vertex].colour[3] >= 0.5f;
            }
            eligible.push_back(solid);
        }
    }

    m_static_shadow_ranges.clear();
    m_static_shadow_ranges.resize(m_models.size());
    m_static_shadow_coverage.assign(m_models.size(), 0);
    // Include every model a later static-layer instance update can select.
    for (const auto model : m_opaque) {
        for (unsigned solid = 0; solid < 2; ++solid) {
            m_static_shadow_ranges[model][solid] = prepare_shadow_draws(
                m_models[model].primitives,
                m_materials,
                m_shadow_solid_primitives[model],
                solid != 0
            );
        }
        for (const auto& range : m_static_shadow_ranges[model][1]) {
            m_static_shadow_coverage[model] |= range.solid ? 2 : 1;
        }
    }

    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(level.vertices.size() * sizeof(Vertex)),
        level.vertices.data(),
        GL_STATIC_DRAW
    );
    // Buffers have no type of their own: the indices are filled through the
    // array target, before any vertex array exists to hold the element binding.
    glGenBuffers(1, &m_indices);
    glBindBuffer(GL_ARRAY_BUFFER, m_indices);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
        indices.data(),
        GL_STATIC_DRAW
    );

    if (!level.light_colours.empty()) {
        const std::size_t rows = (level.light_colours.size() + kLightWidth - 1) / kLightWidth;
        std::vector<std::uint32_t> texels(rows * kLightWidth, 0u);
        std::copy(level.light_colours.begin(), level.light_colours.end(), texels.begin());
        glGenTextures(1, &m_lights);
        glBindTexture(GL_TEXTURE_2D, m_lights);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            kLightWidth,
            static_cast<GLsizei>(rows),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            texels.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    for (std::size_t layer = 0; layer < kLayerCount; ++layer) {
        build_groups(static_cast<Layer>(layer), level.instances[layer], false);
    }
    m_moby_layout.valid = false;
    ++m_static_generation;
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // The level's probe bake, found and uploaded by the registered lighting.
    if (m_lighting) {
        m_lighting->load(level);
    }

    // The materials' textures made now, while the level loads, so that its first frames make
    // none (texture-prepare, docs/port/RENDER_PERF.md); off, each is made at its first draw.
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::TexturePrepare)) {
        constexpr double kMiB = 1024.0 * 1024.0;
        const renderer::PreparedTextures made =
            textures.prepare(material_textures(m_materials, m_textures, m_white));
        log::verbose(
            "level textures: {} made at the load, {:.1f} MiB uploaded, {:.1f} MiB with mip chains, "
            "{} batches",
            made.textures,
            static_cast<double>(made.staged_bytes) / kMiB,
            static_cast<double>(made.resident_bytes) / kMiB,
            made.batches
        );
    }

    return true;
}

// ---- Instance groups ----

// ---- World drawing ----

void LevelScene::draw_mobys(
    const std::vector<Instance>& instances,
    const renderer::FrameInput& input,
    renderer::RenderState& state
) {
    auto& groups = m_groups[static_cast<std::size_t>(Layer::Mobys)];
    auto& idle = m_idle[static_cast<std::size_t>(Layer::Mobys)];
    std::vector<Group> own;
    own.swap(groups);
    build_groups(Layer::Mobys, instances, true);
    draw_layer(Layer::Mobys, input, state);
    idle.insert(
        idle.end(), std::make_move_iterator(groups.begin()), std::make_move_iterator(groups.end())
    );
    groups.swap(own);
}

void LevelScene::release() {
#if defined(OPENRAC_VULKAN)
    // The registered static instances go with the level (once the frames reading them retire).
    vkgl::gpu_release_instances();
#endif
    m_motion_records.release();
    m_motion_records_ready = false;
    m_motion_words.data().clear();
    m_motion_batches.clear();
    m_motion_groups.clear();
    for (auto& map : m_sun_maps) {
        release_target(map);
    }
    m_sun_enabled = m_sun_ready = false;
    glDeleteVertexArrays(1, &m_shadow_compact.vao);
    glDeleteBuffers(1, &m_shadow_compact.instances);
    m_shadow_compact = {};
    m_shadow_survivors.clear();
    m_sun_culling = {};
    release_target(m_hero_map);
    m_hero_depth.release();
    m_shadow_solid.release();
    m_atlas_depth.release();
    m_atlas_solid.release();
    release_target(m_shadow_atlas);
    m_shadow_atlas_active = false;
    release_target(m_static_shadow);
    m_static_keys = {};
    release_scrolling_shadows();
    m_moby_layout.valid = false;
    for (auto& bucket : m_shadow_draws) {
        bucket.clear();
    }
    m_shadow_solid_primitives.clear();
    m_static_shadow_ranges.clear();
    m_static_shadow_coverage.clear();
    m_hero_enabled = m_hero_ready = false;
    m_hero_logged_airborne.reset();
    m_hero_transition = {};
    m_hero_centre.reset();
    m_wetness_hero.reset();
    // Only registered lighting has GL objects to free.
    if (m_lighting) {
        m_lighting->release();
    }
    m_surface_depth.release();
    m_surface_depth_solid.release();
    m_early_capable = false;
    if (m_palette != 0) {
        glDeleteTextures(1, &m_palette);
        m_palette = 0;
    }
    m_palette_rows = 0;
    for (auto* all : {&m_groups, &m_idle}) {
        for (auto& groups : *all) {
            for (Group& g : groups) {
                glDeleteVertexArrays(1, &g.vao);
                glDeleteBuffers(1, &g.instances);
                glDeleteVertexArrays(1, &g.camera_vao);
                glDeleteBuffers(1, &g.camera_instances);
                glDeleteVertexArrays(1, &g.pvs_vao);
                glDeleteBuffers(1, &g.pvs_instances);
            }
            groups.clear();
        }
    }
    if (m_vertices != 0) {
        glDeleteBuffers(1, &m_vertices);
        m_vertices = 0;
    }
    if (m_indices != 0) {
        glDeleteBuffers(1, &m_indices);
        m_indices = 0;
    }
    if (m_lights != 0) {
        glDeleteTextures(1, &m_lights);
        m_lights = 0;
    }
    m_mesh.release();
    m_static_translucent.release();
    m_colour_opaque.release();
    m_early_colour_opaque.release();
    for (auto& shader : m_colour_specialised) {
        shader.release();
    }
    for (auto& shader : m_early_colour_specialised) {
        shader.release();
    }
    m_early_mesh.release();
    m_early_solid.release();
    m_solid.release();
    m_depth.release();
    m_depth_solid.release();
    m_camera_survivors.clear();
    m_camera_culling = {};
    m_pvs = {};
    for (auto& model : m_pvs_models) {
        glDeleteBuffers(1, &model.indices);
    }
    m_pvs_models.clear();
    m_cpu_indices.clear();
    // Nothing to choose from until the next level (choose_scenery).
    m_scenery = {};
    m_sky.release();
}

void LevelScene::set_fog(
    const std::array<float, 3>& colour, float near_depth, float far_depth, float near_f, float far_f
) {
    // UpdateViewContext's terms (renderer/world/fog.h): F = depth x slope + offset, clamped to
    // [far F, near F]. Equal depths or no fog asked for (both F 255) leave it off.
    const float range = far_depth - near_depth;
    if (!(range > 0.0f) || (near_f >= 255.0f && far_f >= 255.0f)) {
        m_fog_colour = {colour[0], colour[1], colour[2], 0.0f};
        return;
    }
    const float slope = (far_f - near_f) / range;
    const float offset = (near_f * far_depth - far_f * near_depth) / range;
    m_fog_colour = {colour[0], colour[1], colour[2], 1.0f};
    m_fog_params = {slope, offset, std::min(near_f, far_f), std::max(near_f, far_f)};
}

void LevelScene::draw_sky(const renderer::FrameInput& input, renderer::RenderState& state) {
    // The shells sit around the camera and never move with it: the view's
    // rotation only. Each is blended over the last, without depth.
    renderer::Mat4 rotation = input.camera.view;
    rotation[12] = rotation[13] = rotation[14] = 0.0f;
    const renderer::Mat4 view_projection = renderer::multiply(input.camera.projection, rotation);
    m_sky.use();
    glUniformMatrix4fv(m_sky.uniform("view_projection"), 1, GL_FALSE, view_projection.data());
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glActiveTexture(GL_TEXTURE0);
    for (const Group& g : m_groups[static_cast<std::size_t>(Layer::Sky)]) {
        glBindVertexArray(g.vao);
        for (const Primitive& p : m_models[g.model].primitives) {
            const Material& m = m_materials[static_cast<std::size_t>(p.material)];
            const renderer::TextureHandle texture =
                m.image >= 0 ? m_textures[static_cast<std::size_t>(m.image)] : m_white;
            state.textures.bind_upscaled_texture_colours(
                texture, m_sky, input.texture_filter, !m.clamp_s, !m.clamp_t
            );
            // The shell's matrix as the game drew it this tick; its translation is in the game's
            // sky units, the vertices were exported in 1/1024ths of them (editor/sky.py).
            renderer::Mat4 shell = renderer::identity();
            if (p.shell >= 0 && p.shell < 8 && (input.sky_shell_mask & (1u << p.shell)) != 0) {
                shell = input.sky_shells[static_cast<std::size_t>(p.shell)];
                shell[3] = shell[7] = shell[11] = 0.0f;
                shell[12] /= 1024.0f;
                shell[13] /= 1024.0f;
                shell[14] /= 1024.0f;
                shell[15] = 1.0f;
            }
            glUniformMatrix4fv(m_sky.uniform("shell"), 1, GL_FALSE, shell.data());
            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(p.index_count),
                GL_UNSIGNED_INT,
                offset(p.first_index * sizeof(std::uint32_t)),
                g.count
            );
            ++state.stats.draw_calls;
            state.stats.triangles += p.index_count / 3;
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

bool LevelScene::batches_opaque_colour() const {
#if defined(OPENRAC_VULKAN)
    static const bool& off = stats::probe(
        "OPENRAC_OPAQUE_COLOUR_BATCH_OFF", "Reference: opaque colour in per-layer order"
    );
    return !off && m_colour_opaque.id() && gpu_driven() && vkgl::gpu_materials_active();
#else
    return false;
#endif
}

bool LevelScene::colour_blend_barrier(Layer layer) const {
    // Mobys may have glow, effects or metal passes. Static cutout materials
    // also draw their translucent texels after their opaque texels.
    if (layer == Layer::Mobys) {
        return true;
    }
    for (const auto& group : m_groups[static_cast<std::size_t>(layer)]) {
        if (!group.count) {
            continue;
        }
        for (const auto& primitive : m_models[group.model].primitives) {
            const auto& material = m_materials[static_cast<std::size_t>(primitive.material)];
            if (material.cutout || material.metal || group.blend) {
                return true;
            }
        }
    }
    return false;
}

void LevelScene::draw_layer(
    Layer layer, const renderer::FrameInput& input, renderer::RenderState& state
) {
    if (m_lighting) {
        m_lighting->prepare_deformation(input);
    }
    if (layer == Layer::Mobys && m_lighting) {
        m_lighting->prepare_wetness(input, m_palette, m_wetness_hero);
    }

    if (layer == Layer::Sky) {
        const stats::Scope pass("sky colour", stats::kGpu);
        draw_sky(input, state);
        return;
    }
    if (layer == Layer::Terrain && !state.depth_prepass && !state.world_shadows_prepared) {
        prepare_shadows(input, state);
        state.world_shadows_prepared = true;
    }
    const bool colour_batch = !state.depth_prepass && batches_opaque_colour()
                              && (state.opaque_colour_phase >= 0 || state.opaque_colour_batched);
    const bool reuse_camera = colour_batch && state.opaque_colour_phase != 0;
    const stats::Scope colour(
        layer_name(layer),
        state.depth_prepass || colour_batch || layer == Layer::Mobys || layer == Layer::Ties
            ? 0
            : stats::kGpu
    );
    auto& groups = m_groups[static_cast<std::size_t>(layer)];
    auto& camera_counts = m_camera_culling[static_cast<std::size_t>(layer)];
    if (!reuse_camera) {
        camera_counts = {};
    }
    if (groups.empty()) {
        return;
    }
    const renderer::Mat4 view_projection =
        renderer::multiply(input.camera.projection, input.camera.view);
    // The static groups the camera does not see are left out: a level's ties and shrubs are
    // hundreds of draws, most of them behind or beside the camera.
    const auto sides = frustum_sides(view_projection);
    std::array<std::array<float, 4>, 6> camera_planes;
    std::copy(sides.begin(), sides.end(), camera_planes.begin());
    for (unsigned k = 0; k < 4; ++k) {
        camera_planes[4][k] = view_projection[k * 4 + 3] + view_projection[k * 4 + 2];
        camera_planes[5][k] = view_projection[k * 4 + 3] - view_projection[k * 4 + 2];
    }
    static const bool& camera_culling_off = stats::probe(
        "OPENRAC_CAMERA_INSTANCE_CULL_OFF", "Profiling reference: aggregate-group camera culling"
    );
    auto& visible = m_colour_visible[static_cast<std::size_t>(layer)];
    if (!reuse_camera) {
        visible.assign(groups.size(), true);
    }
    static const bool& pvs_off =
        stats::probe("OPENRAC_PVS_OFF", "Profiling reference: disable baked camera visibility");
    const auto pvs_mask = pvs_off ? Pvs::all_visible() : m_pvs.select(input.camera.position);
    const bool use_pvs = !pvs_off && !m_pvs.grid.empty();
    const bool gpu = gpu_driven();
    renderer::GpuCullView gpu_view;
    gpu_view.planes = camera_planes;
    stats::gauge("gpu driven mesh", gpu ? 1 : 0);
    gpu_view.plane_count = camera_culling_off ? 0u : 6u;
    gpu_view.pvs = pvs_mask;
    gpu_view.use_pvs = use_pvs;
    std::uint64_t pvs_rejected = 0, pvs_primitives_rejected = 0;
    if (!reuse_camera) {
        for (std::size_t i = 0; i < groups.size(); ++i) {
            visible[i] = !groups[i].bounded || !outside(sides, groups[i].low, groups[i].high);
            auto& group = groups[i];
            group.gpu_camera = group.gpu_pvs = {};
            if (gpu && !visible[i]) {
                camera_counts.candidates += group.count;
                camera_counts.rejected += group.count;
                group.pvs_count = group.count;
                group.camera_count = 0;
                continue;
            }
            if (gpu && visible[i]) {
                group.gpu_camera = gpu_batch(group, gpu_view);
                group.pvs_count = group.camera_count = group.count;
                camera_counts.candidates += group.count;
                camera_counts.submitted +=
                    group.count;  // candidate upper bound; GPU counts are asynchronous
                group.camera_valid = group.pvs_valid = false;
                continue;
            }
            if (!use_pvs) {
                if (!group.pvs_valid || group.pvs_mask != pvs_mask) {
                    group.camera_valid = false;
                }
                group.pvs_count = group.count;
                group.pvs_mask = pvs_mask;
                group.pvs_valid = true;
            } else if (!group.pvs_valid || group.pvs_mask != pvs_mask) {
                m_pvs_survivors.clear();
                for (std::size_t n = 0; n < group.cpu_instances.size(); ++n) {
                    if (group.cpu_instances[n].moby_light[3] > 1.5f
                        || Pvs::visible(pvs_mask, group.occlusion[n])) {
                        m_pvs_survivors.push_back(group.cpu_instances[n]);
                    }
                }
                group.pvs_count = static_cast<int>(m_pvs_survivors.size());
                if (group.pvs_count && group.pvs_count != group.count) {
                    Group compact;
                    if (!group.pvs_vao) {
                        compact = make_group(group.model);
                        group.pvs_vao = compact.vao;
                        group.pvs_instances = compact.instances;
                    } else {
                        compact.vao = group.pvs_vao;
                        compact.instances = group.pvs_instances;
                    }
                    fill_group(compact, m_pvs_survivors, GL_STREAM_DRAW);
                }
                group.pvs_mask = pvs_mask;
                group.pvs_valid = true;
                group.camera_valid = false;
            }
            pvs_rejected += group.count - group.pvs_count;
            camera_counts.candidates += group.count;
            if (!visible[i]) {
                camera_counts.rejected += group.count;
                continue;
            }
            if (camera_culling_off) {
                camera_counts.submitted += group.pvs_count;
                camera_counts.rejected += group.count - group.pvs_count;
                continue;
            }
            if (!group.camera_valid || group.camera_clip != view_projection) {
                m_camera_survivors.clear();
                for (std::size_t n = 0; n < group.cpu_instances.size(); ++n) {
                    const auto& bounds = group.caster_bounds[n];
                    const bool ui = group.cpu_instances[n].moby_light[3] > 1.5f;
                    if (use_pvs && !ui && !Pvs::visible(pvs_mask, group.occlusion[n])) {
                        continue;
                    }
                    if (bounds.valid && !ui && outside(camera_planes, bounds.low, bounds.high)) {
                        continue;
                    }
                    m_camera_survivors.push_back(group.cpu_instances[n]);
                }
                group.camera_count = static_cast<int>(m_camera_survivors.size());
                if (group.camera_count && group.camera_count != group.count) {
                    Group compact;
                    if (!group.camera_vao) {
                        compact = make_group(group.model);
                        group.camera_vao = compact.vao;
                        group.camera_instances = compact.instances;
                    } else {
                        compact.vao = group.camera_vao;
                        compact.instances = group.camera_instances;
                    }
                    fill_group(compact, m_camera_survivors, GL_STREAM_DRAW);
                }
                group.camera_clip = view_projection;
                group.camera_valid = true;
            }
            camera_counts.submitted += group.camera_count;
            camera_counts.rejected += group.count - group.camera_count;
            for (const auto& bounds : group.caster_bounds) {
                camera_counts.unbounded += !bounds.valid;
            }
        }
    }
    if (stats::enabled()) {
        const auto name = std::string("camera ") + layer_name(layer);
        stats::gauge(name + " submitted", static_cast<double>(camera_counts.submitted));
        stats::gauge(name + " culled", static_cast<double>(camera_counts.rejected));
    }
    // On a target wider than the game's picture, the 2D layer's mobys are cut at the picture's sides
    // (mesh.vert).
    // An ES driver without GL_EXT_clip_cull_distance has no clip planes (renderer/shader.h, es_source).
    const bool cut = layer == Layer::Mobys && state.target.picture_scale_x < 1.0f
                     && (!gl::is_es() || gl::has_extension("GL_EXT_clip_cull_distance"));
    if (cut) {
        glEnable(GL_CLIP_DISTANCE0);
        glEnable(GL_CLIP_DISTANCE1);
    }
    // The joint palettes (live mobys): unit 1; the textures go on unit 0.
    const bool palettes = layer == Layer::Mobys && m_palette != 0;
    // The frame's uniforms, on the program the next passes draw with: the solid one for the static
    // layers' opaque materials without a cutout (mesh.frag, OPENRAC_SOLID), the general one else.
    const auto use = [&](const renderer::Shader& shader, bool depth_matched) {
        shader.use();
        glUniformMatrix4fv(shader.uniform("view_projection"), 1, GL_FALSE, view_projection.data());
        glUniform1f(shader.uniform("picture_scale"), state.target.picture_scale_x);
        if (state.depth_prepass) {
            glUniformMatrix4fv(shader.uniform("view"), 1, GL_FALSE, input.camera.view.data());
            glUniform1i(shader.uniform("skinning"), palettes ? 1 : 0);
            glUniform1i(shader.uniform("palette_texture"), static_cast<GLint>(kPaletteUnit));
            glUniform1i(shader.uniform("light_texture"), static_cast<GLint>(kLightUnit));
            glUniform1i(shader.uniform("tex_T0"), 0);
            return;
        }
        glUniform1i(shader.uniform("gtao_enabled"), state.gtao_visibility != 0 ? 1 : 0);
        glUniform1i(shader.uniform("gtao_visibility"), 11);
        if (state.gtao_visibility != 0) {
            glActiveTexture(GL_TEXTURE0 + 11);
            glBindTexture(GL_TEXTURE_2D, state.gtao_visibility);
            glActiveTexture(GL_TEXTURE0);
        }
        // The terrain's vertex colours are the game's own lighting (the extractor lights it as
        // LightTfrags does): it gets no viewer sun.
        glUniform1i(shader.uniform("lighting"), lighting && layer != Layer::Terrain ? 1 : 0);
        glUniform1i(shader.uniform("skinning"), palettes ? 1 : 0);
        glUniform1i(shader.uniform("palette_texture"), static_cast<GLint>(kPaletteUnit));
        glUniform1i(shader.uniform("light_texture"), static_cast<GLint>(kLightUnit));
        if (m_light_sets.size() == 16 * 16) {
            glUniform4fv(shader.uniform("light_sets"), 64, m_light_sets.data());
        }
        std::array<float, 32> point_positions{}, point_colours{};
        for (std::size_t i = 0; i < 8; ++i) {
            std::copy_n(m_point_lights.data() + i * 8, 4, point_colours.data() + i * 4);
            std::copy_n(m_point_lights.data() + i * 8 + 4, 4, point_positions.data() + i * 4);
        }
        glUniform4fv(shader.uniform("probe_point_position"), 8, point_positions.data());
        glUniform4fv(shader.uniform("probe_point_colour"), 8, point_colours.data());
        // The registered lighting's uniforms; without it the shader keeps the game's lighting.
        if (m_lighting) {
            m_lighting->bind(
                shader, layer, m_hero_enabled || m_sun_enabled, &input.camera, depth_matched
            );
        }
        if (m_lighting) {
            m_lighting->bind_wetness(shader);
        }
        bind_sun_shadow(shader, input.camera);
        // The early lighting's result, where registered lighting drew one.
        if (m_lighting) {
            m_lighting->bind_early(shader, input, state);
        }
        bind_hero_shadow(shader);
        glUniform4f(
            shader.uniform("fog_colour"),
            m_fog_colour[0],
            m_fog_colour[1],
            m_fog_colour[2],
            m_fog_colour[3]
        );
        glUniform4f(
            shader.uniform("fog_params"),
            m_fog_params[0],
            m_fog_params[1],
            m_fog_params[2],
            m_fog_params[3]
        );
        glUniformMatrix4fv(shader.uniform("view"), 1, GL_FALSE, input.camera.view.data());
        glUniform1i(shader.uniform("effect_pass"), 0);
        glUniform1i(shader.uniform("metal_pass"), 0);
    };
    glActiveTexture(GL_TEXTURE0 + kLightUnit);
    glBindTexture(GL_TEXTURE_2D, m_lights);
    if (palettes || colour_batch) {
        glActiveTexture(GL_TEXTURE0 + kPaletteUnit);
        glBindTexture(GL_TEXTURE_2D, m_palette);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);  // reversed depth
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);  // the editor's materials are double-sided
    glActiveTexture(GL_TEXTURE0);
    // Mobys in two passes: their solid faces, then their glow faces blended over them as the GS
    // blends them (ALPHA 0x44: (Cs - Cd) As + Cd), without writing depth.
    // Then the translucent texels of cutout materials, blended the same way, without depth.
    // Mobys: a fourth pass for those MobyProc blends (Instance::blend), then a fifth for the metal
    // packets (the shine pass), after everything else.
    // The static layers: their opaque materials without a cutout (solid), then the opaque texels
    // of the cutout ones, then those cutouts' translucent texels.
    const bool mobys = layer == Layer::Mobys;
    const int passes = mobys ? 5 : 3;
    static constexpr const char* moby_pass_names[] =
        {"mobys opaque", "mobys glow", "mobys translucent", "mobys effects", "mobys metal"};
    // The material state the program holds now (flush): none yet.
    int set_material = -1;
    renderer::TextureHandle set_texture = 0;  // bound on unit 0 (and its colours) for `program`
    int set_wrap = -1;                        // and its clamp: bit 0 across, bit 1 down
    unsigned set_emission_texture = 0;
    int set_cutout = -1;
    decltype(Material::colour) set_colour{};
    bool colour_set = false;
    const renderer::Shader* program = nullptr;
    bool program_depth_matched = false;
    // The mobys' glow pass runs twice (scene/moby_passes.h): its texels at or above the GS's alpha
    // reference first, writing depth, then the rest without.
    const int steps = mobys ? static_cast<int>(kMobyPassSteps.size()) : passes;
    for (int step = 0; step < steps; ++step) {
        const MobyPassStep moby_step =
            mobys ? kMobyPassSteps[static_cast<std::size_t>(step)] : MobyPassStep{};
        const int pass = mobys ? moby_step.pass : step;
        const int glow_split = moby_step.glow_split;
        if (state.depth_prepass && (mobys ? pass > 0 : pass > 1)) {
            break;
        }
        if (colour_batch) {
            const bool opaque = mobys ? pass == 0 : pass <= 1;
            if (state.opaque_colour_phase >= 0) {
                if (!opaque || (state.opaque_colour_phase == 0) != (!mobys && pass == 0)) {
                    continue;
                }
            } else if (opaque) {
                continue;
            }
        }
        const bool solid = !mobys && pass == 0;
        const bool reused =
            state.reuse_prepass_depth && !state.depth_prepass && (mobys ? pass == 0 : pass <= 1);
        std::optional<stats::Scope> layer_pass;
        if (layer == Layer::Ties && !state.depth_prepass && state.opaque_colour_phase < 0) {
            static constexpr const char* names[] =
                {"ties solid", "ties cutout", "ties translucent"};
            layer_pass.emplace(names[pass], stats::kGpu, std::source_location::current());
        }
        if (mobys && !state.depth_prepass && state.opaque_colour_phase < 0) {
            layer_pass.emplace(moby_pass_names[pass], stats::kGpu, std::source_location::current());
        }
        // Chrome overlays reuse the same cached GI so they preserve gameplay edge reflections.
        const bool early = !state.depth_prepass
                           && (mobys ? pass == kMobySolid || pass == kMobyMetal : pass <= 1)
                           && m_lighting && m_lighting->early_matches(input, state);
        const bool batch_shader = colour_batch && state.opaque_colour_phase >= 0;
        static const bool& specialisation_off = stats::probe(
            "OPENRAC_OPAQUE_SPECIALISE_OFF", "Reference: unspecialised mesh colour shaders"
        );
        const unsigned variant = solid ? 0 : mobys ? 2 : 1;
        const auto& specialised = early ? m_early_colour_specialised : m_colour_specialised;
        const bool static_translucent = !state.depth_prepass && !mobys && pass == 2
                                        && m_static_translucent.id() && !specialisation_off;
        const renderer::Shader* selected = &m_mesh;
        // Both depth variants retain identical positions and alpha coverage.
        if (state.depth_prepass) {
            // Early lighting and AO request smooth normals alongside depth.
            if (state.target.normal_texture != 0) {
                selected = solid ? &m_surface_depth_solid : &m_surface_depth;
            } else {
                selected = solid ? &m_depth_solid : &m_depth;
            }
        } else if (static_translucent) {
            // Static blended packets omit moby-only vertex work.
            selected = &m_static_translucent;
        } else if (batch_shader && !specialisation_off) {
            // Opaque variants specialise alpha handling for each geometry kind.
            selected = &specialised[variant];
        } else if (batch_shader) {
            // The reference batch keeps dynamic alpha handling.
            selected = early ? &m_early_colour_opaque : &m_colour_opaque;
        } else if (early) {
            // Opaque colour consumes the cached compute lighting.
            selected = solid ? &m_early_solid : &m_early_mesh;
        } else {
            // Blended surfaces and unsupported early passes retain fragment lighting.
            selected = solid ? &m_solid : &m_mesh;
        }

        const renderer::Shader& wanted = *selected;
        const bool depth_matched = mobys ? pass == 0 : pass <= 1;
        if (program != &wanted || program_depth_matched != depth_matched) {
            use(wanted, depth_matched);
            program = &wanted;
            program_depth_matched = depth_matched;
            set_material = -1;
            set_texture = 0;
            set_wrap = -1;
            set_cutout = -1;
            colour_set = false;
        }
        const int cutout = program->uniform("cutout");
        const int colour = program->uniform("material_colour");
        glUniform1i(program->uniform("opaque_solid"), solid ? 1 : 0);
        // The cutout materials' opaque texels (static layers).
        const bool cutouts = !mobys && pass == 1;
        // The blended mobys' pass: their whole models, all texels, at their alpha (blend 1:
        // ALPHA 0x44, (Cs - Cd) As + Cd) or added (blend 2: 0x48, Cs As + Cd), depth tested and
        // not written, as ReRAC draws MobyProc's translucent and additive groups.
        const bool effects = mobys && pass == 3;
        // The metal packets: blended over the moby with ALPHA 0x44, depth tested and not written,
        // pulled a little toward the camera (MobyProc adds 1000 to their GS Z; mesh.vert).
        const bool metal = mobys && pass == 4;
        const bool translucent = pass == 2;
        glDepthFunc(reused ? GL_EQUAL : GL_GEQUAL);
        glDepthMask(reused ? GL_FALSE : GL_TRUE);
        glUniform1i(program->uniform("metal_pass"), metal ? 1 : 0);
        glUniform1i(program->uniform("glow_pass"), mobys && pass == 1 ? 1 : 0);
        glUniform1i(program->uniform("translucent_pass"), translucent ? 1 : 0);
        glUniform1i(program->uniform("effect_pass"), effects ? 1 : 0);
        if (mobys ? pass >= 1 : pass >= 2) {
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            glDepthMask(mobys && moby_step.depth_write ? GL_TRUE : GL_FALSE);
        }
        glUniform2f(
            program->uniform("gameplay_rim_lift"), m_gameplay_rim_lift[0], m_gameplay_rim_lift[1]
        );
        glUniform1i(program->uniform("glow_split"), glow_split);
        glUniform2f(
            program->uniform("gameplay_rim_parameters"), m_gameplay_rim[0], m_gameplay_rim[1]
        );
        for (std::size_t gi = 0; gi < groups.size(); ++gi) {
            Group& g = groups[gi];
            if (!visible[gi] || (!metal && (g.blend != 0) != effects)) {
                continue;
            }
            const bool culled = !camera_culling_off && (mobys ? !metal : pass <= 1);
            const int instances = culled ? g.camera_count : g.pvs_count;
            if (!instances) {
                continue;
            }
            if (m_lighting) {
                m_lighting->bind_deformation(*program, g.floats_on_water);
            }
            if (effects) {
                glBlendFuncSeparate(
                    GL_SRC_ALPHA, g.blend == 2 ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE
                );
            }
            glUniform1i(program->uniform("probe_effect"), g.blend == 2 ? 1 : 0);
            glUniform1i(program->uniform("hero_shadow_receiver"), g.hero_shadow_caster ? 1 : 0);
            glUniform1i(
                program->uniform("gameplay_rim_enabled"),
                mobys && g.gameplay_essential && m_gameplay_rim[0] > 0 ? 1 : 0
            );
            glBindVertexArray(instances == g.count ? g.vao : culled ? g.camera_vao : g.pvs_vao);
            auto batch = g.gpu_camera;
            if (batch && !culled) {
                if (!g.gpu_pvs) {
                    auto broad = gpu_view;
                    broad.plane_count = 0;
                    g.gpu_pvs = gpu_batch(g, broad);
                }
                batch = g.gpu_pvs;
            }
            // Primitives that follow one another in the index buffer with one material are one
            // draw: the same triangles in the same order, in one call instead of several.
            std::uint32_t run_first = 0;
            std::uint32_t run_count = 0;
            int run_material = -1;
            const auto flush = [&] {
                if (run_count == 0) {
                    return;
                }
                // The material's state is set only when it is not the last draw's: a model's parts
                // mostly share a few, and each call into the driver costs more than the test.
                if (!(state.depth_prepass && solid) && run_material != set_material) {
                    const Material& m = m_materials[static_cast<std::size_t>(run_material)];
                    if (enhanced_rendering_available()) {
                        glUniform3f(
                            program->uniform("material_emission"),
                            m.emission[0],
                            m.emission[1],
                            m.emission[2]
                        );
                        glUniform1i(
                            program->uniform("emission_vertex_colour"),
                            m.emission_vertex_colour ? 1 : 0
                        );
                        glUniform1f(program->uniform("material_diffuse_scale"), m.diffuse_scale);
                        glUniform1f(
                            program->uniform("material_emission_contrast"), m.emission_contrast
                        );
                        glUniform1i(program->uniform("emission_texture"), 12);
                        const auto emission_texture =
                            m.emission_image >= 0
                                ? m_textures[static_cast<std::size_t>(m.emission_image)]
                                : m_white;
                        const unsigned gl_emission = state.textures.gl_texture(emission_texture);
                        if (gl_emission != set_emission_texture) {
                            glActiveTexture(GL_TEXTURE0 + 12);
                            glBindTexture(GL_TEXTURE_2D, gl_emission);
                            glActiveTexture(GL_TEXTURE0);
                            set_emission_texture = gl_emission;
                        }
                    }
                    const renderer::TextureHandle texture =
                        m.image >= 0 ? m_textures[static_cast<std::size_t>(m.image)] : m_white;
                    // Materials share textures: rebinding one the program already samples costs
                    // driver calls for nothing. A texture's clamp is its material's.
                    const int wrap = (m.clamp_s ? 1 : 0) | (m.clamp_t ? 2 : 0);
                    if (texture != set_texture || wrap != set_wrap) {
                        state.textures.bind_upscaled_texture_colours(
                            texture, *program, input.texture_filter, !m.clamp_s, !m.clamp_t
                        );
                        set_texture = texture;
                        set_wrap = wrap;
                    }
                    if (static_cast<int>(m.cutout) != set_cutout) {
                        glUniform1i(cutout, m.cutout ? 1 : 0);
                        set_cutout = static_cast<int>(m.cutout);
                    }
                    if (!colour_set || m.colour != set_colour) {
                        glUniform4f(colour, m.colour[0], m.colour[1], m.colour[2], m.colour[3]);
                        set_colour = m.colour;
                        colour_set = true;
                    }
                    set_material = run_material;
                }
                submit_instances(batch, run_first, run_count, instances);
                ++state.stats.draw_calls;
                state.stats.triangles += static_cast<std::uint64_t>(run_count / 3)
                                         * static_cast<std::uint64_t>(instances);
                run_count = 0;
            };
            // The opaque pass of a static layer in material order (m_opaque).
            const std::uint32_t model =
                pass <= 1 && !mobys && g.model < m_opaque.size() ? m_opaque[g.model] : g.model;
            const auto* primitives = &m_models[model].primitives;
            bool compact_indices = false;
            if (use_pvs && layer == Layer::Terrain) {
                auto& cached = m_pvs_models[model];
                if (!cached.valid || cached.mask != pvs_mask) {
                    cached.primitives.clear();
                    cached.rejected = 0;
                    std::vector<std::uint32_t> visible_indices;
                    for (const auto& primitive : *primitives) {
                        if (!Pvs::visible(pvs_mask, primitive.occlusion)) {
                            ++cached.rejected;
                            continue;
                        }
                        auto kept = primitive;
                        kept.first_index = static_cast<std::uint32_t>(visible_indices.size());
                        visible_indices.insert(
                            visible_indices.end(),
                            m_cpu_indices.begin() + primitive.first_index,
                            m_cpu_indices.begin() + primitive.first_index + primitive.index_count
                        );
                        cached.primitives.push_back(kept);
                    }
                    if (cached.rejected && !visible_indices.empty()) {
                        if (!cached.indices) {
                            glGenBuffers(1, &cached.indices);
                        }
                        glBindBuffer(GL_ARRAY_BUFFER, cached.indices);
                        glBufferData(
                            GL_ARRAY_BUFFER,
                            static_cast<GLsizeiptr>(visible_indices.size() * sizeof(std::uint32_t)),
                            visible_indices.data(),
                            GL_DYNAMIC_DRAW
                        );
                        glBindBuffer(GL_ARRAY_BUFFER, 0);
                    }
                    cached.mask = pvs_mask;
                    cached.valid = true;
                }
                pvs_primitives_rejected += cached.rejected;
                if (cached.rejected) {
                    primitives = &cached.primitives;
                    compact_indices = !primitives->empty();
                    if (compact_indices) {
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cached.indices);
                    }
                }
            }
            for (const Primitive& p : *primitives) {
                const bool is_metal = m_materials[static_cast<std::size_t>(p.material)].metal;
                if (is_metal != metal) {
                    continue;
                }
                // The glow pass draws a glow packet's faces only: the rest would be discarded.
                if (mobys && pass == kMobyGlow && !p.glow_vertices
                    && renderer::render_opt::enabled(renderer::render_opt::Opt::MobyGlowSkip)) {
                    continue;
                }
                const bool is_cutout = m_materials[static_cast<std::size_t>(p.material)].cutout;
                if ((translucent || cutouts) && !is_cutout) {
                    continue;
                }
                if (solid && is_cutout) {
                    continue;
                }
                // Only its glowing triangles there, from the level's own index buffer (a
                // PVS-compacted one holds the whole primitives alone).
                std::uint32_t first = p.first_index;
                std::uint32_t count = p.index_count;
                if (mobys && pass == kMobyGlow && p.glow_count != 0 && !compact_indices
                    && renderer::render_opt::enabled(renderer::render_opt::Opt::MobyGlowSkip)) {
                    first = p.glow_first;
                    count = p.glow_count;
                }
                if (run_count != 0 && (p.material == run_material || (state.depth_prepass && solid))
                    && first == run_first + run_count) {
                    run_count += count;
                    continue;
                }
                flush();
                run_first = first;
                run_count = count;
                run_material = p.material;
            }
            flush();
            if (compact_indices) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);
            }
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_GEQUAL);
    glBindVertexArray(0);
    if (stats::enabled()) {
        const auto name = std::string("pvs ") + layer_name(layer);
        stats::gauge(name + " instances culled", static_cast<double>(pvs_rejected));
        stats::gauge(
            name + " primitive passes culled", static_cast<double>(pvs_primitives_rejected)
        );
        stats::gauge("pvs enabled", !pvs_off && !m_pvs.grid.empty() ? 1 : 0);
    }
    if (cut) {
        glDisable(GL_CLIP_DISTANCE0);
        glDisable(GL_CLIP_DISTANCE1);
    }
    if (palettes && !colour_batch) {
        glActiveTexture(GL_TEXTURE0 + kPaletteUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }
}

LayerRenderer::LayerRenderer(LevelScene& scene, Layer layer)
    : BucketRenderer(layer_name(layer), bucket_of(layer)),
      m_scene(scene),
      m_layer(layer) {}

void LayerRenderer::render(const renderer::FrameInput& input, renderer::RenderState& state) {
    m_scene.draw_layer(m_layer, input, state);
    if (m_layer == Layer::Mobys) {
        m_scene.draw_probe_spheres(input, state);
    }
}

void LayerRenderer::render_depth(const renderer::FrameInput& input, renderer::RenderState& state) {
    if (m_layer != Layer::Sky) {
        m_scene.draw_layer(m_layer, input, state);
    }
}

}  // namespace openrac::scene
