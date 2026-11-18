// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Typed transfer recording on allocations already imported and ordered by GraphExecution.
#include <algorithm>
#include <cstdlib>

#include "renderer/graph/breadcrumbs.h"
#include "renderer/vulkan/graph/attachment_copy.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"

namespace openrac::vkgl::detail {
Result<void> validate_graph_blit(
    const renderer::graph::BlitRegion& region,
    const Image& source,
    const Image& destination,
    bool deferred
) {
    const auto fits = [](const auto& begin, const auto& end, const Image& image) {
        return begin[0] != end[0] && begin[1] != end[1] && std::min(begin[0], end[0]) >= 0
               && std::min(begin[1], end[1]) >= 0 && std::max(begin[0], end[0]) <= image.width
               && std::max(begin[1], end[1]) <= image.height;
    };
    const bool full =
        std::abs(std::int64_t{region.destination_end[0]} - region.destination_begin[0])
            == destination.width
        && std::abs(std::int64_t{region.destination_end[1]} - region.destination_begin[1])
               == destination.height;
    if ((!deferred && (!source.image || !destination.image)) || &source == &destination
        || (source.image && source.image == destination.image)
        || source.aspect != VK_IMAGE_ASPECT_COLOR_BIT
        || destination.aspect != VK_IMAGE_ASPECT_COLOR_BIT
        || source.samples != VK_SAMPLE_COUNT_1_BIT || destination.samples != VK_SAMPLE_COUNT_1_BIT
        || !(source.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        || !(destination.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        || !fits(region.source_begin, region.source_end, source)
        || !fits(region.destination_begin, region.destination_end, destination)
        || (!full && !region.clear)) {
        return unexpected(Error{"graph blit requires valid colour regions and a complete write"});
    }
    return {};
}

void record_graph_blit(
    VkCommandBuffer command,
    const renderer::graph::BlitRegion& region,
    const Image& source,
    Image& destination
) {
    namespace graph = renderer::graph;
    if (region.clear) {
        VkClearColorValue clear{};
        std::ranges::copy(*region.clear, clear.float32);
        constexpr VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        NativeCommandEncoder{command}.clear_color_image(
            destination.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range
        );
        count_command(Command::Copy);
        // The blit can overwrite pixels initialized by the clear in the same command.
        constexpr graph::State write{
            graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
        };
        record_graph_image_barrier(command, {0, graph::ImageRange{}, write, write}, destination);
    }
    constexpr VkImageSubresourceLayers layer{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    const VkImageBlit blit{
        layer,
        {{region.source_begin[0], region.source_begin[1], 0},
         {region.source_end[0], region.source_end[1], 1}},
        layer,
        {{region.destination_begin[0], region.destination_begin[1], 0},
         {region.destination_end[0], region.destination_end[1], 1}},
    };
    NativeCommandEncoder{command}.blit_image(
        source.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        region.linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST
    );
    destination.discarded = 0;
    count_copy(
        static_cast<std::uint32_t>(
            std::abs(region.destination_end[0] - region.destination_begin[0])
        ),
        static_cast<std::uint32_t>(
            std::abs(region.destination_end[1] - region.destination_begin[1])
        )
    );
}
}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {
Result<void> GraphExecution::validate_commands(const graph::CommandBuffer& commands) {
    for (const auto& command : commands.commands()) {
        const auto checked = transfer_command(command, false);
        if (!checked) {
            return checked;
        }
    }
    return {};
}

Result<void> GraphExecution::execute(const graph::CommandBuffer& commands) {
    graph::BreadcrumbScope operation{"graph commands"};
    for (const auto& command : commands.commands()) {
        const auto recorded = transfer_command(command, true);
        if (!recorded) {
            return recorded;
        }
    }
    operation.complete();
    return {};
}

Result<void> GraphExecution::transfer_command(const graph::Command& command, bool encode) {
    if (const auto* copy = std::get_if<graph::BufferCopy>(&command)) {
        return transfer_buffer(*copy, encode);
    }
    if (const auto* copy = std::get_if<graph::BufferToImageCopy>(&command)) {
        return transfer_buffer_to_image(*copy, encode);
    }
    if (const auto* blit = std::get_if<graph::ImageBlit>(&command)) {
        return transfer_image_blit(*blit, encode);
    }
    return transfer_full_image(std::get<graph::FullImageTransfer>(command), encode);
}

Result<void> GraphExecution::transfer_buffer(const graph::BufferCopy& copy, bool encode) {
    using namespace detail;
    const auto& bindings = m_recording->bindings;
    if (copy.source.index >= bindings.size() || copy.destination.index >= bindings.size()) {
        return unexpected(Error{"buffer copy requires valid graph handles"});
    }
    const auto source = bindings[copy.source.index].buffer;
    const auto destination = bindings[copy.destination.index].buffer;
    if (!source.buffer || !destination.buffer || !copy.bytes.size || copy.bytes.offset > source.size
        || copy.bytes.size > source.size - copy.bytes.offset
        || copy.destination_offset > destination.size
        || copy.bytes.size > destination.size - copy.destination_offset
        || ((source.offset + copy.bytes.offset) % 4) != 0
        || ((destination.offset + copy.destination_offset) % 4) != 0) {
        return unexpected(Error{"buffer copy range or alignment is invalid"});
    }
    if (encode) {
        const VkBufferCopy region{
            source.offset + copy.bytes.offset,
            destination.offset + copy.destination_offset,
            copy.bytes.size
        };
        NativeCommandEncoder{m_recording->command()}
            .copy_buffer(source.buffer, destination.buffer, 1, &region);
        count_command(Command::Copy);
    }
    return {};
}

Result<void> GraphExecution::transfer_full_image(
    const graph::FullImageTransfer& transfer, bool encode
) {
    using namespace detail;
    const auto& bindings = m_recording->bindings;
    if (!g || !g->open || transfer.source.index >= bindings.size()
        || transfer.destination.index >= bindings.size()) {
        return unexpected(Error{"surface transfer requires valid image handles"});
    }
    const auto& source = bindings[transfer.source.index].image;
    const auto& destination = bindings[transfer.destination.index].image;
    if (!source || !destination || source == destination || source->format != destination->format
        || source->width != destination->width || source->height != destination->height
        || source->width <= 0 || source->height <= 0) {
        return unexpected(
            Error{"surface transfer requires matching independent image allocations"}
        );
    }
    using Kind = graph::ImageTransferKind;
    const bool depth = (source->aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
    const bool copy = transfer.kind == Kind::Copy;
    const bool depth_resolve = transfer.kind == Kind::ResolveDepth;
    const auto aspects = static_cast<VkImageAspectFlags>(transfer.aspect);
    if (!aspects || (aspects & source->aspect) != aspects
        || (aspects & destination->aspect) != aspects
        || (copy && source->samples != destination->samples)
        || (!copy
            && (depth != depth_resolve || source->samples == VK_SAMPLE_COUNT_1_BIT
                || destination->samples != VK_SAMPLE_COUNT_1_BIT))) {
        return unexpected(Error{"surface transfer has incompatible aspects or sample counts"});
    }
    if (!encode) {
        return {};
    }
    const auto from = layout_of(attachment_copy_state(depth_resolve, graph::Access::Read).layout);
    const auto to = layout_of(attachment_copy_state(depth_resolve, graph::Access::Write).layout);
    if (!image_has_layout(*source, {transfer.aspect}, from)
        || !image_has_layout(*destination, {transfer.aspect}, to)) {
        return unexpected(Error{"surface transfer requires the compiled image layouts"});
    }
    const VkExtent3D extent{
        static_cast<std::uint32_t>(source->width), static_cast<std::uint32_t>(source->height), 1
    };
    const auto command = m_recording->command();
    if (depth_resolve) {
        record_depth_resolve(
            command, *source, *destination, {{0, 0}, {extent.width, extent.height}}, aspects
        );
    } else if (!copy) {
        const VkImageSubresourceLayers layer{aspects, 0, 0, 1};
        record_colour_resolve(command, *source, *destination, {layer, {}, layer, {}, extent});
    } else {
        std::array<VkImageCopy, 2> regions{};
        std::size_t count = 0;
        // Layouts are coupled, but copies must touch only the declared aspects.
        for (auto aspect :
             {VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_ASPECT_STENCIL_BIT}) {
            if ((aspects & aspect) != 0) {
                const VkImageSubresourceLayers layer{
                    static_cast<VkImageAspectFlags>(aspect), 0, 0, 1
                };
                regions[count++] = {layer, {}, layer, {}, extent};
            }
        }
        record_image_copy(command, *source, *destination, std::span(regions).first(count));
    }
    return {};
}

Result<void> GraphExecution::transfer_image_blit(const graph::ImageBlit& blit, bool encode) {
    using namespace detail;
    const auto& bindings = m_recording->bindings;
    if (!g || !g->open || blit.source.index >= bindings.size()
        || blit.destination.index >= bindings.size()) {
        return unexpected(Error{"graph blit requires valid image handles"});
    }
    const auto& source = bindings[blit.source.index].image;
    const auto& destination = bindings[blit.destination.index].image;
    if (!source || !destination) {
        return unexpected(Error{"graph blit requires bound image allocations"});
    }
    if (encode
        && (!image_has_layout(*source, {}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            || !image_has_layout(*destination, {}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))) {
        return unexpected(Error{"graph blit requires transfer image layouts"});
    }
    const bool deferred = !encode
                          && (bindings[blit.source.index].transient_image
                              || bindings[blit.destination.index].transient_image);
    const auto valid = validate_graph_blit(blit.region, *source, *destination, deferred);
    if (valid && encode) {
        record_graph_blit(m_recording->command(), blit.region, *source, *destination);
    }
    return valid;
}

Result<void> GraphExecution::transfer_buffer_to_image(
    const graph::BufferToImageCopy& copy, bool encode
) {
    using namespace detail;

    if (!g || !g->open || copy.source.index >= m_recording->bindings.size()
        || copy.destination.index >= m_recording->bindings.size()) {
        return unexpected(Error{"graph copy requires an active recording and valid handles"});
    }

    const auto& from = m_recording->bindings[copy.source.index].buffer;
    const auto& to = m_recording->bindings[copy.destination.index].image;

    // The transfer pass must establish the destination layout before this command is recorded.
    if (!from.buffer || !to
        || (encode && !image_has_layout(*to, {}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))) {
        return unexpected(Error{"graph copy requires a buffer and transfer-destination image"});
    }

    const VkDeviceSize bytes = buffer_image_bytes(*to);

    // Full-image copies may not overrun the buffer or use an unsupported colour representation.
    if (!bytes || copy.bytes.offset > from.size || copy.bytes.size > from.size - copy.bytes.offset
        || copy.bytes.size < bytes || !(to->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        return unexpected(Error{"graph copy size or image format is unsupported"});
    }

    const VkDeviceSize texel_bytes =
        bytes / static_cast<VkDeviceSize>(to->width) / static_cast<VkDeviceSize>(to->height);

    // Vulkan requires the source byte offset to be aligned to the destination's texel block size.
    if ((from.offset + copy.bytes.offset) % texel_bytes != 0) {
        return unexpected(Error{"graph copy buffer offset is not texel-aligned"});
    }

    if (encode) {
        record_buffer_image_copy(
            {from.buffer, from.offset + copy.bytes.offset, copy.bytes.size}, *to
        );
    }
    return {};
}
}  // namespace openrac::vkgl
