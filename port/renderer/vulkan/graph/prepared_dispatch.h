// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Immutable per-frame compute bindings, prepared before graph compilation and never reused across
// frame slots.
#pragma once
#include <array>

#include "renderer/graph/command_buffer.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// Descriptors and offsets borrow frame storage; graph bindings pin sampled image allocations.
struct PreparedDispatch {
    VkPipeline pipeline =
        VK_NULL_HANDLE;  // Program-owned pipeline, stable through scoped execution.
    VkPipelineLayout layout = VK_NULL_HANDLE;      // Matching immutable program layout.
    VkDescriptorSet descriptors = VK_NULL_HANDLE;  // Frame-pool descriptor allocation.
    std::uint32_t uniform_offset = 0;  // Frozen offset of the frame's loose uniform upload.
    bool dynamic_uniforms = false;     // Whether the layout takes the frozen dynamic offset.
    renderer::graph::Dispatch command{};
};

// Records only the frozen bindings and dispatch; graph entry barriers already establish visibility.
void record_prepared_dispatch(const PreparedDispatch& packet);
}  // namespace openrac::vkgl::detail
