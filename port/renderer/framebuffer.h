// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// An off-screen render target: an RGBA8 colour texture and a depth-stencil
// buffer. The renderer draws a frame into one (OpenGOAL renders the game at
// its own resolution and scales it to the window), and the headless checks
// read their pixels back from one.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace openrac::renderer {

class FrameBuffer {
public:
    FrameBuffer() = default;
    ~FrameBuffer();
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    bool create(int width, int height, std::string& error);
    void release();

    unsigned id() const { return m_framebuffer; }

    unsigned colour_texture() const { return m_colour; }

    int width() const { return m_width; }

    int height() const { return m_height; }

    // The colour buffer as RGBA bytes, top row first.
    std::vector<std::uint8_t> read_rgba() const;

    // Copy to another framebuffer (0: the window), scaled to fill
    // dst_width x dst_height with linear filtering.
    void blit_to(unsigned target, int dst_width, int dst_height) const;

private:
    unsigned m_framebuffer = 0;
    unsigned m_colour = 0;
    unsigned m_depth = 0;
    int m_width = 0;
    int m_height = 0;
};

}  // namespace openrac::renderer
