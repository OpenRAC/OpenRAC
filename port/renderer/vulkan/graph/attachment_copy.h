// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native attachment copies shared by graph recording and GL-shaped blits; callers establish layouts.
#pragma once
#include <span>

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// Resolves sample-zero depth/stencil inside `area`, with both images in depth/stencil attachment layout.
void record_depth_resolve(
    VkCommandBuffer command,
    Image& source,
    Image& destination,
    const VkRect2D& area,
    VkImageAspectFlags aspect
);
// Copies nonempty same-sample regions with transfer layouts, without adding another barrier.
void record_image_copy(
    VkCommandBuffer command, Image& source, Image& destination, std::span<const VkImageCopy> regions
);
// Averages multisample colour with source/destination transfer layouts, without adding another barrier.
void record_colour_resolve(
    VkCommandBuffer command, Image& source, Image& destination, const VkImageResolve& region
);
}  // namespace openrac::vkgl::detail
