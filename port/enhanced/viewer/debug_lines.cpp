// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// DebugLines: shared line upload and reversed-depth wireframe drawing for lighting diagnostics.

#include "enhanced/viewer/debug_lines.h"

#include <cstddef>
#include <utility>

#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"

namespace openrac::lighting {
namespace {
// Hidden edges remain legible without competing with visible edges.
constexpr float kHiddenAlpha = .18f;
}  // namespace

DebugLines::~DebugLines() {
    release();
}

void DebugLines::set_vertices(std::vector<DebugLineVertex> vertices) {
    m_vertices = std::move(vertices);
    m_uploaded = false;
}

void DebugLines::upload() {
    using namespace openrac::gl;

    // Moving the player inside the same volumes does not require another upload.
    if (m_uploaded) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(DebugLineVertex)),
        m_vertices.data(),
        GL_DYNAMIC_DRAW
    );
    m_uploaded = true;
}

bool DebugLines::prepare(std::string& error) {
    using namespace openrac::gl;

    // Shader compilation is deferred until the overlay is requested.
    if (m_shader.id() != 0) {
        return true;
    }

    if (!m_shader.build(
            "lighting debug lines",
            shaders::area_light_overlay_vert,
            shaders::area_light_overlay_frag,
            error
        )) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_buffer);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugLineVertex), nullptr);
    glEnableVertexAttribArray(1);
    // The byte offset describes an interleaved GPU vertex, not a CPU pointer to dereference.
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(DebugLineVertex),
        // NOLINTNEXTLINE(performance-no-int-to-ptr): GL takes a byte offset in this pointer argument.
        reinterpret_cast<const void*>(offsetof(DebugLineVertex, colour))
    );
    return true;
}

bool DebugLines::draw(
    const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
) {
    using namespace openrac::gl;

    // No GPU state is created on compatibility backends or for empty diagnostic meshes.
    if (!enhanced_rendering_available() || m_vertices.empty()) {
        return true;
    }

    if (!prepare(error)) {
        return false;
    }

    upload();
    glBindVertexArray(m_vao);
    m_shader.use();
    const auto matrix = renderer::multiply(input.camera.projection, input.camera.view);
    glUniformMatrix4fv(m_shader.uniform("view_projection"), 1, GL_FALSE, matrix.data());
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    // Reversed depth separates faint hidden edges from opaque visible edges without changing world depth.
    glDepthFunc(GL_LESS);
    glUniform1f(m_shader.uniform("debug_alpha"), kHiddenAlpha);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    glDepthFunc(GL_GEQUAL);
    glUniform1f(m_shader.uniform("debug_alpha"), 1);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    state.stats.draw_calls += 2;
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void DebugLines::release() {
    using namespace openrac::gl;
    m_shader.release();

    // Explicit scene release normally clears these before destruction or a level change.
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }

    if (m_buffer != 0) {
        glDeleteBuffers(1, &m_buffer);
    }

    m_vao = 0;
    m_buffer = 0;
    m_vertices.clear();
    m_uploaded = false;
}
}  // namespace openrac::lighting
