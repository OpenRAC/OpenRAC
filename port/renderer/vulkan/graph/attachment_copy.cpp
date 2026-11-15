// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements attachment_copy.h using the native commands and accounting shared with framebuffer blits.
#include "renderer/vulkan/graph/attachment_copy.h"

#include "renderer/vulkan/graph/graph_immediate_work.h"

namespace openrac::vkgl::detail {
void record_depth_resolve(
    VkCommandBuffer command,
    Image& source,
    Image& destination,
    const VkRect2D& area,
    VkImageAspectFlags aspect
) {
    GraphImmediateWork::resolve_depth(command, source, destination, area, aspect);
    destination.touched = g->recording;
    count_render_pass();
    count_command(Command::RenderPass);
    count_attachment(source, area.extent, true, true);
    count_attachment(destination, area.extent, false, true);
}

void record_image_copy(
    VkCommandBuffer command, Image& source, Image& destination, std::span<const VkImageCopy> regions
) {
    GraphImmediateWork::copy_image(command, source, destination, regions);
    destination.touched = g->recording;
    count_copy(regions.front().extent.width, regions.front().extent.height);
}

void record_colour_resolve(
    VkCommandBuffer command, Image& source, Image& destination, const VkImageResolve& region
) {
    GraphImmediateWork::resolve_image(command, source, destination, region);
    destination.discarded = 0;
    destination.touched = g->recording;
    count_copy(region.extent.width, region.extent.height);
}
}  // namespace openrac::vkgl::detail
