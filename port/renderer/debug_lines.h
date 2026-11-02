// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unlit world diagnostics with opaque visible edges and faint edges behind scene geometry.

#pragma once

#include <string>
#include <vector>

#include "renderer/math.h"
#include "renderer/shader.h"

namespace openrac::renderer {
struct FrameInput;
struct RenderState;

// One GL_LINES endpoint in world space, with an unlit RGB colour.
struct DebugLineVertex {
    renderer::Vec3 position{};  // World coordinates, with Z as height.
    renderer::Vec3 colour{};    // Display colour, independent of scene lighting.
};

// Appends a clip volume after intersecting its six planes; singular/non-finite boxes are rejected.
[[nodiscard]] bool append_debug_frustum(
    std::vector<DebugLineVertex>& vertices, const renderer::Mat4& clip, renderer::Vec3 colour
);

// Appends the twelve edges of a world-space axis-aligned box, in stable corner order.
void append_debug_box(
    std::vector<DebugLineVertex>& vertices,
    renderer::Vec3 minimum,
    renderer::Vec3 maximum,
    renderer::Vec3 colour
);

// Render-thread-owned line mesh; its graphics context outlives this object or release().
class DebugLines {
public:
    DebugLines() = default;
    // Releases diagnostic GPU resources with the graphics context current.
    ~DebugLines();
    DebugLines(const DebugLines&) = delete;
    DebugLines& operator=(const DebugLines&) = delete;
    DebugLines(DebugLines&&) = delete;
    DebugLines& operator=(DebugLines&&) = delete;

    // Replaces paired endpoints; the next draw uploads the new mesh once.
    void set_vertices(std::vector<DebugLineVertex> vertices);
    // Draws visible and hidden edges without writing scene depth; reports shader failures in error.
    [[nodiscard]] bool draw(
        const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
    );
    // Clears CPU geometry and GPU resources with the graphics context current.
    void release();

private:
    // Creates the GPU vertex layout and lazily compiles the shared line shader.
    [[nodiscard]] bool prepare(std::string& error);
    // Uploads geometry only after set_vertices changes it.
    void upload();
    std::vector<DebugLineVertex> m_vertices;  // Owned paired world endpoints.
    renderer::Shader m_shader;                // Unlit debug shader.
    unsigned m_vao = 0;                       // Owned vertex layout.
    unsigned m_buffer = 0;                    // Owned endpoint buffer.
    bool m_uploaded = false;                  // Current CPU mesh is resident.
};

}  // namespace openrac::renderer
