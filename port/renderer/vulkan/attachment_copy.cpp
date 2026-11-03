// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements attachment_copy.h using the native commands and accounting shared with framebuffer blits.
#include "renderer/vulkan/attachment_copy.h"

namespace openrac::vkgl::detail {
void record_depth_resolve(
    VkCommandBuffer command,
    Image& source,
    Image& destination,
    const VkRect2D& area,
    VkImageAspectFlags aspect
) {
    destination.discarded = 0;
    destination.touched = g->recording;
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth.imageView = attachment_view(source);
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    depth.resolveImageView = attachment_view(destination);
    depth.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.renderArea = area;
    info.layerCount = 1;
    info.pDepthAttachment = &depth;
    // Depth-only targets omit stencil; combined targets resolve both aspects with the same sample rule.
    if ((aspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
        info.pStencilAttachment = &depth;
    }
    vkCmdBeginRendering(command, &info);
    vkCmdEndRendering(command);
    count_render_pass();
    count_command(Command::RenderPass);
    count_attachment(source, area.extent, true, true);
    count_attachment(destination, area.extent, false, true);
}

void record_image_copy(
    VkCommandBuffer command, Image& source, Image& destination, std::span<const VkImageCopy> regions
) {
    vkCmdCopyImage(
        command,
        source.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<std::uint32_t>(regions.size()),
        regions.data()
    );
    destination.discarded = 0;
    destination.touched = g->recording;
    count_copy(regions.front().extent.width, regions.front().extent.height);
}

void record_colour_resolve(
    VkCommandBuffer command, Image& source, Image& destination, const VkImageResolve& region
) {
    vkCmdResolveImage(
        command,
        source.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    destination.discarded = 0;
    destination.touched = g->recording;
    count_copy(region.extent.width, region.extent.height);
}
}  // namespace openrac::vkgl::detail
