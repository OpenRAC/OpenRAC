// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exact RGBA8 frame snapshots in a GPU atlas, with one host readback per scenario.
// Deliberate depth-pass readbacks remain the caller's responsibility.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "renderer/framebuffer.h"
#include "renderer/gl.h"

namespace openrac::test {

// Owns a vertical strip of frame snapshots; create/release require a current GL context.
class RenderFrameBatch {
public:
    // Allocates one RGBA8 image without changing the source framebuffer's storage.
    [[nodiscard]] bool create(int width, int height, int count, std::string& error) {
        gl::GLint maximum = 0;
        gl::glGetIntegerv(gl::GL_MAX_TEXTURE_SIZE, &maximum);
        // Validate before multiplication or allocation, including malformed test requests.
        if (width <= 0 || height <= 0 || count <= 0 || width > maximum || height > maximum
            || count > maximum / height) {
            error = "invalid render snapshot atlas extent";
            return false;
        }
        m_width = width;
        m_height = height;
        m_count = count;
        return m_atlas.create(width, height * count, error);
    }

    // Releases GPU storage before the current context closes.
    ~RenderFrameBatch() { m_atlas.release(); }

    RenderFrameBatch() = default;
    RenderFrameBatch(const RenderFrameBatch&) = delete;
    RenderFrameBatch& operator=(const RenderFrameBatch&) = delete;

    // Records an unfiltered copy; read() waits for the pixels once per scenario.
    [[nodiscard]] bool capture(unsigned framebuffer, int index) {
        namespace gl = openrac::gl;
        // A bad slot cannot write outside this scenario's atlas or mutate render state.
        if (index < 0 || index >= m_count) {
            return false;
        }
        gl::glDisable(gl::GL_SCISSOR_TEST);
        gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
        gl::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, framebuffer);
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, m_atlas.id());
        gl::glBlitFramebuffer(
            0,
            0,
            m_width,
            m_height,
            0,
            (m_count - 1 - index) * m_height,
            m_width,
            (m_count - index) * m_height,
            gl::GL_COLOR_BUFFER_BIT,
            gl::GL_NEAREST
        );
        // Like FrameBuffer::blit_to, this ends the frame; the next draw establishes its state.
        gl::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, 0);
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, 0);
        return true;
    }

    // Returns the original top-down RGBA8 bytes for every snapshot, in capture order.
    auto read() const {
        const auto pixels = m_atlas.read_rgba();
        const auto bytes =
            static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 4;
        std::vector<std::vector<std::uint8_t>> frames;
        // A failed or incomplete readback cannot provide comparison frames.
        if (pixels.size() != bytes * static_cast<std::size_t>(m_count)) {
            return frames;
        }
        for (int index = 0; index < m_count; ++index) {
            const auto first =
                pixels.begin()
                + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(index) * bytes);
            frames.emplace_back(first, first + static_cast<std::ptrdiff_t>(bytes));
        }
        return frames;
    }

private:
    renderer::FrameBuffer m_atlas;  // RGBA8 snapshots, owned until the scenario is complete.
    int m_width = 0, m_height = 0, m_count = 0;  // Extents and number of snapshot slots.
};

}  // namespace openrac::test
