// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection validation and a single packed HDR texture upload; no runtime filtering.
#include "enhanced/viewer/environment_lighting.h"

#include <fstream>
#include <stdexcept>

#include "common/log.h"
#include "enhanced/viewer/environment_overlay.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"

namespace openrac::lighting {
using namespace gl;

namespace {
// Water deformation uses units 18-20; selection masks and metadata use 21-22.
constexpr unsigned kEnvironmentUnit = 23;

// Shared packed format for real captures and the descriptor-safe black fallback.
void upload(unsigned texture, unsigned width, unsigned height, const void* pixels) {
    glActiveTexture(GL_TEXTURE0 + kEnvironmentUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R11F_G11F_B10F,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RGB,
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        pixels
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glActiveTexture(GL_TEXTURE0);
}
}  // namespace

void EnvironmentLighting::load(const std::filesystem::path& directory, const ProbeGrid& gi) {
    release();
    const auto path = directory / "environment.orpe";

    if (!std::filesystem::is_regular_file(path)) {
        return;
    }

    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file || file.tellg() < 0 || file.tellg() > std::streamoff(1024 * 1024 * 1024)) {
            throw std::runtime_error("invalid environment file size");
        }

        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.tellg()));
        file.seekg(0);

        if (!file.read(
                reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
            )) {
            throw std::runtime_error("cannot read environment file");
        }

        auto asset = decode_environment(bytes);

        if (asset.level != gi.level || asset.scene_hash != gi.scene_hash
            || asset.settings_hash != gi.settings_hash
            || asset.gi_hash != environment_gi_hash(directory)) {
            throw std::runtime_error(
                "environment capture differs from active GI bake; rebake reflections"
            );
        }

        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);

        if (asset.width() > static_cast<unsigned>(limit)
            || asset.height() > static_cast<unsigned>(limit)) {
            throw std::runtime_error("environment atlas exceeds device dimensions");
        }

        glGenTextures(1, &m_texture);
        upload(m_texture, asset.width(), asset.height(), asset.pixels.data());

        if (glGetError() != GL_NO_ERROR) {
            throw std::runtime_error("environment atlas upload failed");
        }

        m_resolution = asset.resolution;
        m_columns = asset.columns();
        m_debug.set_vertices(environment_probe_debug_vertices(asset.probes));

        m_metadata = environment_metadata(asset.probes);

        std::string selection_error;
        if (!m_selection.prepare(m_metadata, selection_error) && !selection_error.empty()) {
            log::warn("environment tile culling unavailable: {}", selection_error);
        }
        log::info(
            "environment probes: {} captures, {} GGX levels, {} bytes",
            asset.probes.size(),
            asset.levels(),
            asset.pixels.size() * 4
        );
    } catch (const std::exception& error) {
        release();
        log::warn("baked environment reflections unavailable: {}", error.what());
    }
}

void EnvironmentLighting::bind(
    const renderer::Shader& shader, bool enabled, const renderer::Camera* camera
) const {
    const auto count = shader.uniform("environment_count");

    if (count < 0) {
        return;  // Programs without an environment material need no reflection descriptors.
    }

    if (!m_texture && !m_fallback) {
        const unsigned black = 0;
        glGenTextures(1, &m_fallback);
        upload(m_fallback, 1, 1, &black);
    }

    m_selection.bind(shader, enabled ? camera : nullptr);
    glUniform1i(count, enabled ? static_cast<int>(m_metadata.size() / 5) : 0);
    glUniform1i(shader.uniform("environment_map"), kEnvironmentUnit);
    glUniform1i(shader.uniform("environment_resolution"), static_cast<int>(m_resolution));

    glUniform1i(shader.uniform("environment_columns"), static_cast<int>(m_columns));

    glActiveTexture(GL_TEXTURE0 + kEnvironmentUnit);
    glBindTexture(GL_TEXTURE_2D, m_texture ? m_texture : m_fallback);
    glActiveTexture(GL_TEXTURE0);
}

void EnvironmentLighting::draw_debug(
    const renderer::FrameInput& input, renderer::RenderState& state, bool shown
) {
    if (!shown) {
        return;
    }
    std::string error;
    if (!m_debug_failed
        && (!m_debug.draw(input, state, error) || !draw_previews(input, state, error))) {
        log::warn("environment probe overlay disabled: {}", error);
        m_debug_failed = true;
    }
}

bool EnvironmentLighting::draw_previews(
    const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
) {
    if (!enhanced_rendering_available() || !m_texture || m_metadata.empty()) {
        return true;
    }
    if (!m_preview_shader.id()) {
        const auto fragment = std::string("#version 410 core\n")
                              + std::string(shaders::environment_glsl)
                              + std::string(shaders::environment_preview_frag);
        if (!m_preview_shader.build(
                "environment capture spheres", shaders::environment_preview_vert, fragment, error
            )) {
            return false;
        }
        glGenVertexArrays(1, &m_preview_vao);
    }
    glBindVertexArray(m_preview_vao);
    m_preview_shader.use();
    bind(m_preview_shader, true);
    const auto matrix = renderer::multiply(input.camera.projection, input.camera.view);
    glUniformMatrix4fv(m_preview_shader.uniform("view_projection"), 1, GL_FALSE, matrix.data());
    glUniform3f(
        m_preview_shader.uniform("preview_camera"),
        input.camera.position[0],
        input.camera.position[1],
        input.camera.position[2]
    );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_FALSE);  // Diagnostic spheres never hide scene geometry or later markers.
    glDisable(GL_BLEND);
    // Latitude/longitude triangles wind inward, matching the diffuse probe sphere renderer.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    constexpr unsigned kVerticesPerSphere = 24 * 16 * 6;
    const auto count = static_cast<unsigned>(m_metadata.size() / 5);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count * kVerticesPerSphere));
    ++state.stats.draw_calls;
    state.stats.triangles += count * (kVerticesPerSphere / 3);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
    return true;
}

void EnvironmentLighting::release() {
    m_selection.release();
    m_debug.release();
    m_preview_shader.release();
    if (m_preview_vao) {
        glDeleteVertexArrays(1, &m_preview_vao);
        m_preview_vao = 0;
    }
    m_debug_failed = false;
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }

    if (m_fallback) {
        glDeleteTextures(1, &m_fallback);
    }

    m_texture = m_fallback = 0;
    m_metadata.clear();
}
}  // namespace openrac::lighting
