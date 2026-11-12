// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Raster setup retained across temporal preparation: uniforms, sampler units and framebuffer roles.
#pragma once

#include <array>
#include <functional>
#include <optional>

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// Borrows context-owned names through the synchronous scope; graph imports pin image storage.
class PreparedRaster {
public:
    // Captures the bound raster program and both framebuffer attachment descriptions.
    PreparedRaster();
    // Installs captured values for one callback, then restores preparation's later CPU-side values.
    void record(const std::function<void()>& draw);

private:
    // Exchanges retained program values, invalidating its loose-uniform upload cache.
    void exchange_program();
    // Exchanges framebuffer roles after ending any pending rendering.
    void exchange_targets();
    State m_state;                         // Raster state at this pass's preparation point.
    std::uint64_t m_program_serial = 0;    // Rejects relinking before execution.
    std::vector<std::uint8_t> m_uniforms;  // Per-pass std140 bytes.
    std::vector<int> m_units;              // Sampler-unit choices.
    std::vector<GLuint> m_blocks;          // Uniform-block bindings.
    std::array<GLuint, 2> m_targets{};     // Distinct draw/read names, zero for the default.
    std::array<std::optional<Framebuffer>, 2> m_attachments;  // Per-pass attachment descriptions.
};
}  // namespace openrac::vkgl::detail
