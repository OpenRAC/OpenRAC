// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <algorithm>
#include <bit>
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
#include "scene/shadow_graph.h"
#include "scene_shaders.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::scene {
using namespace openrac::gl;
using namespace render_detail;

bool LevelScene::make_shadow_atlas(std::string& error) {
    if (!m_atlas_depth.id()) {
        std::string vertex(deformation_vertex(shaders::shadow_depth_vert));
        vertex.insert(
            vertex.find('\n') + 1,
            "#extension GL_ARB_shader_viewport_layer_array : require\n"
            "#define OPENRAC_SHADOW_ATLAS 1\n"
        );
        if (!m_atlas_depth.build("atlas shadow depth", vertex, shaders::hero_shadow_frag, error)) {
            return false;
        }
        std::string fragment(shaders::hero_shadow_frag);
        vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
        fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
        if (!m_atlas_solid.build("atlas opaque shadow depth", vertex, fragment, error)) {
            m_atlas_depth.release();
            return false;
        }
    }
    std::string dry_error;
    if (!make_dry_shadow_programs(true, dry_error)) {
        log::warn("Dry atlas shadow variants unavailable: {}", dry_error);
    }
    if (!m_shadow_atlas) {
        m_shadow_atlas = std::make_unique<renderer::FrameBuffer>();
    }
    if (!m_shadow_atlas->id()
        && !create_shadow_target(*m_shadow_atlas, m_shadow_layout.size, error)) {
        return false;
    }
    // The reference targets are recreated on demand if the user switches
    // paths; retaining both sets would waste shadow-map storage every frame.
    release_target(m_hero_map);
    for (auto& map : m_sun_maps) {
        release_target(map);
    }
    return true;
}

void LevelScene::prepare_shadow_atlas_draws() {
    // Shader bindings reserve unit six for the hero and the next four for sun cascades.
    constexpr unsigned kFirstShadowUnit = 6;
    // Avoid sampling feedback while depth attachments are written again.
    for (unsigned i = 0; i < m_shadow_view_projection.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + kFirstShadowUnit + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    // Indexed viewports share one matrix table for the sun cascades and the hero.
    for (unsigned i = 0; i < kSunCascades; ++i) {
        m_shadow_view_projection[i] = m_sun_cascades.matrices[i];
    }
    m_shadow_view_projection[kSunCascades] = m_hero_matrix;
    // Pending commands belong only to this frame's matrices and target allocation.
    for (auto& bucket : m_shadow_draws) {
        bucket.clear();
    }
}

void LevelScene::clear_shadow_atlas(bool discard) {
    glClearDepth(0);
    // Stencil and colour have no shadow consumers, so tiled GPUs need not retain them.
    if (discard) {
        const GLenum unread = GL_COLOR_ATTACHMENT0;
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &unread);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void LevelScene::record_shadow_atlas(
    unsigned scrolling, bool cache, bool discard, renderer::RenderState& state
) {
#if defined(OPENRAC_VULKAN)
    begin_shadow_target(m_shadow_atlas->id());
    // A copied cache already defines depth; all other paths initialise the entire atlas here.
    if (!cache) {
        clear_shadow_atlas(discard);
    }
    vkgl::indexed_viewports(m_shadow_layout.rects);
    // The sun's four views share draw buckets until the final sun or hero map flushes them.
    if (m_sun_ready) {
        // Each indexed viewport keeps the original cascade-specific caster selection.
        for (unsigned i = 0; i < kSunCascades; ++i) {
            auto casters = cache ? ShadowCasters::Dynamic : ShadowCasters::All;
            // Scrolling caches contain only static opaque casters; alpha coverage stays dynamic.
            if (scrolling & (1u << i)) {
                casters = ShadowCasters::DynamicAlpha;
            }
            draw_shadow_map(
                m_shadow_atlas->id(),
                m_shadow_layout.rects[i][2],
                m_sun_cascades.matrices[i],
                false,
                m_hero_ready,
                state,
                i,
                static_cast<int>(i),
                i == kSunCascades - 1 && !m_hero_ready,
                casters
            );
        }
    }
    // Hero casters stay isolated from the sun maps under the existing shadow system.
    if (m_hero_ready) {
        draw_shadow_map(
            m_shadow_atlas->id(),
            m_shadow_layout.rects[kSunCascades][2],
            m_hero_matrix,
            true,
            false,
            state,
            kSunCascades,
            static_cast<int>(kSunCascades),
            true
        );
    }
#else
    (void)scrolling;
    (void)cache;
    (void)discard;
    (void)state;
#endif
}

void LevelScene::record_shadow_atlas_views(
    unsigned views, bool discard, renderer::RenderState& state
) {
#if defined(OPENRAC_VULKAN)
    begin_shadow_target(m_shadow_atlas->id());
    auto rects = m_shadow_layout.rects;
    int first = -1, last = -1;
    int low_x = 0, low_y = 0, high_x = 0, high_y = 0;

    // The rectangle that holds these views.
    for (int i = 0; i < static_cast<int>(rects.size()); ++i) {
        if (!(views & (1u << i))) {
            continue;
        }

        const auto& r = rects[static_cast<std::size_t>(i)];
        if (first < 0) {
            first = i;
            low_x = r[0];
            low_y = r[1];
            high_x = r[0] + r[2];
            high_y = r[1] + r[3];
        }
        low_x = std::min(low_x, r[0]);
        low_y = std::min(low_y, r[1]);
        high_x = std::max(high_x, r[0] + r[2]);
        high_y = std::max(high_y, r[1] + r[3]);
        last = i;
    }

    if (first < 0) {
        return;
    }

    // A view the pass leaves alone takes a drawn one's rectangle: a draw counts as writing every
    // indexed viewport, and the pass must cover no kept cascade.
    for (std::size_t i = 0; i < rects.size(); ++i) {
        if (!(views & (1u << i))) {
            rects[i] = rects[static_cast<std::size_t>(first)];
        }
    }

    // Cleared first and alone, the rectangle is all the pass covers: nothing of it is loaded, and
    // the kept cascades beside it are neither loaded nor stored. The colour nothing reads is let
    // go and the stencil cleared with the depth, or they would be loaded.
    if (discard) {
        const GLenum unread = GL_COLOR_ATTACHMENT0;
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &unread);
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(low_x, low_y, high_x - low_x, high_y - low_y);
    glClearDepth(0);
    glClear(discard ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    vkgl::indexed_viewports(rects);

    // The pass's last view flushes the draw buckets its views share.
    for (unsigned i = 0; i < kSunCascades; ++i) {
        if (!(views & (1u << i))) {
            continue;
        }

        draw_shadow_map(
            m_shadow_atlas->id(),
            m_shadow_layout.rects[i][2],
            m_sun_cascades.matrices[i],
            false,
            m_hero_ready,
            state,
            i,
            static_cast<int>(i),
            static_cast<int>(i) == last
        );
    }

    if (views & (1u << kSunCascades)) {
        draw_shadow_map(
            m_shadow_atlas->id(),
            m_shadow_layout.rects[kSunCascades][2],
            m_hero_matrix,
            true,
            false,
            state,
            kSunCascades,
            static_cast<int>(kSunCascades),
            true
        );
    }
#else
    (void)views;
    (void)discard;
    (void)state;
#endif
}

void LevelScene::finish_shadow_atlas(bool discard, const renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    vkgl::indexed_viewports({});
#endif
    glBindVertexArray(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // Only depth feeds lighting; dropping colour/stencil does not invalidate exported depth.
    if (discard) {
        const GLenum unread[] = {GL_COLOR_ATTACHMENT0, GL_STENCIL_ATTACHMENT};
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, unread);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
    glViewport(0, 0, state.target.width, state.target.height);
}

void LevelScene::draw_shadow_atlas(renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    const stats::Scope pass("shadow atlas", stats::kGpu);
    const auto before = state.stats;
    prepare_shadow_atlas_draws();
    const unsigned scrolling =
        m_sun_ready && m_scroll_requested ? update_scrolling_shadows(state) : 0;
    bool cache = !scrolling && m_sun_ready
                 && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowStaticCache);
    unsigned rerendered = m_sun_ready ? kSunCascades : 0;
    // Allocation failure must draw all casters rather than treating missing cached depth as valid.
    if (cache) {
        rerendered = draw_cached_cascades(state);
        cache = m_static_shadow && m_static_shadow->id();
    }
    // Scrolling, disabled and failed caches cannot lend stale keys or depth to this frame.
    if (!cache) {
        release_target(m_static_shadow);
        m_static_keys = {};
    }
    const bool discard =
        glInvalidateFramebuffer != nullptr
        && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowAtlasDiscard);
    // Kept far cascades (shadow-far-alternate) are neither cleared nor drawn: the nearest cascade
    // with the hero's map, then the far cascades due this frame, each in a pass of its own.
    const unsigned kept = m_sun_ready && !cache && !scrolling ? m_kept_cascades : 0;
    if (kept) {
        const unsigned sun = (1u << kSunCascades) - 1;
        const unsigned near = 1u | (m_hero_ready ? 1u << kSunCascades : 0u);
        rerendered = kSunCascades - static_cast<unsigned>(std::popcount(kept));

        for (const unsigned views : {near, sun & ~kept & ~1u}) {
            const auto drawn = shadow_depth_pass(
                "dynamic shadow atlas",
                m_shadow_atlas->id(),
                ShadowDepthContents::PreserveSeparate,
                [this, &state, views, discard] { record_shadow_atlas_views(views, discard, state); }
            );
            // A rejected graph must not advertise an atlas ready for lighting.
            if (!drawn) {
                throw std::runtime_error(drawn.error().message);
            }
        }
    } else {
        // The scoped callback preserves bucket batching and its existing last-cascade flush.
        const auto drawn = shadow_depth_pass(
            "dynamic shadow atlas",
            m_shadow_atlas->id(),
            cache ? ShadowDepthContents::Preserve : ShadowDepthContents::Clear,
            [this, &state, scrolling, cache, discard] {
                record_shadow_atlas(scrolling, cache, discard, state);
            }
        );
        // A rejected graph must not advertise an atlas ready for lighting.
        if (!drawn) {
            throw std::runtime_error(drawn.error().message);
        }
    }
    stats::gauge("shadow_far_kept", static_cast<double>(std::popcount(kept)));
    // Compute imports the actual sampled depth exported by the completed atlas phase.
    if (scrolling) {
        combine_scrolling_shadows(scrolling, state);
    }
    // Completion runs after every deferred caster, cache copy and composition fallback.
    const auto finished =
        shadow_depth_ready(m_shadow_atlas->id(), [this, &state, discard, before, rerendered] {
            complete_shadow_atlas(state, discard, before, rerendered);
        });
    // An incomplete final phase cannot expose the atlas as ready.
    if (!finished) {
        throw std::runtime_error(finished.error().message);
    }
    // Publish the same final bindings during preparation; attachment invalidation stays deferred.
    glBindVertexArray(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
    glViewport(0, 0, state.target.width, state.target.height);
#else
    (void)state;
#endif
}

void LevelScene::complete_shadow_atlas(
    const renderer::RenderState& state,
    bool discard,
    renderer::FrameStats before,
    unsigned rerendered
) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_atlas->id());
    finish_shadow_atlas(discard, state);
    report_sun_culling();
    stats::gauge(
        "vk_shadow_draws", static_cast<double>(state.stats.draw_calls - before.draw_calls)
    );
    stats::gauge(
        "vk_shadow_triangles", static_cast<double>(state.stats.triangles - before.triangles)
    );
    stats::gauge("shadow_static_rerenders", static_cast<double>(rerendered));
}

void LevelScene::begin_shadow_target(unsigned framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, m_shadow_layout.size, m_shadow_layout.size);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}
}  // namespace openrac::scene
