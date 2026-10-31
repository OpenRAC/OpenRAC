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
        && !m_shadow_atlas->create(m_shadow_layout.size, m_shadow_layout.size, error, 1, true)) {
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

void LevelScene::draw_shadow_atlas(renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    const stats::Scope pass("shadow atlas", stats::kGpu);
    const auto draws_before = state.stats.draw_calls;
    const auto triangles_before = state.stats.triangles;
    for (unsigned i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + 6 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    for (unsigned i = 0; i < 4; ++i) {
        m_shadow_view_projection[i] = m_sun_cascades.matrices[i];
    }
    m_shadow_view_projection[4] = m_hero_matrix;
    for (auto& bucket : m_shadow_draws) {
        bucket.clear();
    }
    const unsigned scrolling =
        m_sun_ready && m_scroll_requested ? update_scrolling_shadows(state) : 0;
    const bool cache =
        !scrolling && m_sun_ready
        && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowStaticCache);
    unsigned rerendered = m_sun_ready ? kSunCascades : 0;

    // Nothing samples the atlas's colour or reads its stencil: neither is loaded or written back
    // (shadow-atlas-discard; the depth is cleared with the stencil so it is not loaded either).
    const bool discard =
        glInvalidateFramebuffer != nullptr
        && renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowAtlasDiscard);
    const GLenum unread[] = {GL_COLOR_ATTACHMENT0, GL_STENCIL_ATTACHMENT};

    // The static casters come from the cache, the atlas holding its copy; otherwise it is cleared.
    if (cache) {
        rerendered = draw_cached_cascades(state);
    } else {
        release_target(m_static_shadow);
        m_static_keys = {};
        begin_shadow_target(m_shadow_atlas->id());
        glClearDepth(0);

        // The colour let go before the pass, the stencil cleared with the depth: nothing loaded.
        if (discard) {
            glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, unread);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        } else {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }

    vkgl::indexed_viewports(m_shadow_layout.rects);
    if (m_sun_ready) {
        for (unsigned i = 0; i < 4; ++i) {
            auto casters = cache ? ShadowCasters::Dynamic : ShadowCasters::All;
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
                i == 3 && !m_hero_ready,
                casters
            );
        }
    }
    if (m_hero_ready) {
        draw_shadow_map(
            m_shadow_atlas->id(),
            m_shadow_layout.rects[4][2],
            m_hero_matrix,
            true,
            false,
            state,
            4,
            4,
            true
        );
    }
    if (scrolling) {
        combine_scrolling_shadows(scrolling, state);
    }
    vkgl::indexed_viewports({});
    glBindVertexArray(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Only the depth is sampled: the colour and the stencil are not written back.
    if (discard) {
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, unread);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
    glViewport(0, 0, state.target.width, state.target.height);
    // What the atlas cost this frame, cache or not, for RENDER_PERF.md's R-18 comparison.
    stats::gauge("vk_shadow_draws", static_cast<double>(state.stats.draw_calls - draws_before));
    stats::gauge(
        "vk_shadow_triangles", static_cast<double>(state.stats.triangles - triangles_before)
    );
    stats::gauge("shadow_static_rerenders", static_cast<double>(rerendered));
#else
    (void)state;
#endif
}

unsigned LevelScene::draw_cached_cascades(renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    std::array<StaticShadowKey, kSunCascades> keys{};
    std::array<bool, kSunCascades> missed{};
    unsigned misses = 0;
    unsigned last = 0;

    // A fresh cache holds nothing: every cascade is drawn into it.
    if (!m_static_shadow || !m_static_shadow->id()) {
        m_static_keys = {};
    }

    // Each cascade's key: what its static casters depend on (sun_shadow.h).
    for (unsigned i = 0; i < kSunCascades; ++i) {
        const auto& volume = m_sun_cascades.casters[i];
        keys[i].valid = true;
        keys[i].matrix = m_sun_cascades.matrices[i];
        keys[i].planes = volume.planes;
        keys[i].plane_count = volume.plane_count;
        keys[i].separate_hero = m_hero_ready;
        keys[i].generation = m_static_generation;
        missed[i] = !same_static_key(keys[i], m_static_keys[i]);

        // A changed key draws again; the last one drawn flushes the buckets.
        if (missed[i]) {
            ++misses;
            last = i;
        }
    }

    // A cascade changed: its rectangle of the cache is cleared and its static casters drawn.
    if (misses > 0) {
        const bool fresh = !m_static_shadow || !m_static_shadow->id();

        // The cache is made as the atlas is, so that its depth format and size are the same.
        if (fresh) {
            m_static_shadow = std::make_unique<renderer::FrameBuffer>();
            std::string error;

            // No memory for it: the atlas draws every caster, as with the entry off.
            if (!m_static_shadow
                     ->create(m_shadow_layout.size, m_shadow_layout.size, error, 1, true)) {
                log::warn("static shadow cache unavailable: {}", error);
                release_target(m_static_shadow);
                begin_shadow_target(m_shadow_atlas->id());
                glClearDepth(0);
                glClear(GL_DEPTH_BUFFER_BIT);
                return kSunCascades;
            }
        }

        begin_shadow_target(m_static_shadow->id());
        glClearDepth(0);

        // A new cache is cleared whole, as the atlas is: its hero and unused texels stay 0.
        if (fresh) {
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        // An old one only where a cascade is drawn again.
        if (!fresh) {
            glEnable(GL_SCISSOR_TEST);

            // Each changed cascade's rectangle.
            for (unsigned i = 0; i < kSunCascades; ++i) {
                // Unchanged: its rectangle is kept.
                if (!missed[i]) {
                    continue;
                }

                const auto& rect = m_shadow_layout.rects[i];
                glScissor(rect[0], rect[1], rect[2], rect[3]);
                glClear(GL_DEPTH_BUFFER_BIT);
            }

            glDisable(GL_SCISSOR_TEST);
        }

        vkgl::indexed_viewports(m_shadow_layout.rects);

        // The static casters of each changed cascade, with the atlas's own shaders and views.
        for (unsigned i = 0; i < kSunCascades; ++i) {
            // Unchanged: copied from the cache as it is.
            if (!missed[i]) {
                continue;
            }

            draw_shadow_map(
                m_static_shadow->id(),
                m_shadow_layout.rects[i][2],
                m_sun_cascades.matrices[i],
                false,
                m_hero_ready,
                state,
                i,
                static_cast<int>(i),
                i == last,
                ShadowCasters::Static
            );
            m_static_keys[i] = keys[i];
        }

        vkgl::indexed_viewports({});
    }

    // The atlas starts as the cache: the static depth, 0 where nothing static is (unchanged
    // copy: vkCmdCopyImage of the depth aspect).
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_static_shadow->id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_atlas->id());
    glBlitFramebuffer(
        0,
        0,
        m_shadow_layout.size,
        m_shadow_layout.size,
        0,
        0,
        m_shadow_layout.size,
        m_shadow_layout.size,
        GL_DEPTH_BUFFER_BIT,
        GL_NEAREST
    );
    begin_shadow_target(m_shadow_atlas->id());
    return misses;
#else
    (void)state;
    return 0;
#endif
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
