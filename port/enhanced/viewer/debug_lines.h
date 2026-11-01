// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enhanced diagnostics retain their availability gate while sharing the renderer's line mesh.
#pragma once

#include <utility>

#include "renderer/debug_lines.h"
#include "renderer/gl.h"

namespace openrac::lighting {
using renderer::append_debug_box;
using renderer::DebugLineVertex;

// Keeps enhanced-only diagnostics disabled on compatibility backends and non-enhanced builds.
class DebugLines {
public:
    // Replaces the owned paired endpoints for the next diagnostic draw.
    void set_vertices(std::vector<DebugLineVertex> vertices) {
        m_lines.set_vertices(std::move(vertices));
    }

    // Uses the same visible/hidden line styling only when enhanced rendering is available.
    [[nodiscard]] bool draw(
        const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
    ) {
        return !gl::enhanced_rendering_available() || m_lines.draw(input, state, error);
    }

    // Releases shared graphics objects with the context current.
    void release() { m_lines.release(); }

private:
    renderer::DebugLines m_lines;  // Owned mesh, shader and vertex buffer.
};

}  // namespace openrac::lighting
