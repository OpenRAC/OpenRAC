// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// An OverlayList's triangles blended over a framebuffer, through the renderer's GL: on OpenGL and
// on the Vulkan renderer's alike. It keeps its program and buffer from the first draw on; release()
// frees them while the context is current.

#pragma once

#include <span>

#include "renderer/shader.h"
#include "renderer/ui/overlay_list.h"

namespace openrac::renderer::ui {

class OverlayDraw {
public:
    OverlayDraw() = default;
    OverlayDraw(const OverlayDraw&) = delete;
    OverlayDraw& operator=(const OverlayDraw&) = delete;

    // Blends `vertices` over `framebuffer`, `width` x `height` pixels, the picture stretched over
    // it, by alpha and keeping the frame's own alpha. Nothing for no vertices. The context is
    // current; leaves blending off and the draw framebuffer at 0.
    void draw(unsigned framebuffer, int width, int height, std::span<const OverlayVertex> vertices);

    // Frees the GL objects, if a draw made them.
    void release();

private:
    bool ready();

    Shader m_shader;
    unsigned m_vao = 0;
    unsigned m_buffer = 0;
    bool m_failed = false;
};

}  // namespace openrac::renderer::ui
