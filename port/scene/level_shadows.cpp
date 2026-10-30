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

bool LevelScene::make_dry_shadow_programs(bool atlas, std::string& error) {
    auto& programs = atlas ? m_dry_atlas : m_dry_shadow;
    for (unsigned solid = 0; solid < programs.size(); ++solid) {
        if (programs[solid].id()) {
            continue;
        }
        // Positions, palettes and alpha coverage are unchanged. Only water deformation,
        // which is disabled for these groups, is absent from the vertex program.
        std::string vertex =
            m_lighting ? m_lighting->dry_shadow_vertex(std::string(shaders::shadow_depth_vert))
                       : std::string(shaders::shadow_depth_vert);
        std::string fragment(shaders::hero_shadow_frag);
        if (atlas) {
            vertex.insert(
                vertex.find('\n') + 1,
                "#extension GL_ARB_shader_viewport_layer_array : require\n"
                "#define OPENRAC_SHADOW_ATLAS 1\n"
            );
        }
        if (solid) {
            vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
            fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
        }
        if (!programs[solid].build(
                std::string(atlas ? "dry atlas shadow " : "dry shadow ")
                    + (solid ? "solid" : "alpha"),
                vertex,
                fragment,
                error
            )) {
            for (auto& shader : programs) {
                shader.release();
            }
            return false;
        }
    }
    return true;
}

bool LevelScene::enable_hero_shadow(bool enabled, std::string& error) {
    if (enabled && (!enhanced_rendering_available() || !probe_data_available())) {
        error = "Modern shadows require Vulkan, an enhanced build and validated probe data";
        return false;
    }
    m_hero_ready = false;
    m_hero_enabled = false;
    if (!enabled) {
        return true;
    }
    if (!m_hero_depth.id()
        && !m_hero_depth.build(
            "Ratchet shadow depth",
            deformation_vertex(shaders::shadow_depth_vert),
            shaders::hero_shadow_frag,
            error
        )) {
        return false;
    }
    std::string dry_error;
    if (!make_dry_shadow_programs(false, dry_error)) {
        log::warn("Dry shadow variants unavailable: {}", dry_error);
    }
    if (!m_hero_map) {
        m_hero_map = std::make_unique<renderer::FrameBuffer>();
    }
    if (!m_hero_map->id()
        && !m_hero_map->create(
            m_shadow_layout.rects[4][2], m_shadow_layout.rects[4][3], error, 1, true
        )) {
        return false;
    }
    m_hero_enabled = true;
    return true;
}

// Recreate all dependent targets together; no cached depth survives a resolution change.
void LevelScene::set_shadow_quality(renderer::ShadowQuality quality) {
    const auto layout = shadow_atlas_layout(quality);
    if (layout.size == m_shadow_layout.size) {
        return;
    }
    release_target(m_shadow_atlas);
    release_target(m_static_shadow);
    release_target(m_hero_map);
    for (auto& map : m_sun_maps) {
        release_target(map);
    }
    release_scrolling_shadows();
    m_static_keys = {};
    m_shadow_layout = layout;
}

void LevelScene::prepare_shadows(const renderer::FrameInput& input, renderer::RenderState& state) {
    m_shadow_geometry_indices = {};
    set_shadow_quality(input.shadow_quality);
    m_shadow_atlas_active = false;
#if defined(OPENRAC_VULKAN)
    static const bool& atlas_off = stats::probe(
        "OPENRAC_SHADOW_ATLAS_OFF", "Reference: separate shadow targets and per-map submission"
    );
    static const bool& layer_reference = stats::probe(
        "OPENRAC_SHADOW_LAYER_REFERENCE", "Reference: flush shadow buckets after each layer"
    );
    static const bool& detail = stats::probe(
        "OPENRAC_SUN_TIMING_DETAIL", "Detailed cascade/layer GPU timers (higher profiling overhead)"
    );
    if (!atlas_off && !layer_reference && !detail && (m_sun_enabled || m_hero_enabled)
        && probe_data_available() && gpu_driven() && vkgl::indexed_viewports_supported(5)) {
        std::string error;
        m_shadow_atlas_active = make_shadow_atlas(error);
        if (!m_shadow_atlas_active) {
            log::warn("shadow atlas unavailable: {}", error);
        }
    }
#endif
    if (!m_shadow_atlas_active) {
        release_target(m_shadow_atlas);
        release_target(m_static_shadow);
        m_static_keys = {};
    }
    prepare_hero_shadow(input, state);
    prepare_sun_shadow(input, state);
    if (m_shadow_atlas_active && (m_hero_ready || m_sun_ready)) {
        draw_shadow_atlas(state);
        report_sun_culling();
    }
    stats::gauge("shadow atlas", m_shadow_atlas_active ? 1 : 0);
    stats::gauge("shadow atlas size", m_shadow_layout.size);
    stats::gauge("sun shadow near size", m_shadow_layout.rects[0][2]);
    stats::gauge("hero shadow size", m_shadow_layout.rects[4][2]);
    stats::gauge("shadow geometry tested indices", double(m_shadow_geometry_indices[0]));
    stats::gauge("shadow geometry rejected indices", double(m_shadow_geometry_indices[1]));
}

void LevelScene::prepare_hero_shadow(
    const renderer::FrameInput& input, renderer::RenderState& state
) {
    m_hero_ready = false;
    if (!probe_data_available() || !m_hero_enabled || !input.hero_pixel_shadow || !m_hero_centre
        || m_lighting->mode() == 2 || m_lighting->mode() == 4) {
        m_hero_transition = {};
        return;
    }
    renderer::Vec3 direction{0, 0, -1};
    if (m_light_sets.size() == 256) {
        std::copy_n(m_light_sets.data() + 4, 3, direction.data());
    }
    direction = m_lighting->sunlight_direction(direction);
    const float blend = hero_shadow_blend(input.hero_airborne, input.seconds, m_hero_transition);
    const auto projection = hero_shadow_projection(*m_hero_centre, direction, blend);
    m_hero_matrix = projection.matrix;
    m_hero_airborne = input.hero_airborne;
    if (!m_hero_logged_airborne || *m_hero_logged_airborne != m_hero_airborne) {
        log::info(
            "Ratchet shadow map: {}, travel direction {:.3f} {:.3f} {:.3f}",
            m_hero_airborne ? "airborne/downward" : "grounded/main light",
            projection.direction[0],
            projection.direction[1],
            projection.direction[2]
        );
        m_hero_logged_airborne = m_hero_airborne;
    }
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!m_shadow_atlas_active) {
        if (!m_hero_map) {
            m_hero_map = std::make_unique<renderer::FrameBuffer>();
        }
        std::string error;
        if (!m_hero_map->id()
            && !m_hero_map->create(
                m_shadow_layout.rects[4][2], m_shadow_layout.rects[4][3], error, 1, true
            )) {
            log::warn("hero shadow target unavailable: {}", error);
            return;
        }
        draw_shadow_map(
            m_hero_map->id(), m_shadow_layout.rects[4][2], m_hero_matrix, true, false, state
        );
    }
    m_hero_ready = true;
}

bool LevelScene::enable_sun_shadow(bool enabled, std::string& error) {
    if (enabled && (!enhanced_rendering_available() || !probe_data_available())) {
        error = "Modern shadows require Vulkan, an enhanced build and validated probe data";
        return false;
    }
    m_sun_enabled = m_sun_ready = false;
    if (!enabled) {
        return true;
    }
    if (!m_hero_depth.id()
        && !m_hero_depth.build(
            "modern shadow depth",
            deformation_vertex(shaders::shadow_depth_vert),
            shaders::hero_shadow_frag,
            error
        )) {
        return false;
    }
    if (!m_shadow_solid.id()) {
        std::string vertex(deformation_vertex(shaders::shadow_depth_vert)),
            fragment(shaders::hero_shadow_frag);
        vertex.insert(vertex.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
        fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_SHADOW_SOLID 1\n");
        if (!m_shadow_solid.build("opaque static shadow depth", vertex, fragment, error)) {
            return false;
        }
    }
    std::string dry_error;
    if (!make_dry_shadow_programs(false, dry_error)) {
        log::warn("Dry shadow variants unavailable: {}", dry_error);
    }
    for (unsigned i = 0; i < kSunCascades; ++i) {
        auto& map = m_sun_maps[i];
        if (!map) {
            map = std::make_unique<renderer::FrameBuffer>();
        }
        if (!map->id()
            && !map->create(
                m_shadow_layout.rects[i][2], m_shadow_layout.rects[i][2], error, 1, true
            )) {
            return false;
        }
    }
    m_sun_enabled = true;
    return true;
}

void LevelScene::prepare_sun_shadow(
    const renderer::FrameInput& input, renderer::RenderState& state
) {
    m_sun_ready = false;
    m_scroll_requested = false;
    for (auto& scroll : m_scroll_shadows) {
        scroll.active = false;
    }
    m_sun_culling = {};
    if (!probe_data_available() || !m_sun_enabled || !input.sun_pixel_shadow
        || m_lighting->mode() == 2 || m_lighting->mode() == 4) {
        return;
    }
    renderer::Vec3 direction{0, 0, -1}, low{3e38f, 3e38f, 3e38f}, high{-3e38f, -3e38f, -3e38f};
    if (m_light_sets.size() == 256) {
        std::copy_n(m_light_sets.data() + 4, 3, direction.data());
    }
    for (std::size_t layer = 1; layer < kLayerCount; ++layer) {
        for (const auto& group : m_groups[layer]) {
            if (!group.bounded) {
                continue;
            }
            for (int k = 0; k < 3; ++k) {
                low[k] = std::min(low[k], group.low[k]);
                high[k] = std::max(high[k], group.high[k]);
            }
        }
    }
    try {
        direction = m_lighting->sunlight_direction(direction);
        m_sun_cascades =
            sun_cascades(input.camera, direction, low, high, 256, input.shadow_quality);
#if defined(OPENRAC_VULKAN)
        static const bool& scrolling = stats::probe(
            "OPENRAC_SCROLLING_SHADOWS", "Experimental toroidal static caches for the sun cascades"
        );
        static const bool& scrolling_off = stats::probe(
            "OPENRAC_SCROLLING_SHADOWS_OFF", "Reference: the ordinary atlas on a tile-based GPU"
        );
        // A tile-based GPU bins every caster of all four cascades each frame, which costs it far
        // more than the cache's composition: there the caches are the default.
        const bool tiled = vkgl::tile_based() && !scrolling_off;
        if ((scrolling || tiled) && gl::is_vulkan() && m_shadow_atlas_active) {
            prepare_scrolling_shadows(direction, low, high);
        }
#endif
    } catch (const std::exception&) {
        // A partial grid update must not survive a frame that never rendered its dirty strips.
        for (auto& scroll : m_scroll_shadows) {
            scroll.grid.invalidate();
        }
        return;
    }  // invalid/transitional camera: leave surfaces lit
    for (unsigned i = 0; i < kSunCascades; ++i) {
        glActiveTexture(GL_TEXTURE0 + 7 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (!m_shadow_atlas_active) {
        for (unsigned i = 0; i < kSunCascades; ++i) {
            if (!m_sun_maps[i]) {
                m_sun_maps[i] = std::make_unique<renderer::FrameBuffer>();
            }
            std::string error;
            if (!m_sun_maps[i]->id()
                && !m_sun_maps[i]->create(
                    m_shadow_layout.rects[i][2], m_shadow_layout.rects[i][2], error, 1, true
                )) {
                log::warn("sun shadow target unavailable: {}", error);
                return;
            }
            draw_shadow_map(
                m_sun_maps[i]->id(),
                m_shadow_layout.rects[i][2],
                m_sun_cascades.matrices[i],
                false,
                m_hero_ready,
                state,
                i
            );
        }
    }
    m_sun_ready = true;
    if (!m_shadow_atlas_active) {
        report_sun_culling();
    }
}

void LevelScene::report_sun_culling() const {
    static constexpr const char* kNames[] =
        {"sun cascade 0 culled",
         "sun cascade 1 culled",
         "sun cascade 2 culled",
         "sun cascade 3 culled"};
    std::uint64_t candidates = 0, rejected = 0, submitted = 0, compacted = 0, unbounded = 0;
    std::uint64_t solid_draws = 0, alpha_draws = 0;
    for (unsigned i = 0; i < kSunCascades; ++i) {
        const auto& count = m_sun_culling[i];
        stats::gauge(kNames[i], static_cast<double>(count.rejected));
        candidates += count.candidates;
        rejected += count.rejected;
        submitted += count.submitted;
        compacted += count.compacted_groups;
        unbounded += count.unbounded;
        solid_draws += count.solid_draws;
        alpha_draws += count.alpha_draws;
    }
    stats::gauge("sun instance candidates", static_cast<double>(candidates));
    stats::gauge("sun instances culled", static_cast<double>(rejected));
    stats::gauge("sun instances submitted", static_cast<double>(submitted));
    stats::gauge("sun solid draws", static_cast<double>(solid_draws));
    stats::gauge("sun alpha draws", static_cast<double>(alpha_draws));
    stats::gauge("sun compacted groups", static_cast<double>(compacted));
    stats::gauge("sun unbounded instances", static_cast<double>(unbounded));
}
}  // namespace openrac::scene
