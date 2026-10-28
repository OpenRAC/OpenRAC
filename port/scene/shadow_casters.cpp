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
    const auto use = [&](bool solid, bool skinning) {
        const auto& shader = solid ? m_shadow_solid : m_hero_depth;
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
    const auto submit = [&](const ShadowDraw& draw) {
        if (atlas_view >= 0) {
            const auto& shader = draw.solid ? m_atlas_solid : m_atlas_depth;
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
            use(draw.solid, draw.skinning);
        }
        glBindVertexArray(draw.vao);
        if (!draw.solid) {
            const auto& shader = atlas_view >= 0 ? m_atlas_depth : m_hero_depth;
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
        submit_instances(draw.batch, draw.first, draw.count, draw.instances);
        ++state.stats.draw_calls;
        state.stats.triangles += std::uint64_t{draw.count / 3} * draw.instances;
    };
    const auto flush_buckets = [&] {
        for (auto& bucket : shadow_draws) {
            for (const auto& command : bucket) {
                submit(command);
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

        static constexpr const char* kNames[kSunCascades][4] =
            {{"sun0 terrain", "sun0 ties", "sun0 shrubs", "sun0 mobys"},
             {"sun1 terrain", "sun1 ties", "sun1 shrubs", "sun1 mobys"},
             {"sun2 terrain", "sun2 ties", "sun2 shrubs", "sun2 mobys"},
             {"sun3 terrain", "sun3 ties", "sun3 shrubs", "sun3 mobys"}};
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
                        vao, instances, batch, start, size, solid, skinning, material, atlas_view
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
