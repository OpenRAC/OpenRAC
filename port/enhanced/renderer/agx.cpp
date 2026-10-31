// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/agx.h"

#include <algorithm>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/native_compute.h"
#endif
#include "renderer_shaders.h"

namespace openrac::enhanced {
using namespace openrac::gl;

AgxPreview::~AgxPreview() {
    release();
}

void AgxPreview::release() {
    m_copy.release();
    m_shader.release();
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_vao = 0;
    m_failed = false;
}

void AgxPreview::apply(renderer::FrameBuffer& output, float exposure) {
    if (m_failed || !enhanced_rendering_available() || output.width() <= 0
        || output.height() <= 0) {
        return;
    }
    const stats::Scope timing("AgX", stats::kGpu);
#if defined(OPENRAC_VULKAN)
    if (is_vulkan() && output.samples() == 1
        && renderer::render_opt::enabled(renderer::render_opt::Opt::AgxDirectOutput)
        && vkgl::storage_texture(output.colour_texture())) {
        static const std::string compute = [] {
            std::string source(renderer::shaders::agx_frag);
            source.replace(0, source.find('\n'), "#version 450\n#define OPENRAC_AGX_COMPUTE 1");
            return source;
        }();
        if (vkgl::transform_colour(
                output.colour_texture(), compute, std::clamp(exposure, -4.8f, 4.8f)
            )) {
            // Preserve the graphics postconditions of the fragment fallback for later HUD work.
            glBindFramebuffer(GL_FRAMEBUFFER, output.id());
            glViewport(0, 0, output.width(), output.height());
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            glDisable(GL_SCISSOR_TEST);
            glDepthMask(GL_TRUE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glBindVertexArray(0);
            glActiveTexture(GL_TEXTURE0);
            stats::gauge("AgX image copies", 0);
            return;
        }
    }
#endif
    std::string error;
    if ((!m_shader.id()
         && !m_shader
                 .build("AgX", renderer::shaders::gtao_vert, renderer::shaders::agx_frag, error))
        || ((m_copy.width() != output.width() || m_copy.height() != output.height())
            && !m_copy.create(output.width(), output.height(), error))) {
        log::error("AgX disabled: {}", error);
        m_failed = true;
        return;
    }
    stats::gauge("AgX image copies", 1);
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
