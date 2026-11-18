// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Records buffer-only plans on an independent Vulkan device without renderer context state.
#pragma once

#include <functional>
#include <volk.h>

#include "renderer/graph/frame_graph.h"

namespace openrac::vkgl {

// Resource keys carry native buffer handles; the caller retains their owners through completion.
void record_buffer_graph(
    VkCommandBuffer command,
    const renderer::graph::Plan& plan,
    const std::function<void(unsigned)>& record
);

}  // namespace openrac::vkgl
