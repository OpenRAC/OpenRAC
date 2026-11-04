// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Typed transfer recording on allocations already imported and ordered by GraphExecution.
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/graph_execution_internal.h"

namespace openrac::vkgl {
Result<void> GraphExecution::copy_buffer_to_image(
    const graph::BufferUse& source, graph::ImageHandle destination
) {
    using namespace detail;

    // Only the synchronous graph callback may record against these borrowed allocation bindings.
    if (!m_recording->recording || !g || !g->open
        || source.buffer.index >= m_recording->bindings.size()
        || destination.index >= m_recording->bindings.size()) {
        return unexpected(Error{"graph copy requires an active recording and valid handles"});
    }

    const auto& from = m_recording->bindings[source.buffer.index].buffer;
    const auto& to = m_recording->bindings[destination.index].image;

    // The transfer pass must establish the destination layout before this command is recorded.
    if (!from.buffer || !to || to->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        return unexpected(Error{"graph copy requires a buffer and transfer-destination image"});
    }

    const VkDeviceSize bytes = buffer_image_bytes(*to);

    // Full-image copies may not overrun the buffer or use an unsupported colour representation.
    if (!bytes
        || source.state
               != graph::State{graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer}
        || source.range.offset > from.size || source.range.size > from.size - source.range.offset
        || source.range.size < bytes || !(to->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        return unexpected(Error{"graph copy size or image format is unsupported"});
    }

    const VkDeviceSize texel_bytes =
        bytes / static_cast<VkDeviceSize>(to->width) / static_cast<VkDeviceSize>(to->height);

    // Vulkan requires the source byte offset to be aligned to the destination's texel block size.
    if ((from.offset + source.range.offset) % texel_bytes != 0) {
        return unexpected(Error{"graph copy buffer offset is not texel-aligned"});
    }

    record_buffer_image_copy(
        {from.buffer, from.offset + source.range.offset, source.range.size}, *to
    );
    return {};
}
}  // namespace openrac::vkgl
