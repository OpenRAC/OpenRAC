// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Static shadow-cache decisions and graph-backed updates/copies, retaining the existing cascade keys.
#include <memory>
#include <stdexcept>
#include <string>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl/gl.h"
#include "scene/level_renderer.h"
#include "scene/level_renderer_internal.h"
#include "scene/shadow_graph.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::scene {
using namespace gl;

LevelScene::StaticShadowUpdate LevelScene::static_shadow_update() const {
    StaticShadowUpdate update;
    update.fresh = !m_static_shadow || !m_static_shadow->id();

    unsigned matrix_changes = 0, volume_changes = 0, generation_changes = 0;

    // Every changed caster view invalidates its rectangle independently of the other cascades.
    for (unsigned i = 0; i < kSunCascades; ++i) {
        const auto& volume = m_sun_cascades.casters[i];
        auto& key = update.keys[i];
        key.valid = true;
        key.matrix = m_sun_cascades.matrices[i];
        key.planes = volume.planes;
        key.plane_count = volume.plane_count;
        key.separate_hero = m_hero_ready;
        key.generation = m_static_generation;
        const auto& previous = m_static_keys[i];
        matrix_changes += key.matrix != previous.matrix;
        volume_changes += key.planes != previous.planes || key.plane_count != previous.plane_count;
        generation_changes += key.generation != previous.generation;
        update.missed[i] = update.fresh || !same_static_key(key, previous);

        // The last changed cascade flushes the same shared atlas draw buckets as before.
        if (update.missed[i]) {
            ++update.count;
            update.last = i;
        }
    }

    stats::gauge("shadow static matrix changes", matrix_changes);
    stats::gauge("shadow static volume changes", volume_changes);
    stats::gauge("shadow static generation changes", generation_changes);
    stats::gauge("shadow static fresh target", update.fresh ? 1 : 0);
    return update;
}

bool LevelScene::make_static_shadow_target() {
    // A live cache already has the atlas's dimensions and depth format.
    if (m_static_shadow && m_static_shadow->id()) {
        return true;
    }

    m_static_shadow = std::make_unique<renderer::FrameBuffer>();
    std::string error;

    // Target creation failure selects the atlas's all-caster fallback.
    if (!render_detail::create_shadow_target(*m_static_shadow, m_shadow_layout.size, error)) {
        log::warn("static shadow cache unavailable: {}", error);
        render_detail::release_target(m_static_shadow);
        return false;
    }

    return true;
}

void LevelScene::clear_static_shadow_updates(const StaticShadowUpdate& update) {
    begin_shadow_target(m_static_shadow->id());
    glClearDepth(0);

    // A fresh allocation also needs defined empty hero and unused atlas rectangles.
    if (update.fresh) {
        glClear(GL_DEPTH_BUFFER_BIT);
        return;
    }

    glEnable(GL_SCISSOR_TEST);

    // Changed rectangles are cleared without overwriting any retained static depth.
    for (unsigned i = 0; i < kSunCascades; ++i) {
        // Unchanged rectangles retain the previous frame's static depth.
        if (!update.missed[i]) {
            continue;
        }

        const auto& rect = m_shadow_layout.rects[i];
        glScissor(rect[0], rect[1], rect[2], rect[3]);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    glDisable(GL_SCISSOR_TEST);
}

void LevelScene::record_static_shadow_updates(
    const StaticShadowUpdate& update, renderer::RenderState& state
) {
#if defined(OPENRAC_VULKAN)
    clear_static_shadow_updates(update);
    vkgl::indexed_viewports(m_shadow_layout.rects);

    // Keys are committed only after the phase's existing draw buckets have been recorded.
    for (unsigned i = 0; i < kSunCascades; ++i) {
        // Unchanged rectangles retain the previous frame's static depth.
        if (!update.missed[i]) {
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
            i == update.last,
            ShadowCasters::Static
        );
    }

    vkgl::indexed_viewports({});
#else
    (void)update;
    (void)state;
#endif
}

void LevelScene::copy_static_shadow_depth() {
    // The callback records the unchanged full-depth copy after its graph transfer barriers.
    const auto copied = shadow_depth_copy(m_static_shadow->id(), m_shadow_atlas->id(), [this] {
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
    });

    // Do not use the dynamic atlas if its required static-depth copy was not recorded.
    if (!copied) {
        throw std::runtime_error(copied.error().message);
    }
}

unsigned LevelScene::draw_cached_cascades(renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    const auto update = static_shadow_update();

    // A warm cache has no writer this frame; its imported contents feed the copy directly.
    if (update.count) {
        // The caller detects the absent cache and renders every caster into a cleared atlas.
        if (!make_static_shadow_target()) {
            return kSunCascades;
        }

        // Partial updates preserve the complete valid image surrounding their dirty rectangles.
        const auto drawn = shadow_depth_pass(
            "static shadow cache updates",
            m_static_shadow->id(),
            update.fresh ? ShadowDepthContents::Clear : ShadowDepthContents::Preserve,
            [this, &state, update] {
                record_static_shadow_updates(update, state);
                // Publish only after the queued writer has recorded its complete updates.
                m_static_keys = update.keys;
            },
            ShadowDepthConsumer::Copy
        );

        // Cache keys must not advance after a rejected or incomplete update.
        if (!drawn) {
            throw std::runtime_error(drawn.error().message);
        }
    }

    copy_static_shadow_depth();
    return update.count;
#else
    (void)state;
    return 0;
#endif
}
}  // namespace openrac::scene
