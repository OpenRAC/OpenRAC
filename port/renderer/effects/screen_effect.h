// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A game's screen effects as native post passes (openrac_frame_screen_effect,
// docs/port/SCREEN_EFFECTS.md): the target as drawn so far copied, shrunk to a half, a quarter and an
// eighth, blurred there and put back over the target, with the game's tint, threshold and
// strength. Drawn by the 2D renderer where the recording puts the effect, with renderer/gl/gl.h's
// calls, so it runs on OpenGL and on the Vulkan renderer alike.
//
// Its GL objects are made at the first effect and freed by release(), while the context is
// current.

#pragma once

#include <array>
#include <cstddef>

#include "renderer/frame/frame_record.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"

namespace openrac::renderer {

struct RenderTarget;

class ScreenEffects {
public:
    ScreenEffects() = default;
    ~ScreenEffects();
    ScreenEffects(const ScreenEffects&) = delete;
    ScreenEffects& operator=(const ScreenEffects&) = delete;

    // `effect` over `target` (its colour as drawn so far, read through `readable`, the
    // framebuffer it is resolved into when multisampled). Leaves the framebuffer, program, vertex
    // array and textures changed: the caller sets its own again.
    void apply(const ScreenEffect& effect, const RenderTarget& target, unsigned readable);
    void release();

private:
    // One size of the copy: two framebuffers to blur from one into the other, and which holds it.
    struct Level {
        std::array<FrameBuffer, 2> buffers;
        std::size_t current = 0;  // Index of the last buffer written, always 0 or 1.

        unsigned texture() const { return buffers[current].colour_texture(); }

        int width() const { return buffers[0].width(); }

        int height() const { return buffers[0].height(); }
    };

    bool prepare(int width, int height);
    // Draws `mode` from `source` (width x height) into `into`'s other buffer, which then holds it.
    void pass(int mode, unsigned source, int width, int height, Level& into);
    void blur(Level& level, int times);
    void resample(const Level& from, Level& into);
    // The copy put back over the target, from the frame's copy and the blurred half.
    void put_back(int mode, const RenderTarget& target);

    Shader m_shader;
    unsigned m_vao = 0;
    unsigned m_frame = 0;  // the target as drawn so far (first row at the bottom)
    int m_frame_width = 0, m_frame_height = 0;
    std::array<Level, 3> m_levels;  // a half, a quarter, an eighth of the target
    ScreenEffect m_effect{};
    bool m_failed = false;
};

}  // namespace openrac::renderer
