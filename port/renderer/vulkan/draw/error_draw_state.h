// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unknown vertex shaders draw an opaque viewport diagnostic without changing subsequent GL state.
#pragma once
#include <algorithm>
#include <array>
#include <iterator>

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// A render-thread override borrowing one draw's state, restored before the next caller uses it.
class ErrorDrawState {
public:
    // Makes the viewport diagnostic opaque and independent of incompatible caller geometry state.
    explicit ErrorDrawState(State& state)
        : m_state(state),
          m_flags{
              state.blend,
              state.depth_test,
              state.cull,
              state.stencil_test,
              state.primitive_restart,
              state.depth_mask
          },
          m_mask{
              state.colour_mask[0], state.colour_mask[1], state.colour_mask[2], state.colour_mask[3]
          },
          m_polygon(state.polygon_mode) {
        state.blend = false;
        state.depth_test = false;
        state.cull = false;
        state.stencil_test = false;
        state.primitive_restart = false;
        state.depth_mask = false;
        state.polygon_mode = gl::GL_FILL;
        std::fill(std::begin(state.colour_mask), std::end(state.colour_mask), true);
    }

    // Restores the caller's fixed state even if draw recording throws.
    ~ErrorDrawState() {
        m_state.blend = m_flags[0];
        m_state.depth_test = m_flags[1];
        m_state.cull = m_flags[2];
        m_state.stencil_test = m_flags[3];
        m_state.primitive_restart = m_flags[4];
        m_state.depth_mask = m_flags[5];
        m_state.polygon_mode = m_polygon;
        std::copy(m_mask.begin(), m_mask.end(), m_state.colour_mask);
    }

    ErrorDrawState(const ErrorDrawState&) = delete;
    ErrorDrawState& operator=(const ErrorDrawState&) = delete;

private:
    State& m_state;               // Borrowed for one immediate draw inside the owning graph pass.
    std::array<bool, 6> m_flags;  // Saved blend/depth/cull/stencil/restart and depth-write enables.
    std::array<bool, 4> m_mask;   // Original RGBA write masks.
    GLenum m_polygon;             // Original fill, line or point mode.
};
}  // namespace openrac::vkgl::detail
