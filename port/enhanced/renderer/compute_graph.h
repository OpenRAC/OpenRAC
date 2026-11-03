// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronous enhanced compute passes with explicit outputs and reflected, descriptor-matched input reads.
#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace openrac::enhanced {
// A complete output range, overwritten by this dispatch before shaders consume it.
struct ComputeOutput {
    unsigned buffer = 0;      // Renderer-owned, stable storage-buffer name.
    std::uint64_t bytes = 0;  // Written bytes starting at offset zero.
};

// Shortlist branches consume metadata but do not sample the image bindings of a combined kernel.
enum class ComputeInputs : std::uint8_t {
    All,
    UniformBlocks
};

// Runs `record` synchronously on the render thread, with Vulkan graph dependencies or OpenGL compatibility barriers.
// Outputs and callback are borrowed for this call only; callbacks must not reallocate or submit imports.
// False reports validation/recording errors in `error`, before consumers may use the outputs.
[[nodiscard]] bool run_compute_graph(
    std::string_view name,
    std::span<const ComputeOutput> outputs,
    const std::function<void()>& record,
    std::string& error,
    ComputeInputs inputs = ComputeInputs::All
);
}  // namespace openrac::enhanced
