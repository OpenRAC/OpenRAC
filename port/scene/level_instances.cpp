// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <map>
#include <stdexcept>

#include "assets/geometry/shrub_lighting.h"
#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#include "scene/hero_shadow.h"
#include "scene/level_renderer.h"
#include "scene/level_renderer_internal.h"
#include "scene/moby_passes.h"
#include "scene/shadow_atlas.h"
#include "scene_shaders.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::scene {
using namespace openrac::gl;
using namespace render_detail;

// Points attribute 0 of the bound vertex array at the positions: the tight buffer of their own
// (vertex-position-stream), so that depth-only passes and a tile GPU's binning read 12 bytes a
// vertex, or the wide vertex.
void LevelScene::point_positions() const {
    if (m_position_stream) {
        glBindBuffer(GL_ARRAY_BUFFER, m_positions);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), offset(0));
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offset(offsetof(Vertex, position))
        );
    }
}

// Follows vertex-position-stream when it is switched while a level is loaded (the Graphics sweep,
// the equivalence test): every vertex array made so far is pointed again.
void LevelScene::sync_position_stream() {
    const bool wanted =
        m_positions != 0
        && renderer::render_opt::enabled(renderer::render_opt::Opt::VertexPositionStream);
    if (wanted == m_position_stream) {
        return;
    }
    m_position_stream = wanted;
    const auto point = [this](unsigned vao) {
        if (vao != 0) {
            glBindVertexArray(vao);
            point_positions();
        }
    };
    for (const auto* all : {&m_groups, &m_idle}) {
        for (const auto& groups : *all) {
            for (const Group& g : groups) {
                point(g.vao);
                point(g.camera_vao);
                point(g.pvs_vao);
            }
        }
    }
    point(m_shadow_compact.vao);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// A class model's vertex array: the level's vertices and indices, and an instance buffer of its own
// (empty until fill_group).
LevelScene::Group LevelScene::make_group(std::uint32_t model) {
    Group g;
    g.model = model;
    glGenVertexArrays(1, &g.vao);
    glBindVertexArray(g.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);
    constexpr auto kStride = static_cast<GLsizei>(sizeof(Vertex));
    glEnableVertexAttribArray(0);
    point_positions();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, colour)));
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 4, GL_UNSIGNED_BYTE, kStride, offset(offsetof(Vertex, joints)));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, light_slot)));
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, weights)));
    glGenBuffers(1, &g.instances);
    glBindBuffer(GL_ARRAY_BUFFER, g.instances);
    constexpr auto kInstanceStride = static_cast<GLsizei>(sizeof(InstanceData));
    for (GLuint column = 0; column < 4; ++column) {
        glEnableVertexAttribArray(4 + column);
        glVertexAttribPointer(
            4 + column,
            4,
            GL_FLOAT,
            GL_FALSE,
            kInstanceStride,
            offset(std::size_t{column} * 4 * sizeof(float))
        );
        glVertexAttribDivisor(4 + column, 1);
    }
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(
        8, 4, GL_FLOAT, GL_FALSE, kInstanceStride, offset(offsetof(InstanceData, tint))
    );
    glVertexAttribDivisor(8, 1);
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(
        9, 1, GL_FLOAT, GL_FALSE, kInstanceStride, offset(offsetof(InstanceData, palette))
    );
    glVertexAttribDivisor(9, 1);
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(
        13, 1, GL_FLOAT, GL_FALSE, kInstanceStride, offset(offsetof(InstanceData, lights))
    );
    glVertexAttribDivisor(13, 1);
    glEnableVertexAttribArray(14);
    glVertexAttribPointer(
        14, 4, GL_FLOAT, GL_FALSE, kInstanceStride, offset(offsetof(InstanceData, moby_light))
    );
    glVertexAttribDivisor(14, 1);
    glEnableVertexAttribArray(15);
    glVertexAttribPointer(
        15, 4, GL_FLOAT, GL_FALSE, kInstanceStride, offset(offsetof(InstanceData, moby_ambient))
    );
    glVertexAttribDivisor(15, 1);
    return g;
}

// A group's instances, sent to its buffer. `usage`: GL_STATIC_DRAW for the level's own, sent once;
// GL_STREAM_DRAW for the live ones, sent again every frame (the driver then gives the buffer new
// storage rather than wait for the frame still drawing from the old one).
void LevelScene::fill_group(Group& g, const std::vector<InstanceData>& list, unsigned usage) {
    g.count = static_cast<int>(list.size());
    glBindBuffer(GL_ARRAY_BUFFER, g.instances);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(list.size() * sizeof(InstanceData)),
        list.data(),
        usage
    );
}

// Returns whether every instance of `instances` is opaque and lit as scenery, so that the opaque
// shadow shader (no texture, no discard) may draw the group's opaque faces.
bool LevelScene::solid_shadow_instances(const std::vector<InstanceData>& instances) {
    return std::all_of(instances.begin(), instances.end(), [](const InstanceData& data) {
        return std::isfinite(data.tint[3]) && data.tint[3] >= 1.0f && data.moby_light[3] <= 0.5f;
    });
}

// Returns OPENRAC_MOBY_SPHERE_CULL_OFF's switch: animated mobys left unbounded.
bool LevelScene::moby_sphere_culling_off() {
    static const bool& off = stats::probe(
        "OPENRAC_MOBY_SPHERE_CULL_OFF",
        "Profiling reference: leave animated mobys unbounded (next upload)"
    );
    return off;
}

// Returns whether the palette texture's texels `sent` (`texels` of them) are `matrices` followed
// by zeros, bit for bit: what set_palette would send again.
bool LevelScene::same_palette(
    const std::vector<JointColumns>& matrices,
    const std::vector<JointColumns>& sent,
    std::size_t texels
) {
    // Another size: another texture's worth.
    if (sent.size() != texels || matrices.size() > texels) {
        return false;
    }

    const std::size_t bytes = matrices.size() * sizeof(JointColumns);

    // The joints themselves.
    if (std::memcmp(matrices.data(), sent.data(), bytes) != 0) {
        return false;
    }

    // The rest of the last row, zeros when it was sent.
    const JointColumns zero{};
    return std::all_of(
        sent.begin() + static_cast<std::ptrdiff_t>(matrices.size()),
        sent.end(),
        [&](const JointColumns& texel) { return std::memcmp(&texel, &zero, sizeof(zero)) == 0; }
    );
}

// Returns the key build_groups groups `instance` by: its class model, its blend and whether it
// is a gameplay subject and casts the hero's shadow.
std::uint32_t LevelScene::group_key(const Instance& instance) {
    return instance.model << 6 | (instance.floats_on_water ? 32u : 0u)
           | (instance.gameplay_essential ? 16u : 0u)
           | (static_cast<std::uint32_t>(instance.blend & 7) << 1)
           | (instance.hero_shadow_caster ? 1u : 0u);
}

// Returns `instance` as the vertex shader's per-instance attributes take it.
LevelScene::InstanceData LevelScene::instance_data(const Instance& instance) {
    InstanceData data{};
    std::copy(instance.matrix.begin(), instance.matrix.end(), data.matrix);
    std::copy(instance.tint.begin(), instance.tint.end(), data.tint);
    data.palette = static_cast<float>(instance.palette);
    data.lights = static_cast<float>(instance.lights);
    std::copy(instance.moby_light.begin(), instance.moby_light.end(), data.moby_light);
    std::copy(instance.moby_ambient.begin(), instance.moby_ambient.end(), data.moby_ambient);
    return data;
}

// Returns the world bounds of one instance of `shape` with `data`: the shape's box through the
// instance's matrix for a rigid mesh, its world `sphere` for an animated one (unless `sphere_off`),
// padded; invalid (unbounded) when either cannot be trusted.
LevelScene::CasterBounds LevelScene::instance_bounds(
    const Model& shape,
    const InstanceData& data,
    const std::array<float, 4>& sphere,
    bool sphere_off,
    bool floats_on_water
) {
    CasterBounds bounds;
    bounds.low.fill(3e38f);
    bounds.high.fill(-3e38f);
    bounds.valid = data.palette < 0 && shape.min[0] <= shape.max[0];
    renderer::Mat4 matrix;
    std::copy_n(data.matrix, 16, matrix.begin());
    bounds.valid &= matrix[3] == 0 && matrix[7] == 0 && matrix[11] == 0 && matrix[15] == 1;
    for (int corner = 0; corner < 8 && bounds.valid; ++corner) {
        const auto p = renderer::transform_point(
            matrix,
            {(corner & 1) ? shape.max[0] : shape.min[0],
             (corner & 2) ? shape.max[1] : shape.min[1],
             (corner & 4) ? shape.max[2] : shape.min[2]}
        );
        for (int k = 0; k < 3; ++k) {
            bounds.valid &= std::isfinite(p[k]) && shape.min[k] <= shape.max[k];
            bounds.low[k] = std::min(bounds.low[k], p[k]);
            bounds.high[k] = std::max(bounds.high[k], p[k]);
        }
    }
    if (data.palette >= 0 && !sphere_off && data.moby_light[3] <= 1.5f) {
        bounds.valid = sphere[3] > 0 && std::all_of(sphere.begin(), sphere.end(), [](float x) {
                           return std::isfinite(x);
                       });
        for (int k = 0; k < 3 && bounds.valid; ++k) {
            bounds.low[k] = sphere[k] - sphere[3];
            bounds.high[k] = sphere[k] + sphere[3];
            bounds.valid &= std::isfinite(bounds.low[k]) && std::isfinite(bounds.high[k]);
        }
    }
    if (bounds.valid) {
        for (int k = 0; k < 3; ++k) {
            const float pad =
                .001f + 1e-6f * std::max(std::abs(bounds.low[k]), std::abs(bounds.high[k]));
            bounds.low[k] -= pad;
            bounds.high[k] += pad;
        }
    }
    if (floats_on_water && bounds.valid) {
        // Enclose any rigid tilt about the anchor, plus the maximum wave height.
        float radius_squared = 0;
        for (int k = 0; k < 3; ++k) {
            const float extent = std::max(
                std::abs(bounds.low[k] - data.matrix[12 + k]),
                std::abs(bounds.high[k] - data.matrix[12 + k])
            );
            radius_squared += extent * extent;
        }
        const float radius = std::sqrt(radius_squared);
        for (int k = 0; k < 3; ++k) {
            const float extent = radius + (k == 2 ? 1.f : 0.f);
            bounds.low[k] = data.matrix[12 + k] - extent;
            bounds.high[k] = data.matrix[12 + k] + extent;
        }
    }
    return bounds;
}

// Returns `bounds` as the GPU culling takes them, with the instance's PVS word and its flags
// (valid box, UI layer).
renderer::GpuCullBounds LevelScene::gpu_cull_bounds(
    const CasterBounds& bounds, const InstanceData& data, std::uint16_t occlusion
) {
    return {
        bounds.low,
        bounds.high,
        occlusion,
        (bounds.valid ? 1u : 0u) | (data.moby_light[3] > 1.5f ? 2u : 0u)
    };
}

void LevelScene::build_groups(Layer layer, const std::vector<Instance>& instances, bool live) {
    // Instances grouped by class model and blend (Instance::blend), in first-seen order. The lists
    // are kept between calls (a live layer is grouped again every frame).
    for (auto& group : m_grouping) {
        group.instances.clear();
        group.floating_ids.clear();
        group.spheres.clear();
        group.occlusion.clear();
    }
    const bool sphere_off = moby_sphere_culling_off();
    std::size_t used = 0;
    m_group_of_key.clear();

    // Each instance into the list of its key, a new list for a key not seen yet (first-seen order).
    for (const Instance& instance : instances) {
        const std::uint32_t key = group_key(instance);
        const auto [found, fresh] = m_group_of_key.try_emplace(key, used);
        const std::size_t at = found->second;

        // A key not seen yet this call: the next list, made the first time it is needed.
        if (fresh) {
            if (used == m_grouping.size()) {
                m_grouping.emplace_back();
            }
            m_grouping[used++].key = key;
        }

        m_grouping[at].instances.push_back(instance_data(instance));
        if (instance.floats_on_water) {
            m_grouping[at].floating_ids.push_back(instance.temporal_id);
        }
        m_grouping[at].spheres.push_back(instance.world_sphere);
        m_grouping[at].occlusion.push_back(instance.occlusion);
    }
    auto& groups = m_groups[static_cast<std::size_t>(layer)];
    auto& idle = m_idle[static_cast<std::size_t>(layer)];
    for (std::size_t i = 0; i < used; ++i) {
        const std::uint32_t model = m_grouping[i].key >> 6;
        // A live layer draws the same classes frame after frame: each keeps its vertex array and
        // buffer, and only the instances are sent again.
        auto kept = std::find_if(idle.begin(), idle.end(), [&](const Group& g) {
            return g.model == model;
        });
        Group g;
        if (kept != idle.end()) {
            g = std::move(*kept);
            if (kept != idle.end() - 1) {
                *kept = std::move(idle.back());
            }
            idle.pop_back();
        } else {
            g = make_group(model);
        }
        g.blend = static_cast<int>((m_grouping[i].key >> 1) & 7);
        g.hero_shadow_caster = (m_grouping[i].key & 1) != 0;
        g.gameplay_essential = (m_grouping[i].key & 16) != 0;
        g.floats_on_water = (m_grouping[i].key & 32) != 0;
        fill_group(g, m_grouping[i].instances, live ? GL_STREAM_DRAW : GL_STATIC_DRAW);
        g.cpu_instances = m_grouping[i].instances;
        g.floating_ids = m_grouping[i].floating_ids;
        g.shadow_solid_instances = solid_shadow_instances(g.cpu_instances);
        g.camera_valid = false;
        g.pvs_valid = false;
        g.occlusion = m_grouping[i].occlusion;
        g.caster_bounds.clear();
        g.gpu_bounds.clear();
        g.caster_bounds.reserve(g.cpu_instances.size());
        const bool native_gpu_bounds = gl::is_vulkan();
        if (native_gpu_bounds) {
            g.gpu_bounds.reserve(g.cpu_instances.size());
        }
        // Rigid meshes keep their tighter native boxes. Animated meshes use the
        // game's current world sphere, never their bind-pose box. Missing or
        // malformed spheres remain unbounded. Refresh on every instance upload.
        for (std::size_t n = 0; n < g.cpu_instances.size(); ++n) {
            CasterBounds bounds = instance_bounds(
                m_models[model],
                g.cpu_instances[n],
                m_grouping[i].spheres[n],
                sphere_off,
                g.floats_on_water
            );
            g.caster_bounds.push_back(bounds);
            if (native_gpu_bounds) {
                g.gpu_bounds.push_back(gpu_cull_bounds(bounds, g.cpu_instances[n], g.occlusion[n]));
            }
        }
        g.shadow_geometry_bounds.clear();
        if (layer != Layer::Mobys && g.cpu_instances.size() == 1 && !g.floats_on_water) {
            for (const auto& chunk : m_shadow_geometry[m_opaque[g.model]]) {
                Model shape;
                shape.min = chunk.low;
                shape.max = chunk.high;
                auto bound = instance_bounds(shape, g.cpu_instances[0], {}, true, false);
                bound.valid &= chunk.bounded;
                g.shadow_geometry_bounds.push_back(bound);
            }
        }

        // Aggregate the same conservative bounds used by per-instance tests. Animated scenery
        // rebuilds these each frame, so it can use the normal group-level camera rejection too.
        g.bounded = false;
        if ((!live || layer == Layer::Animated)
            && std::all_of(
                g.caster_bounds.begin(), g.caster_bounds.end(), [](const CasterBounds& b) {
                    return b.valid;
                }
            )) {
            constexpr float kHuge = 3.0e38f;
            g.low = {kHuge, kHuge, kHuge};
            g.high = {-kHuge, -kHuge, -kHuge};
            for (const auto& bound : g.caster_bounds) {
                for (int k = 0; k < 3; ++k) {
                    g.low[k] = std::min(g.low[k], bound.low[k]);
                    g.high[k] = std::max(g.high[k], bound.high[k]);
                }
            }
            g.bounded = g.count > 0;
        }

#if defined(OPENRAC_VULKAN)
        // A static group's instances change only when the layer is set again: registered then, a
        // view's GPU batch of them sends only the view (gpu-resident-instances, RENDER_PERF.md).
        if (!live && native_gpu_bounds
            && renderer::render_opt::enabled(renderer::render_opt::Opt::GpuResidentInstances)) {
            vkgl::gpu_register_instances(g.instances, g.gpu_bounds);
        }
#endif

        groups.push_back(std::move(g));
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

renderer::GpuInstanceBatch LevelScene::gpu_batch(
    const Group& group, const renderer::GpuCullView& view
) {
    static_assert(sizeof(InstanceData) == 120);
#if defined(OPENRAC_VULKAN)
    return vkgl::gpu_instances(group.instances, group.gpu_bounds, view);
#else
    (void)group;
    (void)view;
    return {};
#endif
}

void LevelScene::submit_instances(
    renderer::GpuInstanceBatch batch, unsigned first, unsigned count, int instances
) {
    // The settings sweep's experiment: the draw is kept, its triangles are not.
    if (m_sweep_one_triangle) {
        count = std::min(count, 3u);
    }
#if defined(OPENRAC_VULKAN)
    if (batch) {
        if (!vkgl::draw_gpu_instances(batch, first, count)) {
            throw std::runtime_error("GPU instance batch expired before draw submission");
        }
        return;
    }
#else
    (void)batch;
#endif
    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(count),
        GL_UNSIGNED_INT,
        offset(first * sizeof(std::uint32_t)),
        instances
    );
}

void LevelScene::set_variant(int number) {
    if (m_variant_image < 0 || number < 0
        || static_cast<std::size_t>(number) >= m_variants.size()) {
        return;
    }
    m_textures[static_cast<std::size_t>(m_variant_image)] =
        m_variants[static_cast<std::size_t>(number)];
}

void LevelScene::set_instances(Layer layer, const std::vector<Instance>& source) {
#ifdef OPENRAC_LIGHT_AUTHORING
    if (m_lighting) {
        m_lighting->prepare_instances(layer, source);
    }
    std::vector<Instance> drawable;
    const bool has_emitters =
        std::any_of(source.begin(), source.end(), [](const auto& i) { return i.lighting_only; });
    if (has_emitters) {
        std::copy_if(source.begin(), source.end(), std::back_inserter(drawable), [](const auto& i) {
            return !i.lighting_only;
        });
    }
    const auto& instances = has_emitters ? drawable : source;
#else
    const auto& instances = source;
#endif
    // The animated-scenery pass submits an empty layer even in levels without
    // animated scenery. No casters changed when that layer was already empty.
    // Removing a nonempty layer must still rebuild it and invalidate cached depth.
    if (layer != Layer::Mobys && instances.empty()
        && m_groups[static_cast<std::size_t>(layer)].empty()) {
        return;
    }
    // The CPU time of handing the scene a layer's instances (the moby layer's, every frame).
    const stats::Scope pass(layer == Layer::Mobys ? "moby instances" : "layer instances");
    m_camera_visibility[static_cast<std::size_t>(layer)].recording = 0;
    if (layer == Layer::Mobys) {
        m_wetness_hero.reset();
        for (const auto& instance : instances) {
            if (instance.hero_shadow_anchor) {
                m_wetness_hero = instance;
                break;
            }
        }
        m_hero_centre.reset();
        for (const auto& instance : instances) {
            if (instance.hero_shadow_caster && instance.moby_light[3] < 1.5f
                && instance.tint[3] > 0) {
                m_hero_centre =
                    renderer::Vec3{instance.matrix[12], instance.matrix[13], instance.matrix[14]};
                break;
            }
        }
        // The body remains the projection anchor even when equipment is submitted first.
        for (const auto& instance : instances) {
            if (instance.hero_shadow_anchor && instance.hero_shadow_caster) {
                m_hero_centre =
                    renderer::Vec3{instance.matrix[12], instance.matrix[13], instance.matrix[14]};
                break;
            }
        }
    }
    // Static instances set again: the static shadow cache's cascades no longer hold them.
    if (layer != Layer::Mobys) {
        ++m_static_generation;
    }

    // The same mobys in the same order as the last time (between two ticks, or a tick that changed
    // no membership): their poses are written into the groups they are in (moby-pose-in-place).
    if (layer == Layer::Mobys
        && renderer::render_opt::enabled(renderer::render_opt::Opt::MobyPoseInPlace)
        && rewrite_mobys(instances)) {
        return;
    }

    auto& groups = m_groups[static_cast<std::size_t>(layer)];
    auto& idle = m_idle[static_cast<std::size_t>(layer)];
    idle.insert(
        idle.end(), std::make_move_iterator(groups.begin()), std::make_move_iterator(groups.end())
    );
    groups.clear();
    // A static layer set again (choose_scenery) keeps its bounds and resident instances.
    build_groups(layer, instances, layer == Layer::Mobys);

    // The moby groups as build_groups made them: where each instance went, for the next call.
    if (layer == Layer::Mobys) {
        remember_mobys(instances);
    }
}

void LevelScene::choose_scenery(
    float scale, const renderer::Vec3& eye, const renderer::Mat4& projection
) {
    // How much farther than its depth a point in the view can be: 1 / the cosine of the view's
    // corner, from the projection's tangents. Within it, a straight-line distance below this times
    // the draw distance holds every instance the game draws.
    const float tx = projection[0] != 0.0f ? 1.0f / projection[0] : 0.0f;
    const float ty = projection[5] != 0.0f ? 1.0f / projection[5] : 0.0f;
    const float reach = std::sqrt(1.0f + tx * tx + ty * ty);
    constexpr float kLeaveMargin = 8.0f;  // game units past its distance before one is left out
    bool rebuilt = false;
    for (std::size_t k = 0; k < m_scenery.size(); ++k) {
        Scenery& scenery = m_scenery[k];
        const Layer layer = kSceneryLayers[k];
        if (scenery.source.empty() || scenery.distances.empty()) {
            continue;
        }
        // Unlimited again after a distance: the instances as loaded, once.
        if (!(scale > 0.0f)) {
            if (!scenery.drawn.empty()) {
                scenery.drawn.clear();
                set_instances(layer, resident_scenery(k));
            }
            continue;
        }
        const bool first = scenery.drawn.empty();
        scenery.drawn.resize(scenery.source.size(), 1);
        bool changed = first;
        // The shrubs ShrubProc draws whole this tick, when it was replayed: the others fade or are
        // billboards (add_shrub_effects), or are not drawn.
        const bool replayed = layer == Layer::Shrubs && shrub_draws_usable();
        for (std::size_t i = 0; replayed && i < scenery.source.size(); ++i) {
            const std::uint8_t drawn = m_shrub_draws[i].mesh == 0x80 ? 1 : 0;
            changed = changed || scenery.drawn[i] != drawn;
            scenery.drawn[i] = drawn;
        }
        for (std::size_t i = 0; !replayed && i < scenery.source.size(); ++i) {
            const Vec3& centre = scenery.centres[i];
            const Vec3 to{centre[0] - eye[0], centre[1] - eye[1], centre[2] - eye[2]};
            const bool was = !first && scenery.drawn[i] != 0;
            // Squared, so that the test needs no root: limit and limit + margin, both positive.
            const float limit = scenery.distances[i] * scale * reach;
            const float reach_to = was ? limit + kLeaveMargin : limit;
            const float away = to[0] * to[0] + to[1] * to[1] + to[2] * to[2];
            const bool shown =
                limit > 0.0f && (was ? away <= reach_to * reach_to : away < reach_to * reach_to);
            const std::uint8_t drawn = shown ? 1 : 0;
            changed = changed || scenery.drawn[i] != drawn;
            scenery.drawn[i] = drawn;
        }
        if (!changed) {
            continue;
        }
        set_instances(layer, resident_scenery(k));
        rebuilt = true;
    }
    // How often the layers are grouped again (a frame's average: the share of frames).
    if (stats::enabled()) {
        stats::gauge("scenery regroups", rebuilt ? 1 : 0);
    }
}

std::vector<Instance> LevelScene::resident_scenery(std::size_t k) const {
    const Scenery& scenery = m_scenery[k];
    const MovedScenery& moved = m_moved[k];
    std::vector<Instance> resident;
    resident.reserve(scenery.source.size());
    for (std::size_t i = 0; i < scenery.source.size(); ++i) {
        if ((!scenery.drawn.empty() && scenery.drawn[i] == 0)
            || (i < moved.state.size() && moved.state[i] != 0)) {
            continue;
        }
        resident.push_back(scenery.source[i]);
    }
    return resident;
}

void LevelScene::move_scenery(const GameScenery& game) {
    // The terrain's occlusion words from the game's records (GameScenery::terrain_occlusion), once
    // they differ from the primitives': the cached visible lists are made again.
    if (!game.terrain_occlusion.empty()) {
        // The streamed chunk the game's records are (Going Commando: the level's own terrain, or a
        // chunk it loaded in its place): the one with as many fragments. Only that one is drawn.
        std::map<int, std::size_t> fragments;
        for (const Model& model : m_models) {
            for (const Primitive& p : model.primitives) {
                if (p.terrain_fragment >= 0) {
                    auto& n = fragments[p.terrain_chunk];
                    n = std::max(n, static_cast<std::size_t>(p.terrain_fragment) + 1);
                }
            }
        }
        int chunk = -1;
        for (const auto& [c, n] : fragments) {
            if (n == game.terrain_occlusion.size() && chunk < 0) {
                chunk = c;
            }
        }
        bool changed = chunk != m_terrain_chunk;
        m_terrain_chunk = chunk;
        for (Model& model : m_models) {
            for (Primitive& p : model.primitives) {
                if (p.terrain_fragment >= 0 && p.terrain_chunk == chunk
                    && static_cast<std::size_t>(p.terrain_fragment)
                           < game.terrain_occlusion.size()) {
                    const std::uint16_t word =
                        game.terrain_occlusion[static_cast<std::size_t>(p.terrain_fragment)];
                    changed = changed || p.occlusion != word;
                    p.occlusion = word;
                }
            }
        }
        if (changed || game.terrain_far != m_terrain_far) {
            m_terrain_far = game.terrain_far;
            for (auto& cached : m_pvs_models) {
                cached.valid = false;
            }
        }
    }
    // How far a matrix entry may be from the loaded one and still be taken for it: the export
    // writes the game's floats, so only a move changes them.
    constexpr float kSame = 1.0e-3f;
    m_moved_count = 0;
    for (std::size_t k = 0; k < m_scenery.size(); ++k) {
        const Scenery& scenery = m_scenery[k];
        MovedScenery& moved = m_moved[k];
        const auto& now = game.matrices[k];
        const std::size_t count = scenery.source.size();
        if (count == 0 || now.size() != count) {
            continue;
        }
        // The first time: the game's matrices must be the loaded instances', or the layer is
        // drawn as loaded (an edited level, another export's order).
        if (moved.checked < 0) {
            std::size_t same = 0;
            for (std::size_t i = 0; i < count; ++i) {
                const Mat4& m = scenery.source[i].matrix;
                same += std::abs(now[i][9] - m[12]) < 0.01f && std::abs(now[i][10] - m[13]) < 0.01f
                        && std::abs(now[i][11] - m[14]) < 0.01f;
            }
            moved.checked = same * 10 >= count * 9 ? 1 : 0;
            if (moved.checked == 0) {
                log::warn(
                    "{}: {} of {} game matrices are the loaded instances'; drawn as loaded",
                    layer_name(kSceneryLayers[k]),
                    same,
                    count
                );
            }
            moved.state.assign(count, 0);
            moved.matrix.assign(count, renderer::identity());
            moved.lit.assign(count, renderer::identity());
            moved.lit_set.assign(count, 0xFF);
            moved.lit_points.assign(count, {});
        }
        if (moved.checked == 0) {
            continue;
        }
        const bool ties = kSceneryLayers[k] == Layer::Ties;
        const bool listed =
            ties && game.tie_lights.size() == count && game.tie_spheres.size() == count;
        // The ones the game hides by a bit of their record (GameScenery::tie_hidden): Dobbo
        // Orbit's buildings, whose ties stay where they stood once Giant Clank has knocked them
        // down, their collision mobys deleted.
        const auto& hidden = ties ? game.tie_hidden : game.shrub_hidden;
        bool changed = false;
        for (std::size_t i = 0; i < count; ++i) {
            const Mat4& m = scenery.source[i].matrix;
            const auto& g = now[i];
            bool zero = true, same = true;
            for (std::size_t c = 0; c < 4; ++c) {
                for (std::size_t r = 0; r < 3; ++r) {
                    const float v = g[c * 3 + r];
                    zero = zero && (c == 3 || v == 0.0f);
                    same = same && std::abs(v - m[c * 4 + r]) < kSame;
                }
            }
            const bool gone = hidden.size() == count && hidden[i] != 0;
            const std::uint8_t state = zero || gone ? 2 : same ? 0 : 1;
            m_moved_count += state == 1;
            changed = changed || state != moved.state[i];
            moved.state[i] = state;
            Mat4& w = moved.matrix[i];
            if (state == 1) {
                for (std::size_t c = 0; c < 4; ++c) {
                    for (std::size_t r = 0; r < 3; ++r) {
                        w[c * 4 + r] = g[c * 3 + r];
                    }
                    w[c * 4 + 3] = c == 3 ? 1.0f : 0.0f;
                }
            }
            // A shrub a point light reaches (its record's list) or the game moved is lit again by
            // LightShrubs' pass with its merged third light; one neither, as loaded.
            if (!ties && i < m_shrub_relight.size() && state != 2) {
                const bool shrub_listed =
                    game.shrub_lights.size() == count && game.shrub_spheres.size() == count;
                const std::uint16_t list = shrub_listed ? game.shrub_lights[i] : 0xFFFF;
                std::array<float, 32> key{};
                for (std::uint16_t l = list, n = 0; (l & 0xF) != 0xF && n < 4; l >>= 4, ++n) {
                    std::copy_n(m_point_lights.data() + 8 * (l & 7), 8, key.data() + 8 * n);
                }
                const std::uint8_t set = state == 1 || list != 0xFFFF ? 0 : 0xFF;
                const Mat4& where = state == 1 ? w : m;
                const std::array<float, 4> centre =
                    shrub_listed ? game.shrub_spheres[i] : std::array<float, 4>{};
                key[31] = static_cast<float>(list);
                if (moved.lit_set[i] == set
                    && (set == 0xFF || (moved.lit[i] == where && moved.lit_points[i] == key))) {
                    continue;
                }
                relight_shrub(i, set == 0xFF ? nullptr : &where, list, centre);
                moved.lit[i] = where;
                moved.lit_set[i] = set;
                moved.lit_points[i] = key;
                continue;
            }
            if (!ties || i >= m_tie_relight.size() || state == 2) {
                continue;
            }
            // A tie the game moved is lit again by where it is now, one a point light reaches
            // with it too; one neither moved nor reached, as loaded.
            std::array<TiePointLight, 4> points{};
            std::array<float, 32> key{};
            std::size_t lights = 0;
            for (std::uint16_t list = listed ? game.tie_lights[i] : 0xFFFF; lights < 4;
                 list >>= 4) {
                const std::size_t slot = list & 0xF;
                if (slot == 0xF) {
                    break;
                }
                TiePointLight& p = points[lights];
                const float* bank = m_point_lights.data() + 8 * (slot & 7);
                p.colour = {bank[0], bank[1], bank[2]};
                p.falloff = bank[3];
                p.position = {bank[4], bank[5], bank[6]};
                p.reach = bank[7];
                std::copy_n(bank, 8, key.data() + 8 * lights);
                ++lights;
            }
            const std::uint8_t set = state == 1 || lights > 0 ? m_tie_relight[i].light_set : 0xFF;
            const Mat4& where = state == 1 ? w : m;
            if (moved.lit_set[i] == set
                && (set == 0xFF || (moved.lit[i] == where && moved.lit_points[i] == key))) {
                continue;
            }
            if (set == 0xFF) {
                relight_moved_tie(i, nullptr);
            } else {
                relight_moved_tie(
                    i,
                    &where,
                    std::span<const TiePointLight>(points.data(), lights),
                    listed ? game.tie_spheres[i] : std::array<float, 4>{}
                );
            }
            moved.lit[i] = where;
            moved.lit_set[i] = set;
            moved.lit_points[i] = key;
        }
        if (changed) {
            set_instances(kSceneryLayers[k], resident_scenery(k));
        }
    }
}

void LevelScene::relight_moved_tie(
    std::size_t i,
    const Mat4* matrix,
    std::span<const TiePointLight> points,
    const std::array<float, 4>& sphere
) {
    const Instance& instance = m_scenery[0].source[i];
    const TieRelight& relight = m_tie_relight[i];
    if (m_lights == 0 || instance.lights < 0 || relight.cls < 0
        || static_cast<std::size_t>(relight.cls) >= m_tie_relight_classes.size()
        || static_cast<std::size_t>(relight.first) + relight.count > m_tie_slots.size()) {
        return;
    }
    const TieRelightClass& cls = m_tie_relight_classes[static_cast<std::size_t>(relight.cls)];
    const std::size_t first = static_cast<std::size_t>(instance.lights);
    const std::size_t count = cls.vertices.size() / 5;
    if (first + count > m_light_colours.size()) {
        return;
    }
    std::vector<std::uint32_t> colours;
    if (matrix != nullptr && m_light_sets.size() == std::size_t{16} * 16) {
        relight_tie(
            cls,
            std::span(m_tie_slots).subspan(relight.first, relight.count),
            *matrix,
            std::span<const float, 16>(m_light_sets.data() + 16 * relight.light_set, 16),
            colours,
            points,
            sphere
        );
    } else {
        colours.assign(m_light_colours.begin() + first, m_light_colours.begin() + first + count);
    }
    upload_light_colours(first, colours);
}

void LevelScene::upload_light_colours(std::size_t first, std::span<const std::uint32_t> colours) {
    // The texture's rows of 1024 colours that the instance's run crosses.
    constexpr std::size_t width = kLightWidth;
    glBindTexture(GL_TEXTURE_2D, m_lights);
    for (std::size_t at = 0; at < colours.size();) {
        const std::size_t texel = first + at;
        const std::size_t run = std::min(colours.size() - at, width - texel % width);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>(texel % width),
            static_cast<GLint>(texel / width),
            static_cast<GLsizei>(run),
            1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            colours.data() + at
        );
        at += run;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void LevelScene::relight_shrub(
    std::size_t i, const Mat4* matrix, std::uint16_t list, const std::array<float, 4>& centre
) {
    const Instance& instance = m_scenery[1].source[i];
    const LevelData::ShrubRelight& relight = m_shrub_relight[i];
    if (m_lights == 0 || instance.lights < 0 || relight.cls < 0
        || static_cast<std::size_t>(relight.cls) >= m_shrub_relight_classes.size()) {
        return;
    }
    const std::size_t first = static_cast<std::size_t>(instance.lights);
    if (first + 24 > m_light_colours.size()) {
        return;
    }
    std::array<std::uint32_t, 24> colours{};
    if (matrix != nullptr && m_light_sets.size() == std::size_t{16} * 16) {
        // Ratchet & Clank's LightShrubs, which Going Commando's (0x2E2C38) is instruction for
        // instruction, point lights merged from the run-time record's centre and list.
        namespace geo = openrac::assets::rac1;
        geo::ShrubClass cls;
        cls.normals.assign(
            m_shrub_relight_classes[static_cast<std::size_t>(relight.cls)].begin(),
            m_shrub_relight_classes[static_cast<std::size_t>(relight.cls)].end()
        );
        geo::ShrubInstance inst;
        for (std::size_t c = 0; c < 4; ++c) {
            for (std::size_t r = 0; r < 4; ++r) {
                inst.matrix[c][r] = (*matrix)[c * 4 + r];
            }
        }
        inst.matrix[3][3] = 0.01f;
        inst.colour = relight.colour;
        inst.dir_lights = relight.select;
        geo::LightBank bank;
        for (std::size_t s = 0; s < 16 && s < bank.sets.size(); ++s) {
            std::memcpy(&bank.sets[s], m_light_sets.data() + 16 * s, sizeof(geo::DirLightSet));
        }
        bank.count = 16;
        geo::PointLightBank points{};
        std::memcpy(points.data(), m_point_lights.data(), sizeof(points));
        const geo::ShrubPalette palette = geo::light_shrub_instance(
            cls, inst, bank, geo::ShrubPointLights{&points, list, &centre}
        );
        for (std::size_t j = 0; j < 24; ++j) {
            colours[j] = static_cast<std::uint32_t>(palette[j][0])
                         | static_cast<std::uint32_t>(palette[j][1]) << 8
                         | static_cast<std::uint32_t>(palette[j][2]) << 16
                         | static_cast<std::uint32_t>(palette[j][3]) << 24;
        }
    } else {
        std::copy_n(
            m_light_colours.begin() + static_cast<std::ptrdiff_t>(first), 24, colours.begin()
        );
    }
    upload_light_colours(first, colours);
}

void LevelScene::add_moved_scenery(
    std::vector<Instance>& live,
    const std::array<bool, 2>& layers,
    const std::function<Mat4(std::size_t, std::size_t, const Mat4&)>& at
) const {
    for (std::size_t k = 0; k < m_scenery.size(); ++k) {
        // A layer whose pass the game did not run this tick (the vendor's frozen world) has none.
        if (k >= layers.size() || !layers[k]) {
            continue;
        }
        const Scenery& scenery = m_scenery[k];
        const MovedScenery& moved = m_moved[k];
        for (std::size_t i = 0; i < moved.state.size() && i < scenery.source.size(); ++i) {
            if (moved.state[i] != 1 || (!scenery.drawn.empty() && scenery.drawn[i] == 0)) {
                continue;
            }
            Instance instance = scenery.source[i];
            instance.matrix = at ? at(k, i, moved.matrix[i]) : moved.matrix[i];
            instance.occlusion = Pvs::always;
            // A stable identity for the motion history: layer and place, apart from the mobys'
            // addresses (below 0x2000000).
            instance.temporal_id = (std::uint64_t{k + 1} << 40) | i;
            live.push_back(instance);
        }
    }
}

bool LevelScene::shrub_draws_usable() const {
    const Scenery& shrubs = m_scenery[1];
    return !shrubs.source.empty() && !shrubs.distances.empty()
           && m_shrub_draws.size() == shrubs.source.size()
           && m_shrub_billboards.size() == shrubs.source.size();
}

void LevelScene::set_shrub_draws(std::vector<ShrubDraw> draws) {
    m_shrub_draws = std::move(draws);
}

void LevelScene::add_shrub_effects(std::vector<Instance>& live) const {
    if (!shrub_draws_usable()) {
        return;
    }
    const auto& source = m_scenery[1].source;
    for (std::size_t i = 0; i < source.size(); ++i) {
        const ShrubDraw& draw = m_shrub_draws[i];
        // The fading list: the mesh at its vertex alpha, its texels at 0x0C and over writing depth.
        if (draw.mesh != 0 && draw.mesh < 0x80) {
            Instance mesh = source[i];
            mesh.tint[3] = static_cast<float>(draw.mesh) / 128.0f;
            mesh.blend = 4;
            live.push_back(mesh);
        }
        // The billboard: its first pass (0x80, TEST 0x5360b) as a moby's face, its second (TEST
        // 0x53001) blended without depth; coloured with the instance's average lit colour.
        if (draw.billboard != 0 && m_shrub_billboards[i] >= 0) {
            Instance billboard;
            billboard.model = static_cast<std::uint32_t>(m_shrub_billboards[i]);
            billboard.matrix = draw.billboard_matrix;
            billboard.tint = m_shrub_colours[i];
            billboard.tint[3] = static_cast<float>(draw.billboard) / 128.0f;
            billboard.blend = draw.billboard == 0x80 ? 1 : 5;
            billboard.occlusion = source[i].occlusion;
            live.push_back(billboard);
        }
    }
}

void LevelScene::remember_mobys(const std::vector<Instance>& instances) {
    auto& layout = m_moby_layout;
    const auto& groups = m_groups[static_cast<std::size_t>(Layer::Mobys)];
    layout.keys.resize(instances.size());
    layout.group.resize(instances.size());
    layout.slot.resize(instances.size());
    layout.spheres.resize(instances.size());
    layout.filled.assign(groups.size(), 0);
    layout.sphere_off = moby_sphere_culling_off();

    // Each instance's group, by its key (build_groups' lists are the groups, in order), and its
    // place in it, in the order build_groups appended it.
    for (std::size_t n = 0; n < instances.size(); ++n) {
        const std::uint32_t key = group_key(instances[n]);
        const std::size_t group = m_group_of_key.at(key);
        layout.keys[n] = key;
        layout.group[n] = static_cast<std::uint32_t>(group);
        layout.slot[n] = layout.filled[group]++;
        layout.spheres[n] = instances[n].world_sphere;
    }

    layout.valid = true;
}

bool LevelScene::rewrite_mobys(const std::vector<Instance>& instances) {
    auto& layout = m_moby_layout;
    auto& groups = m_groups[static_cast<std::size_t>(Layer::Mobys)];

    // No layout, one of another count of mobys, or bounds made with the sphere switch otherwise
    // (OPENRAC_MOBY_SPHERE_CULL_OFF changed since): they are grouped again.
    if (!layout.valid || instances.size() != layout.keys.size()
        || layout.filled.size() != groups.size()
        || layout.sphere_off != moby_sphere_culling_off()) {
        return false;
    }

    // A moby of another class, blend or shadow role at a place: the membership changed.
    for (std::size_t n = 0; n < instances.size(); ++n) {
        // Another key at this place: grouped again.
        if (group_key(instances[n]) != layout.keys[n]) {
            return false;
        }
    }

    const bool native_gpu_bounds = gl::is_vulkan();

    // Each moby's attributes into its place; its bounds again only when its pose or sphere moved.
    for (std::size_t n = 0; n < instances.size(); ++n) {
        Group& g = groups[layout.group[n]];
        const std::uint32_t slot = layout.slot[n];
        const InstanceData data = instance_data(instances[n]);
        const auto& sphere = instances[n].world_sphere;
        const bool moved =
            std::memcmp(&g.cpu_instances[slot], &data, sizeof(data)) != 0
            || std::memcmp(layout.spheres[n].data(), sphere.data(), sizeof(sphere)) != 0;
        g.cpu_instances[slot] = data;
        if (g.floats_on_water) {
            g.floating_ids[slot] = instances[n].temporal_id;
        }
        g.occlusion[slot] = instances[n].occlusion;

        // The same pose and sphere give the same bounds as before.
        if (moved) {
            g.caster_bounds[slot] = instance_bounds(
                m_models[g.model], data, sphere, layout.sphere_off, g.floats_on_water
            );
            layout.spheres[n] = sphere;
        }

        // The GPU's form also carries the PVS word, which may change with the same pose.
        if (native_gpu_bounds) {
            g.gpu_bounds[slot] = gpu_cull_bounds(g.caster_bounds[slot], data, g.occlusion[slot]);
        }
    }

    // Each group's instances sent, and what build_groups derives from them made again.
    for (Group& g : groups) {
        fill_group(g, g.cpu_instances, GL_STREAM_DRAW);
        g.shadow_solid_instances = solid_shadow_instances(g.cpu_instances);
        g.camera_valid = false;
        g.pvs_valid = false;
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void LevelScene::set_palette(const std::vector<JointColumns>& matrices) {
    // The CPU time of the joint palette's upload, every frame the mobys move.
    const stats::Scope pass("moby palette");
    if (matrices.empty()) {
        return;
    }
    if (m_palette == 0) {
        glGenTextures(1, &m_palette);
        glBindTexture(GL_TEXTURE_2D, m_palette);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }
    // Whole rows: the matrices, then zeros to the end of the last row.
    constexpr std::size_t kPerRow = kPaletteWidth / 4;
    const std::size_t rows = (matrices.size() + kPerRow - 1) / kPerRow;
    stats::gauge("moby palette rows", double(rows));

    // The texture holds these joints already, bit for bit (a still palette): nothing is sent.
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::MobyPoseInPlace)
        && rows <= m_palette_rows && same_palette(matrices, m_palette_data, rows * kPerRow)) {
        stats::gauge("moby palette changed rows", 0);
        stats::gauge("moby palette uploaded rows", 0);
        stats::gauge("moby palette upload calls", 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    static const bool& full_upload =
        stats::probe("OPENRAC_VK_PALETTE_PARTIAL_OFF", "Upload complete Vulkan joint palettes");
    if (gl::is_vulkan() && renderer::render_opt::enabled(renderer::render_opt::Opt::MobyPoseInPlace)
        && rows <= m_palette_rows && m_palette_data.size() == rows * kPerRow) {
        std::array<JointColumns, kPerRow> last{};
        const auto tail = matrices.size() - (rows - 1) * kPerRow;
        std::copy_n(matrices.data() + (rows - 1) * kPerRow, tail, last.data());
        std::array<std::pair<std::size_t, std::size_t>, 8> runs{};
        std::size_t run_count = 0;
        std::size_t changed_rows = 0;
        for (std::size_t row = 0; row < rows; ++row) {
            const auto* source = row + 1 == rows ? last.data() : matrices.data() + row * kPerRow;
            auto* previous = m_palette_data.data() + row * kPerRow;
            if (std::memcmp(source, previous, sizeof(last)) == 0) {
                continue;
            }
            std::copy_n(source, kPerRow, previous);
            ++changed_rows;
            if (run_count <= runs.size()) {
                if (run_count != 0 && runs[run_count - 1].second == row) {
                    runs[run_count - 1].second = row + 1;
                } else if (run_count < runs.size()) {
                    runs[run_count++] = {row, row + 1};
                } else {
                    ++run_count;
                }
            }
        }
        stats::gauge("moby palette changed rows", double(changed_rows));
        // Limit transfer calls when dirty rows are scattered or cover most of the palette.
        const bool sparse = !full_upload && run_count <= runs.size() && changed_rows <= rows / 2;
        glBindTexture(GL_TEXTURE_2D, m_palette);
        const auto upload_rows = [&](std::size_t first, std::size_t end) {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                static_cast<GLint>(first),
                kPaletteWidth,
                static_cast<GLsizei>(end - first),
                GL_RGBA,
                GL_FLOAT,
                m_palette_data.data() + first * kPerRow
            );
        };
        if (sparse) {
            for (const auto& [first, end] : std::span(runs).first(run_count)) {
                upload_rows(first, end);
            }
        } else {
            upload_rows(0, rows);
        }
        stats::gauge("moby palette uploaded rows", double(sparse ? changed_rows : rows));
        stats::gauge("moby palette upload calls", double(sparse ? run_count : 1));
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    stats::gauge("moby palette changed rows", double(rows));
    stats::gauge("moby palette uploaded rows", double(rows));
    stats::gauge("moby palette upload calls", 1);
    m_palette_data.assign(rows * kPerRow, JointColumns{});
    std::copy(matrices.begin(), matrices.end(), m_palette_data.begin());
    glBindTexture(GL_TEXTURE_2D, m_palette);
    // The texture keeps its storage while the palettes fit in it: only its texels are sent.
    if (rows > m_palette_rows) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>(kRgba32f),
            kPaletteWidth,
            static_cast<GLsizei>(rows),
            0,
            GL_RGBA,
            GL_FLOAT,
            m_palette_data.data()
        );
        m_palette_rows = rows;
    } else {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            kPaletteWidth,
            static_cast<GLsizei>(rows),
            GL_RGBA,
            GL_FLOAT,
            m_palette_data.data()
        );
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}
}  // namespace openrac::scene
