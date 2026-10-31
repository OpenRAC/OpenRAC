// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/water_float.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "common/log.h"
#include "enhanced/renderer/water.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced {
using namespace gl;

namespace {
// Environment selection uses 21-23 and atmospheric transport uses 24.
constexpr unsigned kPoseUnit = 25;
}  // namespace

WaterFloat::~WaterFloat() {
    release();
}

std::string WaterFloat::vertex_source(std::string source) {
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return source;
    }
    source.replace(0, source.find('\n'), "#version 430 core");
    source.insert(
        source.find('\n') + 1,
        "#define OPENRAC_WATER_FLOAT 1\n#define OPENRAC_WATER_FLOAT_FILTERED 1\n"
    );
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
    std::string source(lighting::shaders::water_float_comp);
    const std::string helpers = std::string(lighting::shaders::water_window_glsl) + "\n"
                                + std::string(lighting::shaders::water_swell_glsl) + "\n"
                                + std::string(lighting::shaders::water_displacement_glsl) + "\n"
                                + std::string(lighting::shaders::water_float_glsl) + "\n";
    source.insert(source.find("void main()"), helpers);
    std::string error;
    if (!m_filter.build_compute("water attachment momentum", source, error)) {
        log::warn("{}", error);
        return;
    }
    glGenBuffers(1, &m_anchor_buffer);
    glGenBuffers(2, m_pose_buffers);
    glGenTextures(2, m_pose_textures);
    for (unsigned i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, m_pose_buffers[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(zero), zero.data(), GL_DYNAMIC_DRAW);
        glActiveTexture(GL_TEXTURE0 + kPoseUnit);
        glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_pose_textures[i]);
        glTexBuffer(
            renderer::world::gl::GL_TEXTURE_BUFFER,
            renderer::world::gl::GL_RGBA32F,
            m_pose_buffers[i]
        );
    }
    glActiveTexture(GL_TEXTURE0);
}

void WaterFloat::prepare(const renderer::FrameInput& input, const scene::LevelScene* scene) {
    const float momentum =
        std::isfinite(input.lily_momentum) ? std::clamp(input.lily_momentum, 0.f, 1.f) : .25f;
    if (m_frame == input.frame && m_seconds == input.seconds && m_requested == input.enhanced_water
        && m_momentum == momentum) {
        return;
    }
    // Follow the water simulation's 60 Hz game clock, including pauses and slow motion.
    const bool reset =
        m_seconds < 0 || input.frame < m_frame || input.frame - m_frame > 15 || !m_requested;
    const double dt =
        reset || water_debug_settings().paused ? 0. : double(input.frame - m_frame) / 60.;
    m_frame = input.frame;
    m_seconds = input.seconds;
    m_requested = input.enhanced_water;
    m_momentum = momentum;
    m_bound_shaders.clear();
    prepare_water_deformation(input);
    m_pose_count = 0;
    if (!scene || !m_filter.id() || !input.enhanced_water || !water_available()) {
        m_previous.clear();
        m_previous_ids.clear();
        return;
    }
    scene->floating_anchors(m_anchors);
    std::sort(m_anchors.begin(), m_anchors.end());
    m_anchors.erase(
        std::unique(
            m_anchors.begin(),
            m_anchors.end(),
            [](const auto& a, const auto& b) { return a.first == b.first; }
        ),
        m_anchors.end()
    );
    m_records.clear();
    for (const auto& attachment : m_anchors) {
        const auto& [anchor, identity] = attachment;
        float index = -1;
        if (!reset) {
            if (identity) {
                if (const auto previous = m_previous_ids.find(identity);
                    previous != m_previous_ids.end()) {
                    const auto& old = m_previous[previous->second].first;
                    float distance_squared = 0;
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        distance_squared += (anchor[axis] - old[axis]) * (anchor[axis] - old[axis]);
                    }
                    // Preserve native lily drift, but never drag a pose across a teleport.
                    if (distance_squared < 4) {
                        index = float(previous->second);
                    }
                }
            } else {
                const auto previous =
                    std::lower_bound(m_previous.begin(), m_previous.end(), attachment);
                if (previous != m_previous.end() && *previous == attachment) {
                    index = float(previous - m_previous.begin());
                }
            }
        }
        m_records.push_back({anchor[0], anchor[1], anchor[2], index});
    }
    if (!m_records.empty()) {
        m_filter.use();
        bind_maps(m_filter);
        glUniform1i(m_filter.uniform("previous_poses"), kPoseUnit);
        glActiveTexture(GL_TEXTURE0 + kPoseUnit);
        glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_pose_textures[m_current]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_anchor_buffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(m_records.size() * sizeof(m_records[0])),
            m_records.data(),
            GL_STREAM_DRAW
        );
        m_current = 1 - m_current;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pose_buffers[m_current]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            GLsizeiptr(m_records.size() * 8 * sizeof(float)),
            nullptr,
            GL_DYNAMIC_DRAW
        );
        m_pose_count = int(m_records.size());
        glUniform1i(m_filter.uniform("attachment_count"), m_pose_count);
        // Exponential lerp gives the same response at different display rates.
        // Height catches up faster than tilt; rebuild the rotation from the unit normal.
        glUniform2f(
            m_filter.uniform("response"),
            momentum > 0 ? float(-std::expm1(-1.5 * dt / momentum)) : 1.f,
            momentum > 0 ? float(-std::expm1(-dt / momentum)) : 1.f
        );
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_anchor_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_pose_buffers[m_current]);
        glDispatchCompute(unsigned((m_records.size() + 63) / 64), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        glActiveTexture(GL_TEXTURE0);
    }
    m_previous = m_anchors;
    m_previous_ids.clear();
    for (std::size_t i = 0; i < m_previous.size(); ++i) {
        if (m_previous[i].second) {
            m_previous_ids.emplace(m_previous[i].second, i);
        }
    }
}

void WaterFloat::bind(const renderer::Shader& shader, bool enabled) const {
    if (shader.uniform("water_float_enabled") < 0 || !m_buffer) {
        return;
    }
    // Shared maps and phase are constant within one displayed frame; only the group flag changes.
    if (m_bound_shaders.insert(shader.id()).second) {
        bind_maps(shader);
        glUniform1i(shader.uniform("water_float_poses"), kPoseUnit);
        glUniform1i(shader.uniform("water_float_count"), m_pose_count);
        glActiveTexture(GL_TEXTURE0 + kPoseUnit);
        glBindTexture(
            renderer::world::gl::GL_TEXTURE_BUFFER,
            m_pose_textures[m_current] ? m_pose_textures[m_current] : m_bodies
        );

        glActiveTexture(GL_TEXTURE0);
    }
    glUniform1i(shader.uniform("water_float_enabled"), enabled && water_available() ? 1 : 0);
}

void WaterFloat::bind_maps(const renderer::Shader& shader) const {
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
}

void WaterFloat::release() {
    m_filter.release();
    if (m_anchor_buffer) {
        glDeleteBuffers(1, &m_anchor_buffer);
    }
    for (unsigned i = 0; i < 2; ++i) {
        if (m_pose_buffers[i]) {
            glDeleteBuffers(1, &m_pose_buffers[i]);
        }
        if (m_pose_textures[i]) {
            glDeleteTextures(1, &m_pose_textures[i]);
        }
        m_pose_buffers[i] = m_pose_textures[i] = 0;
    }
    m_anchor_buffer = m_current = 0;
    m_pose_count = 0;
    m_anchors.clear();
    m_previous.clear();
    m_previous_ids.clear();
    m_records.clear();
    if (m_bodies) {
        glDeleteTextures(1, &m_bodies);
    }
    if (m_map) {
        glDeleteTextures(1, &m_map);
    }
    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }
    m_buffer = m_bodies = m_map = 0;
    m_bound_shaders.clear();
    m_frame = ~std::uint64_t{0};
    m_requested = false;
    m_momentum = -1;
    m_seconds = -1;
}
}  // namespace openrac::enhanced
