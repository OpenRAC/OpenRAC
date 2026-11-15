// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Context-owned CPU diagnostics shared by existing graph executors, never GPU allocation owners.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "renderer/graph/frame_context.h"

namespace openrac::vkgl::detail {
// Presentation-only startup frames do not satisfy a request for renderer work.
enum class CaptureDomain : std::uint8_t {
    Renderer,
    Presentation
};

// Graphics-owner-only capture state; context handoff already excludes simultaneous callers.
struct FrameCapture {
    std::unique_ptr<renderer::graph::FrameContext>
        frame;  // Present only while capture is requested.
    std::optional<Error>
        failure;              // First collection or compilation error, preserved for inspection.
    std::string description;  // Last complete diagnostic dump, with no retained GPU owners.
    std::uint64_t observed_commands = 0;  // Counts distinguish declared callbacks from opaque gaps.
    bool finished = false;  // A completed capture can be inspected even after a failed validation.
    bool log_once = false;  // Environment request waits for the first frame with declared work.
    bool has_passes = false;  // Empty startup frames must not consume the environment request.
};

// Returns the source-declaration retention policy without allocating when diagnostics are disabled.
renderer::graph::DeclarationRetention frame_capture_retention();
// Appends a pass at its actual execution point; the returned event closes a nested callback scope.
std::optional<unsigned> capture_graph_pass(
    const renderer::graph::Plan& plan,
    unsigned pass,
    bool entry,
    CaptureDomain domain = CaptureDomain::Renderer
);
// Marks callback completion and observes commands already covered by this declaration.
void complete_graph_pass(
    const renderer::graph::Plan& plan, unsigned pass, std::optional<unsigned> event
);
// Invalidates an interrupted recording rather than publishing a partial dependency dump.
void abort_frame_capture(std::string_view reason);
// Arms the optional environment request after the graphics context is initialized.
void configure_frame_capture();
// Finishes CPU diagnostics at presentation; never submits commands or waits for the GPU.
void finish_frame_capture();
}  // namespace openrac::vkgl::detail
