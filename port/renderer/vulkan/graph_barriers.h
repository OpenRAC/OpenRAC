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
// Translates a buffer range for callers that batch transitions in one command.
[[nodiscard]] VkBufferMemoryBarrier graph_buffer_barrier(
    const renderer::graph::Barrier& transition, const BufferRef& buffer
);

// The stages a transition waits on and for.
struct GraphStages {
    VkPipelineStageFlags source;
    VkPipelineStageFlags destination;
};

// A buffer transition's stages: stages_of, but an unknown earlier use names the stages that use
// buffers instead of every stage (vk-tight-sync).
[[nodiscard]] GraphStages graph_buffer_stages(const renderer::graph::Barrier& transition);

// One whole-image transition for callers that batch transitions in one command; the image's
// tracked layout is moved on already.
struct GraphImageBarrier {
    VkImageMemoryBarrier barrier;
    VkPipelineStageFlags source;
    VkPipelineStageFlags destination;
};

[[nodiscard]] GraphImageBarrier graph_image_barrier(
    const renderer::graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source = 0
);
// Records one whole-image transition, preserving the shared depth/stencil layout tracker.
void record_graph_image_barrier(
    VkCommandBuffer command,
    const renderer::graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source = 0
);
}  // namespace openrac::vkgl::detail
