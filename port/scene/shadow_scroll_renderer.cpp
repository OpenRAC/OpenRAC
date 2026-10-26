// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "scene/level_renderer.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::scene {
using namespace openrac::gl;

void LevelScene::release_scrolling_shadows() {
    for (auto& scroll : m_scroll_shadows) {
        if (scroll.statics) {
            scroll.statics->release();
        }
        glDeleteTextures(1, &scroll.combined);
        scroll = {};
    }
    m_scroll_requested = false;
}

void LevelScene::prepare_scrolling_shadows(
    renderer::Vec3 direction, renderer::Vec3 low, renderer::Vec3 high
) {
    m_scroll_requested = true;
    const bool force = !renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowScrollReuse);
    for (unsigned i = 0; i < kSunCascades; ++i) {
        auto& scroll = m_scroll_shadows[i];
        scroll.update = scroll.grid.update(
            m_sun_cascades.matrices[i],
            direction,
            low,
            high,
            m_shadow_layout.rects[i][2],
            m_static_generation,
            force
        );
        m_sun_cascades.matrices[i] = scroll.update.matrix;
        // Cached texels cannot depend on the current camera's receiver footprint.
        m_sun_cascades.casters[i] = {};
    }
}

bool LevelScene::make_scrolling_target(unsigned cascade) {
#if defined(OPENRAC_VULKAN)
    auto& scroll = m_scroll_shadows[cascade];
    const int size = m_shadow_layout.rects[cascade][2];
    if (!scroll.statics) {
        scroll.statics = std::make_unique<renderer::FrameBuffer>();
    }
    const bool fresh = !scroll.statics->id() || !scroll.combined;
    std::string error;
    if (!scroll.statics->id() && !scroll.statics->create(size, size, error, 1, true)) {
        log::warn("scrolling shadow cache {} unavailable: {}", cascade, error);
        scroll.grid.invalidate();
        return false;
    }
    if (!scroll.combined) {
        glGenTextures(1, &scroll.combined);
        glBindTexture(GL_TEXTURE_2D, scroll.combined);
        if (!vkgl::allocate_storage_texture(scroll.combined, 0x822E, size, size)) {  // R32F
            glDeleteTextures(1, &scroll.combined);
            scroll.combined = 0;
            scroll.grid.invalidate();
            return false;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (fresh) {
        scroll.update.full = true;
        scroll.update.regions =
            shadow_scroll_regions(scroll.update.origin, scroll.update.origin, size, true);
    }
    return true;
#else
    (void)cascade;
    return false;
#endif
}

void LevelScene::draw_scrolling_regions(unsigned cascade, renderer::RenderState& state) {
    const auto& scroll = m_scroll_shadows[cascade];
    const int size = m_shadow_layout.rects[cascade][2];
    for (const auto& region : scroll.update.regions) {
        draw_shadow_map(
            scroll.statics->id(),
            size,
            shadow_scroll_matrix(scroll.update, region, size),
            false,
            m_hero_ready,
            state,
            kSunCascades,
            -1,
            false,
            ShadowCasters::StaticOpaque,
            &region.physical
        );
    }
}

unsigned LevelScene::update_scrolling_shadows(renderer::RenderState& state) {
    const stats::Scope scope("shadow scroll static", stats::kGpu);
    static constexpr const char* names[] =
        {"shadow scroll 0 updated texels",
         "shadow scroll 1 updated texels",
         "shadow scroll 2 updated texels",
         "shadow scroll 3 updated texels"};
    std::uint64_t pixels = 0, regions = 0, capacity = 0;
    unsigned mask = 0;
    bool full = false;
    for (unsigned i = 0; i < kSunCascades; ++i) {
        if (!make_scrolling_target(i)) {
            continue;
        }
        draw_scrolling_regions(i, state);
        mask |= 1u << i;
        const auto& update = m_scroll_shadows[i].update;
        std::uint64_t changed = 0;
        for (const auto& region : update.regions) {
            changed += static_cast<std::uint64_t>(region.physical[2]) * region.physical[3];
        }
        pixels += changed;
        regions += update.regions.size();
        full = full || update.full;
        capacity +=
            static_cast<std::uint64_t>(m_shadow_layout.rects[i][2]) * m_shadow_layout.rects[i][3];
        stats::gauge(names[i], static_cast<double>(changed));
    }
    stats::gauge("shadow scroll updated texels", static_cast<double>(pixels));
    stats::gauge("shadow scroll capacity texels", static_cast<double>(capacity));
    stats::gauge("shadow scroll regions", static_cast<double>(regions));
    stats::gauge("shadow scroll full refresh", full ? 1 : 0);
    return mask;
}

void LevelScene::combine_scrolling_shadows(unsigned mask, renderer::RenderState& state) {
#if defined(OPENRAC_VULKAN)
    const stats::Scope scope("shadow scroll combine", stats::kGpu);
    unsigned active = 0;
    for (unsigned i = 0; i < kSunCascades; ++i) {
        if (!(mask & (1u << i))) {
            continue;
        }
        auto& scroll = m_scroll_shadows[i];
        scroll.active = vkgl::combine_shadow_depth(
            scroll.statics->depth_texture(),
            m_shadow_atlas->depth_texture(),
            scroll.combined,
            scroll.update.offset,
            m_shadow_layout.rects[i]
        );
        // Restore static casters in this atlas rectangle if composition fails.
        if (!scroll.active) {
            draw_shadow_map(
                m_shadow_atlas->id(),
                m_shadow_layout.rects[i][2],
                m_sun_cascades.matrices[i],
                false,
                m_hero_ready,
                state,
                i,
                static_cast<int>(i),
                true,
                ShadowCasters::StaticOpaque
            );
            scroll.grid.invalidate();
        } else {
            ++active;
        }
    }
    stats::gauge("shadow scroll active", active == kSunCascades ? 1 : 0);
    stats::gauge("shadow scroll active cascades", active);
#else
    (void)mask;
    (void)state;
#endif
}
}  // namespace openrac::scene
