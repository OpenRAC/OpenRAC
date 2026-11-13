// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native graph state translation and whole-image barrier recording.
#pragma once
#include "renderer/graph/command_buffer.h"
#include "renderer/vulkan/graph/graph_state_translation.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// Batches independent transitions at one pass boundary, retaining each entry's stage masks.
class GraphBarrierBatch {
public:
    explicit GraphBarrierBatch(VkCommandBuffer command) : m_command(command) {}

    ~GraphBarrierBatch();
    GraphBarrierBatch(const GraphBarrierBatch&) = delete;
    GraphBarrierBatch& operator=(const GraphBarrierBatch&) = delete;

    void add(
        VkPipelineStageFlags source,
        VkPipelineStageFlags destination,
        const VkImageMemoryBarrier& barrier
    );
    void add(
        VkPipelineStageFlags source,
        VkPipelineStageFlags destination,
        const VkBufferMemoryBarrier& barrier
    );
    void add(const BufferWritesBarrier& writes);

    [[nodiscard]] VkCommandBuffer command() const { return m_command; }

private:
    void flush();

    VkCommandBuffer m_command;
    // Fixed capacity keeps pass recording allocation-free even for highly fragmented resources.
    std::array<VkImageMemoryBarrier2, 8> m_images;
    std::array<VkBufferMemoryBarrier2, 8> m_buffers;
    VkMemoryBarrier2 m_writes{};
    unsigned m_image_count = 0;
    unsigned m_buffer_count = 0;
    VkPipelineStageFlags m_sources = 0;
    VkPipelineStageFlags m_destinations = 0;
};

[[nodiscard]] bool graph_buffer_reads_visible(
    VkBuffer buffer, const renderer::graph::Barrier& transition
);
void invalidate_graph_buffer_reads(VkBuffer buffer, renderer::graph::BufferRange bytes);
// Records an exact buffer transition on the command buffer that owns its producer.
void record_graph_buffer_barrier(
    GraphBarrierBatch& batch,
    const renderer::graph::Barrier& transition,
    const BufferRef& buffer,
    renderer::graph::BufferKind kind = renderer::graph::BufferKind::Data
);
// Tracks ranged textures independently of single-level framebuffer attachment bookkeeping.
[[nodiscard]] bool tracks_subresources(const Image& image);
[[nodiscard]] renderer::graph::ImageRange whole_image(const Image& image);
[[nodiscard]] renderer::graph::ImageContents image_contents(
    const Image& image, renderer::graph::Aspect aspect, unsigned mip, unsigned layer
);
[[nodiscard]] bool image_has_layout(
    const Image& image, const renderer::graph::ImageRange& range, VkImageLayout layout
);
void set_whole_image_state(Image& image, renderer::graph::State state);
void discard_image_contents(Image& image, const renderer::graph::ImageRange& range);
void record_graph_image_access(
    Image& image, const renderer::graph::ImageUse& use, bool completed = true
);
void record_sampled_image_access(const Sampled& sampled, renderer::graph::Stage stages);
void record_coupled_image_layout(
    VkCommandBuffer command, Image& image, const renderer::graph::ImageUse& use
);
[[nodiscard]] renderer::graph::ImageRange sampled_range(const Image& image, int base, int top);
// Records a subresource transition, keeping packed depth/stencil layouts coupled.
void record_graph_image_barrier(
    VkCommandBuffer command,
    const renderer::graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source = 0,
    GraphBarrierBatch* batch = nullptr
);
[[nodiscard]] Result<void> validate_graph_blit(
    const renderer::graph::BlitRegion& region,
    const Image& source,
    const Image& destination,
    bool deferred = false
);
void record_graph_blit(
    VkCommandBuffer command,
    const renderer::graph::BlitRegion& region,
    const Image& source,
    Image& destination
);
}  // namespace openrac::vkgl::detail
