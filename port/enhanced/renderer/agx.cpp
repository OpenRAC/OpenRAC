// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/agx.h"

#include <algorithm>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

AgxPreview::~AgxPreview() {
    release();
}

void AgxPreview::release() {
    m_copy.release();
    m_shader.release();
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    m_failed = false;
}

void AgxPreview::apply(renderer::FrameBuffer& output, float exposure) {
    if (m_failed || !enhanced_rendering_available() || output.width() <= 0
        || output.height() <= 0) {
        return;
    }
    std::string error;
    if ((!m_shader.id()
         && !m_shader.build(
             "AgX", renderer::shaders::gtao_vert, renderer::shaders::agx_frag, error
         ))
        || ((m_copy.width() != output.width() || m_copy.height() != output.height())
            && !m_copy.create(output.width(), output.height(), error))) {
        log::error("AgX disabled: {}", error);
        m_failed = true;
        return;
    }
    const stats::Scope timing("AgX", stats::kGpu);
    // A separate source avoids attachment feedback, including multisampled world targets.
    output.blit_to(m_copy.id(), output.width(), output.height());
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, output.id());
    glViewport(0, 0, output.width(), output.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindVertexArray(m_vao);
    m_shader.use();
    glUniform1f(m_shader.uniform("exposure_ev"), std::clamp(exposure, -4.8f, 4.8f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_copy.colour_texture());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}
}  // namespace openrac::enhanced
