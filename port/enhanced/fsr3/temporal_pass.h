// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Render-thread temporal raster/copy adapters; shader math and displayed-frame history remain with
// their owners.
#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "renderer/vulkan/graph/temporal_graph.h"

namespace openrac::enhanced::fsr3_detail {
// Records one existing draw after explicit temporal dependencies, reporting errors to the enclosing
// frame boundary.
void raster_pass(
    std::string_view name,
    const vkgl::TemporalTarget& target,
    std::span<const unsigned> inputs,
    const std::function<void()>& record
);
// Records a complete existing colour copy or resolve and selects its next image consumer.
void blit_pass(
    std::string_view name,
    unsigned source,
    unsigned destination,
    vkgl::TemporalConsumer consumer,
    const std::function<void()>& record
);
}  // namespace openrac::enhanced::fsr3_detail
