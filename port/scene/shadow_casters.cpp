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
#include "scene/level_lighting.h"
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

void LevelScene::draw_shadow_map(
    unsigned target,
    int resolution,
    const renderer::Mat4& matrix,
    bool hero,
    bool separate_hero,
    renderer::RenderState& state,
    unsigned cascade,
    int atlas_view,
    bool flush_atlas,
    ShadowCasters casters,
    const std::array<int, 4>* region
) {
    static const bool& detailed_timing = stats::probe(
        "OPENRAC_SUN_TIMING_DETAIL", "Detailed cascade/layer GPU timers (higher profiling overhead)"
    );
    static constexpr const char* kMapNames[] = {"sun0 map", "sun1 map", "sun2 map", "sun3 map"};
    const char* pass_name = cascade < kSunCascades ? kMapNames[cascade] : "sun shadow map";
    if (hero) {
        pass_name = "Ratchet shadow";
    }
    const stats::Scope pass(
        pass_name, atlas_view < 0 && (hero || !detailed_timing) ? stats::kGpu : 0
    );
    if (atlas_view < 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glViewport(0, 0, resolution, resolution);
        if (region) {
            glEnable(GL_SCISSOR_TEST);
            glScissor((*region)[0], (*region)[1], (*region)[2], (*region)[3]);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glDepthMask(GL_TRUE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        {
            const stats::Scope clear("shadow clear", !hero && detailed_timing ? stats::kGpu : 0);
            glClearDepth(0);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }
    const renderer::Shader* program = nullptr;
    int set_skinning = -1;
    static const bool& solid_off = stats::probe(
        "OPENRAC_SHADOW_SOLID_OFF",
        "Profiling reference: alpha-capable shader for opaque static shadow casters"
    );
    const auto use = [&](bool solid, bool skinning, bool dry) {
        const auto& shader = dry && m_dry_shadow[solid].id() ? m_dry_shadow[solid]
                             : solid                         ? m_shadow_solid
                                                             : m_hero_depth;
        if (program != &shader) {
            shader.use();
            glUniformMatrix4fv(shader.uniform("view_projection"), 1, GL_FALSE, matrix.data());
            if (!solid) {
                glUniform1i(shader.uniform("palette_texture"), 1);
                glUniform1i(shader.uniform("tex_T0"), 0);
            }
            program = &shader;
            set_skinning = -1;
        }
        if (!solid && set_skinning != int(skinning)) {
            glUniform1i(shader.uniform("skinning"), skinning ? 1 : 0);
            set_skinning = int(skinning);
        }
    };
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_palette);
    glActiveTexture(GL_TEXTURE0);
    const auto sides =
        frustum_sides(region ? shadow_region_clip(matrix, *region, resolution) : matrix);
    static const bool& receiver_culling_off =
        stats::probe("OPENRAC_SUN_RECEIVER_CULL_OFF", "Reference: square shadow-map caster bounds");
    const auto* volume = gl::is_vulkan() && !hero && !receiver_culling_off && cascade < kSunCascades
                                 && m_sun_cascades.casters[cascade].plane_count
                             ? &m_sun_cascades.casters[cascade]
                             : nullptr;
    const auto outside_casters = [&](const renderer::Vec3& low, const renderer::Vec3& high) {
        return volume ? outside(volume->planes, low, high, volume->plane_count)
                      : outside(sides, low, high);
    };
    if (!hero) {
        stats::gauge("sun receiver culling", volume ? 1 : 0);
    }
    const bool gpu = gpu_driven();
    renderer::GpuCullView gpu_view;
    gpu_view.bypass_ui = false;
    std::copy(sides.begin(), sides.end(), gpu_view.planes.begin());
    gpu_view.plane_count = 4;
    if (volume) {
        gpu_view.planes = volume->planes;
        gpu_view.plane_count = volume->plane_count;
    }
    static const bool& culling_off = stats::probe(
        "OPENRAC_SUN_INSTANCE_CULL_OFF",
        "Profiling reference: retain every instance in a surviving cascade group"
    );
    auto* counts = !hero && cascade < kSunCascades ? &m_sun_culling[cascade] : nullptr;
    static const bool& batch_off =
        stats::probe("OPENRAC_SHADOW_BATCH_OFF", "Reference: submit shadow casters in model order");
    const bool cached_draws =
        renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowDrawCache);
    auto& shadow_draws = m_shadow_draws;
    const auto submit = [&](const ShadowDraw& draw,
                            std::span<const renderer::GpuIndexRange> ranges = {}) {
        const bool dry =
            !draw.skinning && !draw.floats_on_water
            && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowDryShaders);
        if (atlas_view >= 0) {
            const auto& shader = dry && m_dry_atlas[draw.solid].id() ? m_dry_atlas[draw.solid]
                                 : draw.solid                        ? m_atlas_solid
                                                                     : m_atlas_depth;
            if (program != &shader) {
                shader.use();
                glUniformMatrix4fv(
                    shader.uniform("shadow_view_projection"),
                    5,
                    GL_FALSE,
                    m_shadow_view_projection[0].data()
                );
                glUniform1i(shader.uniform("palette_texture"), 1);
                glUniform1i(shader.uniform("tex_T0"), 0);
                program = &shader;
                set_skinning = -1;
            }
            glUniform1i(shader.uniform("shadow_view"), draw.view);
            if (!draw.solid && set_skinning != int(draw.skinning)) {
                glUniform1i(shader.uniform("skinning"), draw.skinning ? 1 : 0);
                set_skinning = int(draw.skinning);
            }
        } else {
            use(draw.solid, draw.skinning, dry);
        }
        if (m_lighting) {
            m_lighting->bind_deformation(*program, draw.floats_on_water);
        }
        glBindVertexArray(draw.vao);
        if (!draw.solid) {
            const auto& shader = *program;
            const Material* material = draw.material >= 0 ? &m_materials[draw.material] : nullptr;
            const auto texture = material && material->image >= 0
                                     ? m_textures[static_cast<std::size_t>(material->image)]
                                     : m_white;
            glBindTexture(GL_TEXTURE_2D, state.textures.gl_texture(texture));
            glUniform1i(shader.uniform("cutout"), material && material->cutout ? 1 : 0);
            if (material) {
                glUniform4fv(shader.uniform("material_colour"), 1, material->colour.data());
            } else {
                glUniform4f(shader.uniform("material_colour"), 1, 1, 1, 1);
            }
        }
        if (ranges.empty()) {
            submit_instances(draw.batch, draw.first, draw.count, draw.instances);
            ++state.stats.draw_calls;
            state.stats.triangles += std::uint64_t{draw.count / 3} * draw.instances;
        } else {
#if defined(OPENRAC_VULKAN)
            if (!vkgl::draw_gpu_instance_ranges(draw.batch, ranges)) {
                throw std::runtime_error("GPU shadow ranges expired before submission");
            }
            stats::gauge("shadow opaque bulk ranges", double(ranges.size()));
            state.stats.draw_calls += static_cast<int>(ranges.size());
            for (const auto range : ranges) {
                state.stats.triangles += std::uint64_t{range.count / 3} * draw.instances;
            }
#endif
        }
    };
    const auto flush_buckets = [&] {
        constexpr const char* names[] =
            {"shadow opaque", "shadow unskinned alpha", "shadow skinned alpha"};
        for (std::size_t i = 0; i < shadow_draws.size(); ++i) {
            auto& bucket = shadow_draws[i];
            std::optional<stats::Scope> timing;
            if (atlas_view >= 0 && !bucket.empty()) {
                timing.emplace(names[i], stats::kGpu, std::source_location::current());
            }
            for (std::size_t at = 0; at < bucket.size();) {
                const auto& command = bucket[at];
                std::size_t end = at + 1;
                if (i == 0 && command.batch
                    && renderer::render_opt::enabled(
                        renderer::render_opt::Opt::ShadowGeometryCull
                    )) {
                    for (; end < bucket.size(); ++end) {
                        const auto& next = bucket[end];
                        if (next.vao != command.vao || next.view != command.view
                            || next.instances != command.instances
                            || next.skinning != command.skinning
                            || next.floats_on_water != command.floats_on_water
                            || next.batch.recording != command.batch.recording
                            || next.batch.page != command.batch.page
                            || next.batch.group != command.batch.group) {
                            break;
                        }
                    }
                }
                if (end == at + 1) {
                    submit(command);
                } else {
                    m_shadow_ranges.clear();
                    for (auto n = at; n < end; ++n) {
                        m_shadow_ranges.push_back({bucket[n].first, bucket[n].count});
                    }
                    submit(command, m_shadow_ranges);
                }
                at = end;
            }
            bucket.clear();
        }
    };
    static const bool& layer_reference = stats::probe(
        "OPENRAC_SHADOW_LAYER_REFERENCE", "Reference: flush shadow buckets after each layer"
    );
    for (std::size_t layer = 1; layer < kLayerCount; ++layer) {
        if (hero && layer != static_cast<std::size_t>(Layer::Mobys)) {
            continue;
        }

        const bool moby_layer = layer == static_cast<std::size_t>(Layer::Mobys);

        // The static shadow cache draws the static layers alone, the atlas over it the mobys.
        if ((casters == ShadowCasters::Static && moby_layer)
            || (casters == ShadowCasters::Dynamic && !moby_layer)
            || (casters == ShadowCasters::StaticOpaque && moby_layer)) {
            continue;
        }

        static constexpr const char* kNames[kSunCascades][5] =
            {{"sun0 terrain", "sun0 ties", "sun0 shrubs", "sun0 mobys", "sun0 animated"},
             {"sun1 terrain", "sun1 ties", "sun1 shrubs", "sun1 mobys", "sun1 animated"},
             {"sun2 terrain", "sun2 ties", "sun2 shrubs", "sun2 mobys", "sun2 animated"},
             {"sun3 terrain", "sun3 ties", "sun3 shrubs", "sun3 mobys", "sun3 animated"}};
        // One name for each layer this loop visits: a new layer must add its name here.
        static_assert(std::size(kNames[0]) == kLayerCount - 1);
        std::optional<stats::Scope> layer_pass;
        if (!hero && cascade < kSunCascades) {
            layer_pass.emplace(
                kNames[cascade][layer - 1],
                detailed_timing ? stats::kGpu : 0,
                std::source_location::current()
            );
        }
        const bool skinning = layer == static_cast<std::size_t>(Layer::Mobys) && m_palette;
        for (const auto& group : m_groups[layer]) {
            if ((hero && !group.hero_shadow_caster)
                || (!hero && separate_hero && group.hero_shadow_caster) || group.blend != 0) {
                continue;
            }
            const auto coverage = !moby_layer ? m_static_shadow_coverage[m_opaque[group.model]] : 0;
            if (casters == ShadowCasters::StaticOpaque
                && (!group.shadow_solid_instances || !(coverage & 2))) {
                continue;
            }
            if (casters == ShadowCasters::DynamicAlpha && !moby_layer
                && group.shadow_solid_instances && !(coverage & 1)) {
                continue;
            }
            if (counts) {
                counts->candidates += group.count;
            }
            if (group.bounded
                && (outside(sides, group.low, group.high)
                    || (volume && outside_casters(group.low, group.high)))) {
                if (counts) {
                    counts->rejected += group.count;
                }
                continue;
            }
            unsigned vao = group.vao;
            int instances = group.count;
            renderer::GpuInstanceBatch batch;
            if (gpu) {
                gpu_view.plane_count = 0;
                if (!hero && !culling_off) {
                    gpu_view.plane_count = volume ? volume->plane_count : 4u;
                }
                batch = gpu_batch(group, gpu_view);
            }
            if (!batch && !hero && !culling_off) {
                m_shadow_survivors.clear();
                for (std::size_t i = 0; i < group.cpu_instances.size(); ++i) {
                    const auto& bounds = group.caster_bounds[i];
                    if (bounds.valid && outside_casters(bounds.low, bounds.high)) {
                        if (counts) {
                            ++counts->rejected;
                        }
                        continue;
                    }
                    if (counts && !bounds.valid) {
                        ++counts->unbounded;
                    }
                    m_shadow_survivors.push_back(group.cpu_instances[i]);
                }
                instances = static_cast<int>(m_shadow_survivors.size());
                if (!instances) {
                    continue;
                }
                if (instances != group.count) {
                    if (!m_shadow_compact.vao) {
                        m_shadow_compact = make_group(group.model);
                    }
                    fill_group(m_shadow_compact, m_shadow_survivors, GL_STREAM_DRAW);
                    vao = m_shadow_compact.vao;
                    if (counts) {
                        ++counts->compacted_groups;
                    }
                }
            }
            if (counts) {
                counts->submitted += instances;
            }
            std::uint32_t first = 0, count = 0;
            bool run_solid = false;
            const auto draw =
                [&](std::uint32_t start, std::uint32_t size, bool solid, int material) {
                    const ShadowDraw command{
                        vao,
                        instances,
                        batch,
                        start,
                        size,
                        solid,
                        skinning,
                        group.floats_on_water,
                        material,
                        atlas_view
                    };
                    if (counts) {
                        if (solid) {
                            ++counts->solid_draws;
                        } else {
                            ++counts->alpha_draws;
                        }
                    }
                    if (batch && !batch_off) {
                        const unsigned alpha_pass = skinning ? 2 : 1;
                        shadow_draws[solid ? 0 : alpha_pass].push_back(command);
                    } else {
                        // The CPU culling fallback reuses one compact instance buffer;
                        // consume it before the next group can overwrite its contents.
                        submit(command);
                    }
                };
            if (cached_draws && !moby_layer) {
                const auto model = m_opaque[group.model];
                const bool partition = casters == ShadowCasters::StaticOpaque
                                       || casters == ShadowCasters::DynamicAlpha;
                const bool solid =
                    (!solid_off || partition) && !hero && group.shadow_solid_instances;
                const auto& geometry = m_shadow_geometry[model];
                const bool static_geometry = renderer::render_opt::enabled(
                    renderer::render_opt::Opt::ShadowStaticGeometryCull
                );
                const bool terrain_geometry =
                    layer == static_cast<std::size_t>(Layer::Terrain) && solid && !solid_off
                    && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowGeometryCull);
                const bool geometry_cull =
                    (static_geometry || terrain_geometry) && !skinning && !group.floats_on_water
                    && group.cpu_instances.size() == 1 && !geometry.empty()
                    && group.shadow_geometry_bounds.size() == geometry.size()
                    && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowDryShaders)
                    && (atlas_view >= 0 ? m_dry_atlas[solid].id() : m_dry_shadow[solid].id());
                if (geometry_cull) {
                    ShadowDrawRange run;
                    const auto flush_run = [&] {
                        if (run.count) {
                            draw(run.first, run.count, run.solid, run.material);
                            run = {};
                        }
                    };
                    for (std::size_t i = 0; i < geometry.size(); ++i) {
                        auto range = geometry[i].draw;
                        // Instance alpha determines whether an opaque range can omit coverage tests.
                        range.solid = range.solid && solid && !solid_off;
                        const auto& bounds = group.shadow_geometry_bounds[i];
                        const bool permanent =
                            geometry[i].draw.solid && group.shadow_solid_instances;
                        if ((casters == ShadowCasters::StaticOpaque && !permanent)
                            || (casters == ShadowCasters::DynamicAlpha && permanent)) {
                            flush_run();
                            continue;
                        }
                        if (range.solid || static_geometry) {
                            m_shadow_geometry_indices[0] += range.count;
                            // Map sides only: off-camera occluders remain, and the entire map
                            // retains identical depth. Existing bounds include a rounding pad.
                            if (bounds.valid && outside(sides, bounds.low, bounds.high)) {
                                m_shadow_geometry_indices[1] += range.count;
                                flush_run();
                                continue;
                            }
                        }
                        // Keep adjacent untextured runs joined when instance alpha requires
                        // the coverage shader instead of the solid shader.
                        if (run.count && run.solid == range.solid
                            && (run.solid || (run.material == -1 && range.material == -1))
                            && run.first + run.count == range.first) {
                            run.count += range.count;
                        } else {
                            flush_run();
                            run = range;
                        }
                    }
                    flush_run();
                    continue;
                }
                for (const auto& range : m_static_shadow_ranges[model][solid ? 1 : 0]) {
                    if ((casters == ShadowCasters::StaticOpaque && !range.solid)
                        || (casters == ShadowCasters::DynamicAlpha && range.solid)) {
                        continue;
                    }
                    draw(range.first, range.count, range.solid && !solid_off, range.material);
                }
                continue;
            }
            const auto flush = [&] {
                if (!count) {
                    return;
                }
                draw(first, count, run_solid, -1);
                count = 0;
            };
            // Opaque depth needs no material texture. Static copies have
            // contiguous indices, allowing entire terrain runs in one draw.
            const auto model = layer == static_cast<std::size_t>(Layer::Mobys)
                                   ? group.model
                                   : m_opaque[group.model];
            for (std::size_t pi = 0; pi < m_models[model].primitives.size(); ++pi) {
                const auto& primitive = m_models[model].primitives[pi];
                const auto& material = m_materials[static_cast<std::size_t>(primitive.material)];
                if (material.metal || material.blend || material.colour[3] < .5f) {
                    flush();
                    continue;
                }
                const bool permanent = !moby_layer && group.shadow_solid_instances
                                       && !material.cutout && material.colour[3] >= 1
                                       && m_shadow_solid_primitives[model][pi];
                if ((casters == ShadowCasters::StaticOpaque && !permanent)
                    || (casters == ShadowCasters::DynamicAlpha && permanent)) {
                    flush();
                    continue;
                }
                if (!material.cutout && material.colour[3] >= 1) {
                    const bool solid =
                        !solid_off && !hero && layer != static_cast<std::size_t>(Layer::Mobys)
                        && group.shadow_solid_instances && m_shadow_solid_primitives[model][pi];
                    if (count && (primitive.first_index != first + count || solid != run_solid)) {
                        flush();
                    }
                    if (!count) {
                        first = primitive.first_index;
                        run_solid = solid;
                    }
                    count += primitive.index_count;
                    continue;
                }
                flush();
                draw(primitive.first_index, primitive.index_count, false, primitive.material);
            }
            flush();
        }
        if (layer_reference && atlas_view < 0) {
            flush_buckets();
        }
    }
    // Depth-only buckets can span layers and, with indexed viewports, maps.
    if (atlas_view < 0 || flush_atlas) {
        flush_buckets();
    }
    if (atlas_view < 0) {
        glBindVertexArray(0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
        glViewport(0, 0, state.target.width, state.target.height);
    }
}
}  // namespace openrac::scene
