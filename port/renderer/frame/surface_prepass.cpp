// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Renderer depth/normal prepass recording and its Vulkan graph boundary, before depth reuse and GTAO.
#include "common/stats.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"
#include "renderer/renderer.h"

namespace openrac::renderer {
void Renderer::draw_surface_prepass(const FrameInput& input, RenderState& state, bool normals) {
    using namespace gl;
    glDepthMask(GL_TRUE);
    glClearDepth(0);

    // Alpha zero marks background or unusable authored normals.
    if (normals) {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    state.depth_prepass = true;
    glColorMask(normals, normals, normals, normals);
    const stats::Scope depth("world depth prepass", stats::kGpu);

    // The same enabled scene layers feed graph and reference prepasses.
    for (const auto& renderer : m_renderers) {
        // Disabled layers must remain absent from depth and normal coverage too.
        if (renderer->enabled) {
            renderer->render_depth(input, state);
        }
    }

    state.depth_prepass = false;
}

void Renderer::render_surface_prepass(const FrameInput& input, RenderState& state, bool normals) {
    // Multisample declarations join shared execution; the immediate reference keeps its original prepass.
    if (normals && m_ambient_occlusion
        && (m_surface_prepass.samples() == 1 || collecting_surface_chain())) {
        m_ambient_occlusion->render_prepass(
            m_surface_prepass.id(),
            [this, &input, &state, normals] { draw_surface_prepass(input, state, normals); }
        );
        return;
    }

    draw_surface_prepass(input, state, normals);
}
}  // namespace openrac::renderer
