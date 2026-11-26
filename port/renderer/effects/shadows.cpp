// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/effects/shadows.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <optional>
#include <vector>

#include "common/stats.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer_shaders.h"

namespace openrac::renderer {
using namespace openrac::gl;

namespace {

// The target's pixels the volumes can cover, as x, y, width, height (GL window coordinates, with a
// pixel of margin): only there can the stencil count end nonzero, so the clear and the composite
// stay inside it. Empty when no volume is in view; none (the whole target) when a corner lies
// at or behind the eye, where the projection gives no bound.
std::optional<std::array<int, 4>> volume_area(
    const Mat4& view_projection, const std::vector<Vec3>& vertices, int width, int height
) {
    float x0 = 1, y0 = 1, x1 = -1, y1 = -1;
    for (const Vec3& v : vertices) {
        const auto row = [&](int r) {
            return view_projection[r] * v[0] + view_projection[4 + r] * v[1]
                   + view_projection[8 + r] * v[2] + view_projection[12 + r];
        };
        const float w = row(3);
        if (!(w > 1e-4f)) {
            return std::nullopt;
        }
        const float x = row(0) / w, y = row(1) / w;
        x0 = std::min(x0, x);
        y0 = std::min(y0, y);
        x1 = std::max(x1, x);
        y1 = std::max(y1, y);
    }
    const auto pixel = [](float ndc, int size) {
        return (std::clamp(ndc, -1.0f, 1.0f) * 0.5f + 0.5f) * static_cast<float>(size);
    };
    if (x0 > x1 || y0 > y1 || x1 < -1 || y1 < -1 || x0 > 1 || y0 > 1) {
        return std::array<int, 4>{};
    }
    const int left = std::max(0, static_cast<int>(std::floor(pixel(x0, width))) - 1);
    const int bottom = std::max(0, static_cast<int>(std::floor(pixel(y0, height))) - 1);
    const int right = std::min(width, static_cast<int>(std::ceil(pixel(x1, width))) + 1);
    const int top = std::min(height, static_cast<int>(std::ceil(pixel(y1, height))) + 1);
    return std::array<int, 4>{left, bottom, std::max(0, right - left), std::max(0, top - bottom)};
}

}  // namespace

bool ShadowRenderer::init(RenderState&, std::string& error) {
    if (!m_volume.build("shadow volumes", shaders::shadow_vert, shaders::shadow_frag, error)
        || !m_composite.build(
            "shadow composite", shaders::shadow_composite_vert, shaders::shadow_frag, error
        )) {
        return false;
    }
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_buffer);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), nullptr);
    glBindVertexArray(0);
    return true;
}

// Volume counting clears and writes stencil before the composite samples it.
Result<void> ShadowRenderer::record_colour(const FrameInput& input, RenderState& state) {
    try {
        render(input, state);
        return {};
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    }
}

void ShadowRenderer::render(const FrameInput& input, RenderState& state) {
    if (enhanced_rendering_available() && (input.hero_pixel_shadow || input.sun_pixel_shadow)) {
        return;
    }  // modern shadows resolve in mesh.frag
    auto vertices =
        shadow_volumes(input.shadow_capsules, input.shadow_extrusion, input.shadow_heights);
    // Closed volumes count independently in the stencil, so every caster's join the one pass.
    for (const auto& set : input.shadow_sets) {
        const auto more = shadow_volumes(set.capsules, set.extrusion, set.heights);
        vertices.insert(vertices.end(), more.begin(), more.end());
    }
    if (vertices.empty()) {
        return;
    }
    const auto vp = multiply(input.camera.projection, input.camera.view);
    // On a tiled GPU a full-screen clear and composite touch every tile of a large multisampled
    // target; the volumes of the hero's shadow cover a small part of it.
    static const bool& whole_target = stats::probe(
        "OPENRAC_SHADOW_SCISSOR_OFF",
        "Reference: hero shadow clear and composite over the whole target"
    );
    const auto area = whole_target
                          ? std::nullopt
                          : volume_area(vp, vertices, state.target.width, state.target.height);
    if (area && ((*area)[2] == 0 || (*area)[3] == 0)) {
        return;
    }
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vec3)),
        vertices.data(),
        GL_STREAM_DRAW
    );
    const RasterBuffer geometry{m_buffer, vertices.size() * sizeof(Vec3)};
    const auto draw = [&] {
        glViewport(0, 0, state.target.width, state.target.height);
        if (area) {
            glEnable(GL_SCISSOR_TEST);
            glScissor((*area)[0], (*area)[1], (*area)[2], (*area)[3]);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        glDisable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xff);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 0, 0xff);
        // Depth fail (reversed depth too): count the closed surfaces behind the
        // receiver. Overlapping capsules are one shadow, composited only once.
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
        m_volume.use();
        glUniformMatrix4fv(m_volume.uniform("view_projection"), 1, GL_FALSE, vp.data());
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilMask(0);
        glStencilFunc(GL_NOTEQUAL, 0, 0xff);
        glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        m_composite.use();
        // Retail submit_graphics_setup_command_stream uses ALPHA FIX=32, divisor 128.
        glUniform1f(m_composite.uniform("opacity"), 32.0f / 128.0f);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    };
    raster_pass(
        name(), state.target, {{&geometry, 1}, {}, ColourAttachments::ColourDepthStencil}, draw
    );
    state.stats.draw_calls += 2;
    state.stats.triangles += vertices.size() / 3 + 1;

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glStencilMask(0xff);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void ShadowRenderer::release() {
    m_volume.release();
    m_composite.release();
    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    m_buffer = m_vao = 0;
}
}  // namespace openrac::renderer
