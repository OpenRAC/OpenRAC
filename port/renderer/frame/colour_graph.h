// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Main colour attachment phases and shared graphics inputs, with OpenGL compatibility recording.
#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "common/result.h"

namespace openrac::renderer {
struct RenderTarget;
// Sky preserves colour without depth testing; world geometry also reads and writes depth.
enum class ColourAttachments {
    Colour,
    ColourDepth,
    ColourDepthStencil
};

// Prepared low-level draws restore bound state; ordered bucket callbacks inherit prior bucket state.
enum class ColourRecording {
    Prepared,
    Ordered
};

// Borrowed read-only storage and sampled images, imported before a colour callback can execute.
struct ColourInputs {
    std::span<const unsigned> buffers;   // Stable allocation views through the enclosing scope.
    std::span<const unsigned> textures;  // Complete texture images, including refraction snapshots.
    ColourRecording recording = ColourRecording::Prepared;  // Callback state contract.
};

// Returns whether Vulkan colour-phase dependency declarations are enabled.
[[nodiscard]] bool colour_graph_enabled();
// Records preserved colour and optional depth around a sky/world phase on the render thread.
// Callbacks may run smaller preparation graphs; attachments and `inputs` keep stable allocations.
[[nodiscard]] Result<void> colour_pass(
    std::string_view name,
    const RenderTarget& target,
    const std::function<void()>& record,
    ColourAttachments attachments = ColourAttachments::ColourDepth,
    ColourInputs inputs = {}
);
// Orders currently bound read-only graphics descriptors before subsequent draws in this phase.
// Call after shared lighting/deformation bindings; immutable per-material uploads remain
// backend-owned.
[[nodiscard]] Result<void> graphics_shader_reads(std::string_view name);
enum class SnapshotAttachments { Colour, Depth, ColourDepth };

// Snapshots the requested attachments into a separate single-sample framebuffer before refraction.
// `samples` is the source sample count; callbacks receive false for colour, true for depth.
// Allocations must remain stable through synchronous recording, including both native resolves.
[[nodiscard]] Result<void> colour_snapshot(
    const RenderTarget& source,
    unsigned destination,
    int samples,
    const std::function<void(bool)>& record,
    SnapshotAttachments attachments = SnapshotAttachments::ColourDepth
);
}  // namespace openrac::renderer
