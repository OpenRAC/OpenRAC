// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/viewer/environment_selection.h"

#include "enhanced/lighting/environment.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"

namespace openrac::lighting {
using namespace gl;
namespace wgl = renderer::world::gl;

namespace {
constexpr unsigned kMaskUnit = 21, kMetadataUnit = 22;
static_assert(
    kEnvironmentProbeLimit == 512
);  // Sixteen words in the shared compute/fragment layout.
constexpr unsigned kMaskWords = kEnvironmentProbeLimit / 32;
constexpr unsigned kTileWords = 1 + kMaskWords;
}  // namespace

void EnvironmentSelection::ensure_buffer() const {
    if (m_buffer) {
        return;
    }

    const unsigned zero = 0;
    glGenBuffers(1, &m_buffer);
    glBindBuffer(wgl::GL_TEXTURE_BUFFER, m_buffer);
    glBufferData(wgl::GL_TEXTURE_BUFFER, sizeof(zero), &zero, GL_DYNAMIC_DRAW);
    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + kMaskUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_texture);
    glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_R32UI, m_buffer);
    glGenBuffers(1, &m_metadata_buffer);
    glBindBuffer(wgl::GL_TEXTURE_BUFFER, m_metadata_buffer);
    const std::array<float, 4> empty{};
    glBufferData(wgl::GL_TEXTURE_BUFFER, sizeof(empty), empty.data(), GL_STATIC_DRAW);
    glGenTextures(1, &m_metadata_texture);
    glActiveTexture(GL_TEXTURE0 + kMetadataUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_metadata_texture);
    glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32F, m_metadata_buffer);
    glActiveTexture(GL_TEXTURE0);
}

bool EnvironmentSelection::prepare(
    std::span<const std::array<float, 4>> metadata, std::string& error
) {
    release();

    if (!enhanced_rendering_available() || metadata.empty() || metadata.size() % 5 != 0
        || metadata.size() / 5 > kEnvironmentProbeLimit) {
        return false;
    }

    m_count = static_cast<unsigned>(metadata.size() / 5);
    ensure_buffer();
    glBindBuffer(wgl::GL_TEXTURE_BUFFER, m_metadata_buffer);
    glBufferData(
        wgl::GL_TEXTURE_BUFFER,
        static_cast<GLsizeiptr>(metadata.size_bytes()),
        metadata.data(),
        GL_STATIC_DRAW
    );

    // Metadata also serves the reference path if compute preparation is unavailable.
    if (!glDispatchCompute || !glMemoryBarrier) {
        return false;
    }

    GLint bindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);

    if (bindings <= 8) {
        return false;
    }

    if (!m_cull.build_compute(
            "tiled environment-probe culling", shaders::environment_selection_comp, error
        )) {
        return false;
    }

    return true;
}

bool EnvironmentSelection::dispatch(
    const renderer::Camera& camera, const std::array<int, 4>& viewport
) const {
    if (!m_cull.id() || viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }

    const auto clip = renderer::multiply(camera.projection, camera.view);

    if (m_started && clip == m_clip && viewport == m_viewport) {
        return true;
    }

    const auto x = (unsigned(viewport[2]) + tile_size - 1) / tile_size;
    const auto y = (unsigned(viewport[3]) + tile_size - 1) / tile_size;
    const auto words = std::size_t(x) * y * kTileWords;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);

    if (m_words != words) {
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(words * sizeof(unsigned)),
            nullptr,
            GL_DYNAMIC_DRAW
        );
        m_words = words;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_buffer);
    // Earlier draws may still consume the previous masks through their texture-buffer view.
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    m_cull.use();
    glUniformMatrix4fv(m_cull.uniform("world_to_clip"), 1, GL_FALSE, clip.data());
    glUniform2f(m_cull.uniform("tile_extent"), float(viewport[2]), float(viewport[3]));
    glUniform1i(m_cull.uniform("capture_count"), static_cast<int>(m_count));
    glUniform1i(m_cull.uniform("capture_boxes"), kMetadataUnit);
    glActiveTexture(GL_TEXTURE0 + kMetadataUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_metadata_texture);
    glActiveTexture(GL_TEXTURE0);
    glDispatchCompute(x, y, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    m_started = true;
    m_clip = clip;
    m_viewport = viewport;
    return true;
}

void EnvironmentSelection::bind(
    const renderer::Shader& shader, const renderer::Camera* camera
) const {
    const auto location = shader.uniform("environment_tiled");
    const auto metadata = shader.uniform("environment_metadata");

    if (location < 0 && metadata < 0) {
        return;
    }

    ensure_buffer();  // A descriptor-safe fallback is required even when the uniform disables tiles.
    std::array<int, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    const bool tiled = location >= 0 && camera && dispatch(*camera, viewport);
    shader.use();
    glUniform1i(location, tiled ? 1 : 0);
    glUniform4f(
        shader.uniform("environment_tile_view"),
        float(viewport[0]),
        float(viewport[1]),
        float(viewport[2]),
        float(viewport[3])
    );
    glUniform1i(shader.uniform("environment_tiles"), kMaskUnit);
    glActiveTexture(GL_TEXTURE0 + kMaskUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_texture);
    glUniform1i(metadata, kMetadataUnit);
    glActiveTexture(GL_TEXTURE0 + kMetadataUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_metadata_texture);
    glActiveTexture(GL_TEXTURE0);
}

void EnvironmentSelection::release() {
    m_cull.release();

    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }

    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }

    m_buffer = m_texture = 0;

    if (m_metadata_texture) {
        glDeleteTextures(1, &m_metadata_texture);
    }

    if (m_metadata_buffer) {
        glDeleteBuffers(1, &m_metadata_buffer);
    }

    m_metadata_texture = m_metadata_buffer = 0;
    m_count = 0;
    m_words = 1;
    m_started = false;
    m_viewport = {};
}
}  // namespace openrac::lighting
