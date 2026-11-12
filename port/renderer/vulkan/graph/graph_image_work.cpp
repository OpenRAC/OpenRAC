// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures image transfers, mip generation and clears as graph work.
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/graph_immediate_work.h"
#include "renderer/vulkan/graph/graph_pipeline.h"

namespace openrac::vkgl::detail {
namespace graph = renderer::graph;

namespace {
constexpr graph::State kRead{
    graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource
};
constexpr graph::State kWrite{
    graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
};
constexpr graph::State kBufferRead{
    graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer
};
constexpr graph::State kBufferWrite{
    graph::Stage::Transfer, graph::Access::Write, graph::Layout::Buffer
};

graph::ImageRange range_of(VkImageSubresourceLayers layers) {
    return {
        static_cast<graph::Aspect>(layers.aspectMask),
        layers.mipLevel,
        1,
        layers.baseArrayLayer,
        layers.layerCount
    };
}

bool partial_write(const Image& image, unsigned mip, VkOffset3D offset, VkExtent3D extent) {
    return offset.x || offset.y || offset.z || mip >= static_cast<unsigned>(image.levels)
           || extent.width != static_cast<unsigned>(std::max(image.width >> mip, 1))
           || extent.height != static_cast<unsigned>(std::max(image.height >> mip, 1))
           || extent.depth != 1;
}

void require(Result<void> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

void GraphImmediateWork::transfer(
    std::string_view name,
    VkCommandBuffer command,
    std::span<const ImageAccess> images,
    std::span<const BufferAccess> buffers,
    const std::function<void()>& record,
    std::source_location site
) {
    // A current-pass operation must match its resource contract. Independent
    // upload/readback streams still compile their own graph below.
    if (graph_pipeline_records(command)) {
        for (const auto& use : images) {
            validate_graph_image_access(*use.image, use.range, use.state, use.partial_write);
        }
        for (const auto& use : buffers) {
            validate_graph_buffer_access(use.buffer.buffer, use.range, use.state);
        }
        record();
        return;
    }
    GraphExecution execution(site);
    auto& recording = *execution.m_recording;
    recording.target = command;
    auto pass = execution.make_pass(name);
    pass.images.reserve(images.size());
    pass.buffers.reserve(buffers.size());
    for (const auto& use : images) {
        auto image = use.image->owner.lock();
        if (!image) {
            // Acquired/external images borrow allocation ownership from their
            // presentation/interop scope; retain independent metadata, not a stack address.
            image = std::make_shared<Image>(*use.image);
        }
        const auto handle = recording.import_image("transfer image", std::move(image), true);
        if (!handle) {
            throw std::runtime_error(handle.error().message);
        }
        pass.images.push_back(
            {*handle,
             use.range,
             use.state,
             graph::Load::None,
             graph::Store::Preserve,
             use.partial_write}
        );
    }
    for (const auto& use : buffers) {
        const auto handle = recording.import_buffer("transfer buffer", use.buffer, true);
        if (!handle) {
            throw std::runtime_error(handle.error().message);
        }
        pass.buffers.push_back({*handle, use.range, use.state});
    }
    execution.add_pass(std::move(pass));
    require(execution.run([&](unsigned) { record(); }));
    for (const auto& use : images) {
        // Owned images have already been updated by the graph. Copy the state back for the
        // borrowed external metadata as well.
        if (tracks_subresources(*use.image) || !use.image->owner.expired()) {
            continue;
        }
        use.image->layout = layout_of(use.state.layout);
        if (use.state.access == graph::Access::Write && !use.partial_write) {
            use.image->discarded &= ~static_cast<VkImageAspectFlags>(use.range.aspects);
        }
    }
}

void GraphImmediateWork::copy_image(
    VkCommandBuffer command, Image& source, Image& destination, std::span<const VkImageCopy> regions
) {
    std::array<ImageAccess, 8> local{};
    std::vector<ImageAccess> multiple;
    std::span<ImageAccess> uses;
    if (regions.size() <= local.size() / 2) {
        uses = std::span(local).first(regions.size() * 2);
    } else {
        multiple.resize(regions.size() * 2);
        uses = multiple;
    }
    for (std::size_t index = 0; index < regions.size(); ++index) {
        const auto& region = regions[index];
        uses[index * 2] = {&source, range_of(region.srcSubresource), kRead};
        uses[index * 2 + 1] =
            {&destination,
             range_of(region.dstSubresource),
             kWrite,
             partial_write(
                 destination, region.dstSubresource.mipLevel, region.dstOffset, region.extent
             )};
    }
    transfer("image copy", command, uses, {}, [&] {
        NativeCommandEncoder{command}.copy_image(
            source.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<unsigned>(regions.size()),
            regions.data()
        );
    });
}

void GraphImmediateWork::blit_image(
    VkCommandBuffer command, Image& source, Image& destination, VkImageBlit region, VkFilter filter
) {
    const auto& a = region.dstOffsets[0];
    const auto& b = region.dstOffsets[1];
    const VkOffset3D origin{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
    const VkExtent3D extent{
        static_cast<unsigned>(std::max(a.x, b.x) - origin.x),
        static_cast<unsigned>(std::max(a.y, b.y) - origin.y),
        static_cast<unsigned>(std::max(a.z, b.z) - origin.z)
    };
    const ImageAccess uses[]{
        {&source, range_of(region.srcSubresource), kRead},
        {&destination,
         range_of(region.dstSubresource),
         kWrite,
         partial_write(destination, region.dstSubresource.mipLevel, origin, extent)}
    };
    transfer("image blit", command, uses, {}, [&] {
        NativeCommandEncoder{command}.blit_image(
            source.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region,
            filter
        );
    });
}

void GraphImmediateWork::resolve_image(
    VkCommandBuffer command, Image& source, Image& destination, VkImageResolve region
) {
    const ImageAccess uses[]{
        {&source, range_of(region.srcSubresource), kRead},
        {&destination,
         range_of(region.dstSubresource),
         kWrite,
         partial_write(
             destination, region.dstSubresource.mipLevel, region.dstOffset, region.extent
         )}
    };
    transfer("colour resolve", command, uses, {}, [&] {
        NativeCommandEncoder{command}.resolve_image(
            source.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    });
}

void GraphImmediateWork::upload_image(
    VkCommandBuffer command, BufferRef source, Image& destination, VkBufferImageCopy region
) {
    const ImageAccess images[]{
        {&destination,
         range_of(region.imageSubresource),
         kWrite,
         partial_write(
             destination, region.imageSubresource.mipLevel, region.imageOffset, region.imageExtent
         )}
    };
    const BufferAccess buffers[]{{source, {source.offset, source.size}, kBufferRead}};
    transfer("image upload", command, images, buffers, [&] {
        NativeCommandEncoder{command}.copy_buffer_to_image(
            source.buffer, destination.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
        );
    });
}

void GraphImmediateWork::read_image(
    VkCommandBuffer command, Image& source, BufferRef destination, VkBufferImageCopy region
) {
    const ImageAccess images[]{{&source, range_of(region.imageSubresource), kRead}};
    const BufferAccess buffers[]{
        {destination, {destination.offset, destination.size}, kBufferWrite}
    };
    transfer("image readback", command, images, buffers, [&] {
        NativeCommandEncoder{command}.copy_image_to_buffer(
            source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination.buffer, 1, &region
        );
    });
}

void GraphImmediateWork::generate_mipmaps(VkCommandBuffer command, const ImagePtr& image) {
    GraphExecution execution;
    auto& recording = *execution.m_recording;
    recording.target = command;
    const auto handle = recording.import_image("mipmap chain", image, true);
    if (!handle) {
        throw std::runtime_error(handle.error().message);
    }
    std::vector<VkImageBlit> blits;
    int width = image->width, height = image->height;
    for (int mip = 1; mip < image->levels; ++mip) {
        const int next_width = std::max(width / 2, 1), next_height = std::max(height / 2, 1);
        VkImageBlit blit{};
        blit.srcSubresource = {image->aspect, static_cast<unsigned>(mip - 1), 0, image->layers};
        blit.srcOffsets[1] = {width, height, 1};
        blit.dstSubresource = {image->aspect, static_cast<unsigned>(mip), 0, image->layers};
        blit.dstOffsets[1] = {next_width, next_height, 1};
        blits.push_back(blit);
        execution.add_pass(
            {"generate mip " + std::to_string(mip),
             {{*handle, range_of(blit.srcSubresource), kRead},
              {*handle, range_of(blit.dstSubresource), kWrite}}}
        );
        width = next_width;
        height = next_height;
    }
    execution.add_pass(
        {"sample mipmap chain",
         {{*handle,
           whole_image(*image),
           {graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader,
            graph::Access::Read,
            graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
    require(execution.run([&](unsigned index) {
        const auto& blit = blits.at(index);
        NativeCommandEncoder{command}.blit_image(
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR
        );
        count_copy(
            static_cast<unsigned>(blit.dstOffsets[1].x), static_cast<unsigned>(blit.dstOffsets[1].y)
        );
    }));
}

void GraphImmediateWork::clear_image(
    VkCommandBuffer command,
    Image& image,
    VkImageSubresourceRange range,
    VkClearColorValue colour,
    VkClearDepthStencilValue depth
) {
    if (range.levelCount == VK_REMAINING_MIP_LEVELS) {
        range.levelCount = static_cast<unsigned>(image.levels) - range.baseMipLevel;
    }
    if (range.layerCount == VK_REMAINING_ARRAY_LAYERS) {
        range.layerCount = image.layers - range.baseArrayLayer;
    }
    const ImageAccess use{
        &image,
        {static_cast<graph::Aspect>(range.aspectMask),
         range.baseMipLevel,
         range.levelCount,
         range.baseArrayLayer,
         range.layerCount},
        kWrite
    };
    transfer("clear image", command, std::span{&use, 1}, {}, [&] {
        if (range.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
            NativeCommandEncoder{command}.clear_color_image(
                image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colour, 1, &range
            );
        } else {
            NativeCommandEncoder{command}.clear_depth_stencil_image(
                image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depth, 1, &range
            );
        }
    });
}

void GraphImmediateWork::clear_attachments(
    const std::function<void()>& record, VkImageAspectFlags aspects
) {
    GraphExecution execution;
    auto pass = execution.make_pass("clear attachments");
    const auto target = target_of(g->state.draw_framebuffer);
    if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
        capture_attachment(execution, pass, target.colour, graph::Aspect::Colour);
    }
    if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
        capture_attachment(execution, pass, target.depth, graph::Aspect::Depth);
    }
    if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
        capture_attachment(execution, pass, target.depth, graph::Aspect::Stencil);
    }
    execution.add_pass(std::move(pass));
    require(execution.run([&](unsigned) { record(); }));
}

void GraphImmediateWork::resolve_depth(
    VkCommandBuffer command,
    Image& source,
    Image& destination,
    VkRect2D area,
    VkImageAspectFlags aspects
) {
    const auto stages = graph::Stage::DepthTests | graph::Stage::ColourOutput;
    const graph::ImageRange range{static_cast<graph::Aspect>(aspects)};
    const ImageAccess uses[]{
        {&source, range, {stages, graph::Access::Read, graph::Layout::DepthAttachment}},
        {&destination,
         range,
         {stages, graph::Access::Write, graph::Layout::DepthAttachment},
         partial_write(
             destination,
             0,
             {area.offset.x, area.offset.y, 0},
             {area.extent.width, area.extent.height, 1}
         )}
    };
    const auto source_view = attachment_view(source),
               destination_view = attachment_view(destination);
    transfer("depth resolve", command, uses, {}, [&] {
        NativeCommandEncoder::note_image_work();
        VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depth.imageView = source_view;
        depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        depth.resolveImageView = destination_view;
        depth.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering.renderArea = area;
        rendering.layerCount = 1;
        if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
            rendering.pDepthAttachment = &depth;
        }
        if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
            rendering.pStencilAttachment = &depth;
        }
        NativeCommandEncoder{command}.begin_rendering(&rendering);
        NativeCommandEncoder{command}.end_rendering();
    });
    if (!uses[1].partial_write) {
        destination.discarded &= ~aspects;
    }
}
}  // namespace openrac::vkgl::detail
