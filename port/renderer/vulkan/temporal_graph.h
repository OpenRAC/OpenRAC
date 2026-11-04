// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Explicit image dependencies for temporal raster passes, colour copies and opaque native
// upscalers.
#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

#include "common/result.h"
#include "renderer/graph/frame_graph.h"
#include "renderer/vulkan/native_compute.h"

namespace openrac::vkgl {
// Prepares temporal work, flushing at native SDK decisions before their result is consumed.
// Callbacks and owners live until this synchronous call returns; history commits follow success.
// Inside an enclosing world chain, finishes its pending section before returning to the history owner.
[[nodiscard]] Result<void> temporal_chain(const std::function<void()>& prepare);

// Complete fullscreen outputs discard old contents; overlays and discard-based gathers preserve
// them.
enum class TemporalContents : std::uint8_t {
    Overwrite,
    Preserve
};
// Intermediates feed shaders; final display surfaces return to their attachment roles before HUD
// drawing.
enum class TemporalConsumer : std::uint8_t {
    Shader,
    Attachment
};

// One borrowed attachment allocation, stable through the synchronous recording callback.
struct TemporalTarget {
    unsigned framebuffer = 0;  // Renderer-owned target, or the window's default target.
    TemporalContents contents = TemporalContents::Overwrite;  // Actual shader coverage promise.
    renderer::graph::Aspect aspect = renderer::graph::Aspect::Colour;  // Written attachment.
    bool depth_test = false;  // Borrows the same framebuffer's depth for read-only depth testing.
    TemporalConsumer consumer = TemporalConsumer::Shader;  // Required exit layout.
};

// Records a raster phase with explicit sampled images and an optional read-only depth attachment.
// Inputs must name defined single-level images; `record` must honour the declared
// overwrite/preserve policy.
[[nodiscard]] Result<void> temporal_raster(
    std::string_view name,
    const TemporalTarget& target,
    std::span<const unsigned> inputs,
    const std::function<void()>& record
);
// Records a complete colour copy/resolve, preserving native scaling, sample conversion and
// destination coverage.
// Destination contents are overwritten completely; partial blits must use a separate preserved
// declaration.
// Window-directed copies keep native deferral; their actual GPU copy is declared by the
// presentation graph.
[[nodiscard]] Result<void> temporal_blit(
    std::string_view name,
    unsigned source,
    unsigned destination,
    TemporalConsumer consumer,
    const std::function<void()>& record
);
// Declares external renderer images around a native upscaler, leaving its private history/barriers
// opaque.
// The callback restores sampled-read/GENERAL layouts before returning.
[[nodiscard]] bool temporal_compute(
    std::string_view name,
    std::span<const NativeImageUse> inputs,
    const std::function<bool(VkCommandBuffer, std::span<const NativeImage>)>& record
);
}  // namespace openrac::vkgl
