// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/overlay_draw.h"

#include <cstddef>
#include <string>

#include "common/log.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"
#include "renderer/ui/overlay_draw_sources.h"

namespace openrac::renderer::ui {

using namespace openrac::gl;

namespace {

// The picture's positions to device coordinates, y down; colours as they come.
using shader_sources::renderer_ui_overlay_draw::kVertex;

using shader_sources::renderer_ui_overlay_draw::kFragment;

// What the blending does for `blend`, which the list has already checked is one it has. In each the
// frame's alpha is kept (the alpha factors are 0 and 1 whichever equation: dst * 1 +- src * 0).
// Subtract is the game's own "destination minus source * alpha" (the GS pass of
// renderer/world/gs_pass.cpp): GL's reverse subtract is dst * dfactor - src * sfactor, here
// dst - src * alpha, and a colour target holds nothing below 0.
void set_blend(OverlayBlend blend) {
    switch (blend) {
        case OverlayBlend::Alpha:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            break;
        case OverlayBlend::Add:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
            break;
        case OverlayBlend::Subtract:
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
            break;
    }
}

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
    const OverlayBatch all{0, static_cast<std::uint32_t>(vertices.size()), OverlayBlend::Alpha};
    draw(framebuffer, width, height, vertices, {&all, 1});
}

void OverlayDraw::draw(
    unsigned framebuffer,
    int width,
    int height,
    std::span<const OverlayVertex> vertices,
    std::span<const OverlayBatch> batches
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
    const RasterBuffer geometry{m_buffer, vertices.size_bytes()};
    // Upload and vertex layout are complete before the pass borrows the buffer.
    raster_pass("mod overlay", {framebuffer, width, height}, {{&geometry, 1}}, [&] {
        bool blend_set = false;
        OverlayBlend blend = OverlayBlend::Alpha;
        for (const OverlayBatch& batch : batches) {
            if (batch.count == 0
                || static_cast<std::size_t>(batch.first) + batch.count > vertices.size()) {
                continue;
            }
            if (!blend_set || batch.blend != blend) {
                set_blend(batch.blend);
                blend = batch.blend;
                blend_set = true;
            }
            glDrawArrays(
                GL_TRIANGLES, static_cast<GLint>(batch.first), static_cast<GLsizei>(batch.count)
            );
        }
    });
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Whoever draws next finds the default equation, as it did before there was a subtract.
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void OverlayDraw::release() {
    if (m_buffer != 0) {
        glDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_shader.release();
    m_failed = false;
}

}  // namespace openrac::renderer::ui
