// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private presentation graph recording on the already chosen main or presentation command buffer.
#pragma once

#include <functional>

#include "common/result.h"
#include "renderer/graph/command_buffer.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
struct PresentationEncode {
    Image* staging = nullptr;
    std::function<void()> record;
};

// Borrows the source owner through the callback and the acquired screen allocation; its semaphore
// and swapchain lifetime remain context-owned.
// `screen` selects PRESENT exit; offscreen regressions export a transfer-readable stand-in instead.
[[nodiscard]] Result<void> record_presentation(
    VkCommandBuffer command,
    const ImagePtr& source,
    Image& destination,
    bool screen,
    const std::function<void()>& copy,
    const PresentationEncode& encode = {}
);
[[nodiscard]] Result<void> record_presentation_blit(
    VkCommandBuffer command,
    const ImagePtr& source,
    Image& destination,
    bool screen,
    const renderer::graph::BlitRegion& blit
);
}  // namespace openrac::vkgl::detail
