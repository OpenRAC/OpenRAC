// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The active pass contract and its native pipeline recording boundary.
#pragma once

#include "renderer/graph/command_buffer.h"
#include "renderer/graph/recording_scope.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
struct GraphBinding;

[[nodiscard]] bool graph_pipeline_active();
[[nodiscard]] bool graph_pipeline_records(VkCommandBuffer command);

// Exposes a dispatch's declarations during descriptor preparation without authorizing GPU work.
class ComputeBindingScope {
public:
    ComputeBindingScope(const renderer::graph::Pass& pass, std::span<const GraphBinding> bindings);
    ~ComputeBindingScope();
    ComputeBindingScope(const ComputeBindingScope&) = delete;
    ComputeBindingScope& operator=(const ComputeBindingScope&) = delete;

    const renderer::graph::Pass& pass;
    const std::span<const GraphBinding> bindings;

private:
    const ComputeBindingScope* m_previous;
};

[[nodiscard]] bool compute_metadata_only();
void validate_compute_sampler(const Sampled& sampled);
void validate_compute_outputs(std::span<const BufferRef> storage);
[[nodiscard]] BufferRef compute_storage_binding(const ProgramBlock& block, BufferRef buffer);
void validate_graph_image_access(
    const Image& image,
    renderer::graph::ImageRange range,
    renderer::graph::State state,
    bool partial_write
);
void validate_graph_buffer_access(
    VkBuffer buffer, renderer::graph::BufferRange range, renderer::graph::State state
);

// Valid only while the executor is recording one compiled pass on this thread.
class GraphPipelineScope {
public:
    GraphPipelineScope(
        const renderer::graph::Pass& pass,
        std::span<const GraphBinding> bindings,
        const renderer::graph::PlannedPass& scheduled,
        VkCommandBuffer target = VK_NULL_HANDLE
    );
    ~GraphPipelineScope();
    GraphPipelineScope(const GraphPipelineScope&) = delete;
    GraphPipelineScope& operator=(const GraphPipelineScope&) = delete;

    const renderer::graph::Pass& pass;
    const std::span<const GraphBinding> bindings;
    const VkCommandBuffer target;  // Null follows the main stream, including deferred secondaries.

private:
    renderer::graph::RecordingScope m_commands;
    const GraphPipelineScope* m_previous;
};

// The native target and argument buffer have already been bound by the backend.
void record_pipeline_command(
    VkCommandBuffer buffer,
    const renderer::graph::PipelineCommand& command,
    VkBuffer indirect = VK_NULL_HANDLE,
    std::optional<renderer::graph::BufferRange> generated_arguments = std::nullopt
);
void validate_pipeline_draw(const Target& target);
// Checks only the aspects actually cleared after GL masks are applied.
void validate_pipeline_clear(const Target& target, VkImageAspectFlags aspects);
}  // namespace openrac::vkgl::detail
