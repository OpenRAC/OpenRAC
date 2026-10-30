// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

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

// A class model's vertex array: the level's vertices and indices, and an instance buffer of its own
// (empty until fill_group).
LevelScene::Group LevelScene::make_group(std::uint32_t model) {
    Group g;
    g.model = model;
    glGenVertexArrays(1, &g.vao);
    glBindVertexArray(g.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);
    constexpr auto kStride = static_cast<GLsizei>(sizeof(Vertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride, offset(offsetof(Vertex, position)));
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
    return instance.model << 5 | (instance.floats_on_water ? 16u : 0u)
           | (instance.gameplay_essential ? 8u : 0u)
           | (static_cast<std::uint32_t>(instance.blend & 3) << 1)
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
        const std::uint32_t model = m_grouping[i].key >> 5;
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
        g.blend = static_cast<int>((m_grouping[i].key >> 1) & 3);
        g.hero_shadow_caster = (m_grouping[i].key & 1) != 0;
        g.gameplay_essential = (m_grouping[i].key & 8) != 0;
        g.floats_on_water = (m_grouping[i].key & 16) != 0;
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

void LevelScene::set_instances(Layer layer, const std::vector<Instance>& instances) {
    // The CPU time of handing the scene a layer's instances (the moby layer's, every frame).
    const stats::Scope pass(layer == Layer::Mobys ? "moby instances" : "layer instances");
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
    bool original, const renderer::Vec3& eye, const renderer::Mat4& projection
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
        if (scenery.source.empty()) {
            continue;
        }
        // Off again after being on: the instances as loaded, once.
        if (!original) {
            if (!scenery.drawn.empty()) {
                scenery.drawn.clear();
                set_instances(layer, scenery.source);
            }
            continue;
        }
        const bool first = scenery.drawn.empty();
        scenery.drawn.resize(scenery.source.size(), 1);
        bool changed = first;
        for (std::size_t i = 0; i < scenery.source.size(); ++i) {
            const Vec3& centre = scenery.centres[i];
            const Vec3 to{centre[0] - eye[0], centre[1] - eye[1], centre[2] - eye[2]};
            const bool was = !first && scenery.drawn[i] != 0;
            // Squared, so that the test needs no root: limit and limit + margin, both positive.
            const float limit = scenery.distances[i] * reach;
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
        std::vector<Instance> chosen;
        chosen.reserve(scenery.source.size());
        for (std::size_t i = 0; i < scenery.source.size(); ++i) {
            if (scenery.drawn[i] == 0) {
                continue;
            }
            chosen.push_back(scenery.source[i]);
        }
        set_instances(layer, chosen);
        rebuilt = true;
    }
    // How often the layers are grouped again (a frame's average: the share of frames).
    if (stats::enabled()) {
        stats::gauge("scenery regroups", rebuilt ? 1 : 0);
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

    // The texture holds these joints already, bit for bit (a still palette): nothing is sent.
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::MobyPoseInPlace)
        && rows <= m_palette_rows && same_palette(matrices, m_palette_data, rows * kPerRow)) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

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
