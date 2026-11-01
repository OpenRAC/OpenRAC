// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/overlay_draw.h"

#include <cstddef>
#include <string>

#include "common/log.h"
#include "renderer/gl.h"

namespace openrac::renderer::ui {

using namespace openrac::gl;

namespace {

// The picture's positions to device coordinates, y down; colours as they come.
constexpr const char* kVertex = R"glsl(#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 colour;
out vec4 v_colour;
void main() {
    v_colour = colour;
    gl_Position = vec4(position.x / 256.0 - 1.0, 1.0 - position.y / 224.0, 0.0, 1.0);
}
)glsl";

constexpr const char* kFragment = R"glsl(#version 410 core
in vec4 v_colour;
out vec4 out_colour;
void main() {
    out_colour = v_colour;
}
)glsl";

}  // namespace

bool OverlayDraw::ready() {
    if (m_failed) {
        return false;
    }
    if (m_vao == 0) {
        std::string error;
        if (!m_shader.build("mod overlay", kVertex, kFragment, error)) {
            log::warn("what mods draw over the game cannot be drawn: {}", error);
            m_failed = true;
            return false;
        }
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_buffer);
    }
    return true;
}

void OverlayDraw::draw(
    unsigned framebuffer, int width, int height, std::span<const OverlayVertex> vertices
) {
    if (vertices.empty() || width <= 0 || height <= 0 || !ready()) {
        return;
    }
    // All of the state the triangles depend on, whatever the frame left.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    m_shader.use();
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(),
        GL_STREAM_DRAW
    );
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    const auto stride = static_cast<GLsizei>(sizeof(OverlayVertex));
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glVertexAttribPointer(
        1,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<const void*>(offsetof(OverlayVertex, rgba))
    );
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void OverlayDraw::release() {
    m_shader.release();
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_buffer);
    }
    m_vao = 0;
    m_buffer = 0;
    m_failed = false;
}

}  // namespace openrac::renderer::ui
