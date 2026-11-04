// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <span>

#include "renderer/framebuffer.h"
#include "renderer/math.h"
#include "renderer/shader.h"

namespace openrac::renderer {

// Heat haze over the picture (mod interface 1.12, openrac_api.haze_set): each patch a ball in the
// world whose picture behind it wobbles. Drawn before the HUD on the render worker.
class Haze {
public:
    ~Haze();
    // Each patch: its place (x, y, z, the game's units), radius and strength (a part of its size
    // on the screen). `seconds` moves the wobble.
    void apply(
        FrameBuffer& output,
        const Mat4& view_projection,
        std::span<const std::array<float, 5>> patches,
        float seconds
    );
    void release();

private:
    FrameBuffer m_copy;
    Shader m_shader;
    unsigned m_vao = 0;
    bool m_failed = false;
};

}  // namespace openrac::renderer
