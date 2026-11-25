// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Context-independent Vulkan states and buffer barriers for graph executors.
#pragma once

#include <volk.h>

#include "renderer/graph/frame_graph.h"

namespace openrac::vkgl::detail {

// Imports preserve every possible access associated with the backend's prior image layout.
[[nodiscard]] Result<renderer::graph::State> imported_state(VkImageLayout layout);
// Combined scopes retain every participating stage and access type.
VkPipelineStageFlags stages_of(renderer::graph::Stage stages);
VkAccessFlags access_of(
    renderer::graph::State state,
    renderer::graph::BufferKind kind = renderer::graph::BufferKind::Data
);
VkImageLayout layout_of(renderer::graph::Layout layout);
// Depth resolves use attachment stages; other copies use transfer stages.
[[nodiscard]] renderer::graph::State attachment_copy_state(
    bool depth_resolve, renderer::graph::Access access
);
// Reads depend on earlier writes, while overwrites also wait for readers.
[[nodiscard]] VkPipelineStageFlags graph_buffer_source_stages(
    const renderer::graph::Barrier& transition, bool tight = false
);
// Offset locates a graph allocation within a larger native buffer.
[[nodiscard]] VkBufferMemoryBarrier graph_buffer_barrier(
    const renderer::graph::Barrier& transition,
    VkBuffer buffer,
    VkDeviceSize offset = 0,
    renderer::graph::BufferKind kind = renderer::graph::BufferKind::Data,
    bool tight = false
);

}  // namespace openrac::vkgl::detail
