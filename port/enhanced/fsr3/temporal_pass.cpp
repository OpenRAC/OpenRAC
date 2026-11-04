// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements temporal_pass.h with synchronous native graph recording and the renderer's existing
// frame error boundary.
#include "enhanced/fsr3/temporal_pass.h"

#include <stdexcept>

namespace openrac::enhanced::fsr3_detail {
void raster_pass(
    std::string_view name,
    const vkgl::TemporalTarget& target,
    std::span<const unsigned> inputs,
    const std::function<void()>& record
) {
    const auto result = vkgl::temporal_raster(name, target, inputs, record);

    // Recording failure stops the current frame before partially produced history can be retained.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

void blit_pass(
    std::string_view name,
    unsigned source,
    unsigned destination,
    vkgl::TemporalConsumer consumer,
    const std::function<void()>& record
) {
    const auto result = vkgl::temporal_blit(name, source, destination, consumer, record);

    // Invalid or aliased copies cannot supply a complete reconstructed image to later consumers.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace openrac::enhanced::fsr3_detail
