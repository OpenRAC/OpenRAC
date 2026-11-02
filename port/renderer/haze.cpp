// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/haze.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer_shaders.h"

namespace openrac::renderer {
using namespace openrac::gl;

Haze::~Haze() {
    release();
}

void Haze::release() {
    m_copy.release();
    m_shader.release();
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_vao = 0;
    m_failed = false;
}

void Haze::apply(
    FrameBuffer& output,
    const Mat4& view_projection,
    std::span<const std::array<float, 5>> patches,
    float seconds
) {
    if (m_failed || patches.empty() || output.width() <= 0 || output.height() <= 0) {
        return;
    }
    const stats::Scope timing("Heat haze", stats::kGpu);
    std::string error;
    if ((!m_shader.id()
         && !m_shader.build("Heat haze", shaders::haze_vert, shaders::haze_frag, error))
        || ((m_copy.width() != output.width() || m_copy.height() != output.height())
            && !m_copy.create(output.width(), output.height(), error))) {
        log::error("Heat haze disabled: {}", error);
        m_failed = true;
        return;
    }
    // The picture as it is, read through the patches (no feedback on the target).
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_copy.colour_texture());
    const Mat4& m = view_projection;
    for (std::size_t i = 0; i < patches.size(); ++i) {
        const auto& p = patches[i];
        // The ball's centre in clip space; its size on the screen from its distance.
        const float x = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
        const float y = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
        const float w = m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15];
        if (!(w > 0.05f) || !(p[3] > 0.0f) || !(p[4] > 0.0f)) {
            continue;
        }
        // The projection's scale on x and y: the length of its first two rows' turn part.
        const float sx = std::hypot(m[0], m[4], m[8]);
        const float sy = std::hypot(m[1], m[5], m[9]);
        const float hx = p[3] * sx / w, hy = p[3] * sy / w;
        const float cx = x / w, cy = y / w;
        if (cx - hx > 1.0f || cx + hx < -1.0f || cy - hy > 1.0f || cy + hy < -1.0f) {
            continue;
        }
        const float pixels = std::min(p[4], 1.0f) * hy * 0.5f * static_cast<float>(output.height());
        glUniform4f(m_shader.uniform("patch_place"), cx, cy, hx, hy);
        glUniform4f(
            m_shader.uniform("patch_wave"), pixels, seconds, static_cast<float>(i) * 2.399f, 0.0f
        );
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

}  // namespace openrac::renderer
