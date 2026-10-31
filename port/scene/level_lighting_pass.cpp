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

bool LevelScene::needs_early_lighting(const renderer::FrameInput& input) const {
    return probe_data_available() && m_early_capable && m_lighting->early_supported()
           && m_lighting->mode() != 4
           && (m_lighting->active() || (m_sun_enabled && input.sun_pixel_shadow)
               || (m_hero_enabled && input.hero_pixel_shadow));
}

void LevelScene::bind_shadow_atlas(const renderer::Shader& shader) const {
    glUniform1i(shader.uniform("shadow_atlas_enabled"), m_shadow_atlas_active ? 1 : 0);
    if (m_shadow_atlas_active) {
        const auto packed = [this] {
            std::array<float, 20> result{};
            for (unsigned i = 0; i < 5; ++i) {
                for (unsigned j = 0; j < 4; ++j) {
                    result[4 * i + j] = static_cast<float>(m_shadow_layout.rects[i][j]);
                }
            }
            return result;
        }();
        glUniform4fv(shader.uniform("shadow_atlas_rects"), 5, packed.data());
    }
}

void LevelScene::bind_hero_shadow(const renderer::Shader& shader) const {
    if (!enhanced_rendering_available()) {
        return;
    }
    glUniform1i(shader.uniform("hero_shadow_enabled"), m_hero_ready ? 1 : 0);
    bind_shadow_atlas(shader);
    glUniform1i(shader.uniform("hero_shadow_airborne"), m_hero_airborne ? 1 : 0);
    glUniform1f(shader.uniform("hero_shadow_air_blend"), m_hero_transition.value);
    glUniform1i(shader.uniform("hero_shadow_map"), 6);
    glUniformMatrix4fv(shader.uniform("hero_shadow_matrix"), 1, GL_FALSE, m_hero_matrix.data());
    if (m_hero_ready) {
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(
            GL_TEXTURE_2D,
            m_shadow_atlas_active ? m_shadow_atlas->depth_texture() : m_hero_map->depth_texture()
        );
        glActiveTexture(GL_TEXTURE0);
    }
}

std::string LevelScene::atmosphere_source() const {
    return m_lighting ? m_lighting->atmosphere_source() : std::string{};
}

void LevelScene::bind_atmosphere(const renderer::Shader& shader) const {
    if (m_lighting) {
        m_lighting->bind_atmosphere(shader);
    }
}

void LevelScene::bind_surface_lighting(
    const renderer::Shader& shader, const renderer::Camera& camera
) const {
    if (m_light_sets.size() == std::size_t{16} * 16) {
        glUniform4fv(shader.uniform("light_sets"), 64, m_light_sets.data());
    }
    std::array<float, 32> positions{}, colours{};
    for (std::size_t i = 0; i < 8; ++i) {
        std::copy_n(m_point_lights.data() + i * 8, 4, colours.data() + i * 4);
        std::copy_n(m_point_lights.data() + i * 8 + 4, 4, positions.data() + i * 4);
    }
    glUniform4fv(shader.uniform("probe_point_position"), 8, positions.data());
    glUniform4fv(shader.uniform("probe_point_colour"), 8, colours.data());
    if (m_lighting) {
        m_lighting->bind(shader, Layer::Terrain, true, &camera, false);
    }
    bind_sun_shadow(shader, camera);
}

void LevelScene::bind_sun_shadow(
    const renderer::Shader& shader, const renderer::Camera& camera
) const {
    if (!enhanced_rendering_available()) {
        return;
    }
    glUniform1i(shader.uniform("sun_shadow_enabled"), m_sun_ready ? 1 : 0);
    int separate = 0;
    for (unsigned i = 0; i < kSunCascades; ++i) {
        if (m_scroll_shadows[i].active) {
            separate |= 1 << i;
        }
    }
    glUniform1i(shader.uniform("sun_shadow_separate_maps"), separate);
    bind_shadow_atlas(shader);
    glUniform4fv(shader.uniform("sun_shadow_ends"), 1, m_sun_cascades.ends.data());
    glUniform1f(shader.uniform("sun_shadow_near"), m_sun_cascades.near_distance);
    glUniformMatrix4fv(shader.uniform("sun_shadow_view"), 1, GL_FALSE, camera.view.data());
    const char* map_names[] =
        {"sun_shadow_map0", "sun_shadow_map1", "sun_shadow_map2", "sun_shadow_map3"};
    const char* matrix_names[] =
        {"sun_shadow_matrix[0]",
         "sun_shadow_matrix[1]",
         "sun_shadow_matrix[2]",
         "sun_shadow_matrix[3]"};
    for (unsigned i = 0; i < kSunCascades; ++i) {
        glUniform1i(shader.uniform(map_names[i]), static_cast<GLint>(7 + i));
        glUniformMatrix4fv(
            shader.uniform(matrix_names[i]), 1, GL_FALSE, m_sun_cascades.matrices[i].data()
        );
        if (m_sun_ready) {
            glActiveTexture(GL_TEXTURE0 + 7 + i);
            unsigned texture = m_scroll_shadows[i].combined;
            if (!m_scroll_shadows[i].active) {
                texture = m_shadow_atlas_active ? m_shadow_atlas->depth_texture()
                                                : m_sun_maps[i]->depth_texture();
            }
            glBindTexture(GL_TEXTURE_2D, texture);
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

void LevelScene::render_early_lighting(
    unsigned depth, const renderer::FrameInput& input, renderer::RenderState& state
) {
    // Atmosphere may already have drawn the current maps before the sky. No scene or
    // camera update occurs between that preparation and this frame's early lighting.
    if (!state.world_shadows_prepared
        || !renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowFrameReuse)) {
        prepare_shadows(input, state);
    }
    state.world_shadows_prepared = true;
    std::string error;
    // Drawn only where it is needed (needs_early_lighting), which takes registered lighting.
    state.early_lighting =
        m_lighting
        && m_lighting->render_early(
            depth,
            input,
            state.target,
            [&](const renderer::Shader& shader) {
                m_lighting->bind(shader, Layer::Terrain, true, &input.camera, true);
                bind_sun_shadow(shader, input.camera);
                bind_hero_shadow(shader);
                glUniform1i(shader.uniform("early_ao_enabled"), state.gtao_raw != 0 ? 1 : 0);
                glUniform1i(shader.uniform("early_ao"), 18);
                glUniform1i(shader.uniform("early_ao_scale"), state.gtao_scale);
                glUniform1f(
                    shader.uniform("early_ao_strength"), std::clamp(state.gtao_strength, 0.0f, 3.0f)
                );
                glActiveTexture(GL_TEXTURE0 + 18);
                glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, state.gtao_raw);
                glActiveTexture(GL_TEXTURE0);
            },
            error
        );
    stats::gauge("early lighting active", state.early_lighting ? 1 : 0);
    if (!error.empty()) {
        log::warn("early lighting: {}", error);
    }
}
}  // namespace openrac::scene
