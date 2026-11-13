// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <functional>
#include <span>

#include "renderer/gl/shader.h"
#include "renderer/renderer.h"

namespace openrac::lighting {
// One RGBA8 texel per 2x2 screen pixels: log-encoded irradiance RGB and
// linear cascade visibility A. The 8-byte guide stores surface identity and
// Ratchet shadow visibility in its remaining bits. Smooth normals are borrowed from the
// depth/normal prepass; compute never derives them from depth neighbours.
class EarlyLighting {
public:
    static bool supported();
    // Writes cached GI and shadow visibility after binding the current view's lighting inputs.
    // Returns false with `error` on graph/shader failure; callers retain full-resolution lighting.
    bool render(
        unsigned depth,
        const renderer::FrameInput& input,
        const renderer::RenderTarget& target,
        const std::function<void(const renderer::Shader&)>& bind_lighting,
        std::string& error
    );
    void bind(
        const renderer::Shader& shader,
        const renderer::FrameInput& input,
        const renderer::RenderState& state
    ) const;
    bool matches(const renderer::FrameInput& input, const renderer::RenderState& state) const;
    // Borrows both cache buffers only for the matching view; allocations survive the frame's draws.
    std::span<const unsigned> buffers(
        const renderer::FrameInput& input, const renderer::RenderState& state
    ) const;
    void release();

private:
    // Shadow filtering Soft's and Fast's, each for 2x2 and 4x4 cells (early-lighting-quarter).
    std::array<renderer::Shader, 4> m_compute;
    int m_scale = 2;  // Pixels a side of the cells the last render wrote.
    std::array<unsigned, 2> m_buffers{}, m_textures{};
    int m_width = 0, m_height = 0;
    unsigned m_target = 0;
    renderer::Mat4 m_clip{};
    std::uint64_t m_frame = 0;
    bool m_ready = false;
    bool m_failed = false;
};
}  // namespace openrac::lighting
