// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Executes graph submission and presentation plans on a native queue.
#pragma once

#include <volk.h>

#include "renderer/graph/submission.h"

namespace openrac::vkgl {

// Only the queue executor lowers a compiled submission plan into VkSubmitInfo.
// The plan owns batch order and the placement/stages of external waits/signals.
[[nodiscard]] VkResult submit_graph_queue(
    VkQueue queue, const renderer::graph::SubmissionPlan& plan, VkFence completion
);
[[nodiscard]] VkResult submit_graph_queue(
    VkQueue queue, const renderer::graph::FrameSubmissionPlan& plan, VkFence completion
);
[[nodiscard]] VkResult present_graph_queue(
    VkQueue queue, const renderer::graph::PresentImage& image, const void* extensions = nullptr
);

}  // namespace openrac::vkgl
