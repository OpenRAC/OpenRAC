// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shadow-phase graph contracts. Callbacks retain existing draws and clears on the render thread.
#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include "common/result.h"

namespace openrac::scene {
// Clear callbacks initialise the whole depth image; preserved callbacks may update only dirty rectangles.
// PreserveSeparate is Preserve in a render pass of its own, so that the pass covers only the
// rectangles this callback clears and draws.
enum class ShadowDepthContents : std::uint8_t {
    Clear,
    Preserve,
    PreserveSeparate
};
// The static cache is copied next; completed shadow maps are sampled by lighting and composition.
enum class ShadowDepthConsumer : std::uint8_t {
    Shader,
    Copy
};

// Records one shadow depth phase and exports its completed contents for the next consumer.
// `record` owns the actual full clear or partial preservation promised by `contents`.
// Captures must live through an enclosing shared chain; imported allocations cannot change.
[[nodiscard]] Result<void> shadow_depth_pass(
    std::string_view name,
    unsigned framebuffer,
    ShadowDepthContents contents,
    const std::function<void()>& record,
    ShadowDepthConsumer consumer = ShadowDepthConsumer::Shader
);
// Records the full-depth cache copy and returns its destination to depth-attachment use.
// `record` must copy complete matching depth images; its captures survive any enclosing chain.
[[nodiscard]] Result<void> shadow_depth_copy(
    unsigned source, unsigned destination, const std::function<void()>& record
);
// Finishes atlas bookkeeping after shadow production, without writing its sampled depth.
// The callback may discard unused colour/stencil, but must preserve every lighting input.
[[nodiscard]] Result<void> shadow_depth_ready(
    unsigned framebuffer, const std::function<void()>& record
);
}  // namespace openrac::scene
