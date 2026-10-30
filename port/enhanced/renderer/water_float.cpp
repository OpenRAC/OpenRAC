// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/water_float.h"

#include <array>
#include <utility>

#include "enhanced/renderer/water.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"

namespace openrac::enhanced {
using namespace gl;

WaterFloat::~WaterFloat() {
    release();
}

std::string WaterFloat::vertex_source(std::string source) {
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return source;
    }
    source.replace(0, source.find('\n'), "#version 430 core");
    source.insert(source.find('\n') + 1, "#define OPENRAC_WATER_FLOAT 1\n");
    std::string helpers = std::string(lighting::shaders::water_window_glsl) + "\n"
                          + std::string(lighting::shaders::water_swell_glsl) + "\n"
                          + std::string(lighting::shaders::water_displacement_glsl) + "\n"
                          + std::string(lighting::shaders::water_float_glsl) + "\n";
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderVariants)
        && renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader)) {
        helpers.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        helpers = renderer::shader_constants(std::move(helpers), {{"water_sim_local", 1}});
    }
    source.insert(source.find("void main()"), helpers);
    return source;
}

void WaterFloat::load() {
    release();
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return;
    }
    const std::array<float, 4> zero{};
    glGenBuffers(1, &m_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(zero), zero.data(), GL_STATIC_DRAW);
    glGenTextures(1, &m_bodies);
    glActiveTexture(GL_TEXTURE0 + 18);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_bodies);
    glTexBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, renderer::world::gl::GL_RGBA32F, m_buffer);
    glGenTextures(1, &m_map);
    glActiveTexture(GL_TEXTURE0 + 19);
    glBindTexture(GL_TEXTURE_2D, m_map);
    glTexImage2D(
        GL_TEXTURE_2D, 0, renderer::world::gl::GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, zero.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);
}

void WaterFloat::prepare(const renderer::FrameInput& input) {
    if (m_frame == input.frame && m_seconds == input.seconds
        && m_requested == input.enhanced_water) {
        return;
    }
    m_frame = input.frame;
    m_seconds = input.seconds;
    m_requested = input.enhanced_water;
    m_bound_shaders.clear();
    prepare_water_deformation(input);
}

void WaterFloat::bind(const renderer::Shader& shader, bool enabled) const {
    if (shader.uniform("water_float_enabled") < 0 || !m_buffer) {
        return;
    }
    // Shared maps and phase are constant within one displayed frame; only the group flag changes.
    if (m_bound_shaders.insert(shader.id()).second) {
        glUniform1i(shader.uniform("water_bodies"), 18);
        glUniform1i(shader.uniform("water_coast_map"), 19);
        glUniform1i(shader.uniform("water_wave_map"), 20);
        // Vulkan validates unused branches too; dry levels still require valid descriptors.
        glActiveTexture(GL_TEXTURE0 + 18);
        glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_bodies);
        for (unsigned unit : {19u, 20u}) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, m_map);
        }
        glUniform1i(shader.uniform("water_body_count"), 0);
        bind_water_deformation(shader);

        glActiveTexture(GL_TEXTURE0);
    }
    glUniform1i(shader.uniform("water_float_enabled"), enabled && water_available() ? 1 : 0);
}

void WaterFloat::release() {
    glDeleteTextures(1, &m_bodies);
    glDeleteTextures(1, &m_map);
    glDeleteBuffers(1, &m_buffer);
    m_buffer = m_bodies = m_map = 0;
    m_bound_shaders.clear();
    m_frame = ~std::uint64_t{0};
    m_requested = false;
    m_seconds = -1;
}
}  // namespace openrac::enhanced
