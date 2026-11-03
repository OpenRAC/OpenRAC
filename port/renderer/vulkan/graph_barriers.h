// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native graph state translation and whole-image barrier recording.
#pragma once
#include "renderer/graph/frame_graph.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// Translates legacy entry state and graph execution scopes into Vulkan equivalents.
[[nodiscard]] Result<renderer::graph::State> imported_state(VkImageLayout layout);
VkPipelineStageFlags stages_of(renderer::graph::Stage stages);
VkAccessFlags access_of(renderer::graph::State state);
VkImageLayout layout_of(renderer::graph::Layout layout);
// Returns native copy/resolve scopes; depth resolves use attachment stages instead of transfer.
[[nodiscard]] renderer::graph::State attachment_copy_state(
    bool depth_resolve, renderer::graph::Access access
);
// Records one whole-image transition, preserving the shared depth/stencil layout tracker.
void record_graph_image_barrier(
    VkCommandBuffer command,
    const renderer::graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source = 0
);
}  // namespace openrac::vkgl::detail
