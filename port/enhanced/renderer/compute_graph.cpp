// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enhanced compute graph execution, with OpenGL compatibility barriers.
#include "enhanced/renderer/compute_graph.h"

#include <utility>

#include "renderer/gl/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution.h"
#endif

namespace openrac::enhanced {
#if defined(OPENRAC_VULKAN)
namespace {
namespace graph = renderer::graph;
// Every shader stage may consume masks and cached lighting after these compute dispatches.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Declares whole overwritten outputs and captures inputs from the bound shader descriptors.
Result<graph::Pass> compute_pass(
    vkgl::GraphExecution& execution,
    std::string_view name,
    std::span<const ComputeOutput> outputs,
    ComputeInputs inputs
) {
    auto pass = execution.make_pass(name);

    // Output allocations stay stable through synchronous recording.
    for (const auto& output : outputs) {
        const auto handle = execution.import_buffer(
            name,
            output.buffer,
            vkgl::InitialContents::Discard,
            output.track_shader_reads ? vkgl::BufferVisibility::TrackShaderReads
                                      : vkgl::BufferVisibility::Conservative
        );

        // A transient or missing output cannot be written through a graph import.
        if (!handle) {
            return unexpected(handle.error());
        }

        pass.buffers.push_back(
            {*handle,
             {0, output.bytes},
             {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}
        );
    }

    // Combined shortlist kernels declare image samplers that their shortlist branch never reads.
    const auto capture = inputs == ComputeInputs::UniformBlocks ? vkgl::ComputeInputs::UniformBlocks
                                                                : vkgl::ComputeInputs::All;
    const auto captured = execution.capture_compute_reads(pass, capture);

    // Invalid descriptors must fail before the output dispatch is recorded.
    if (!captured) {
        return unexpected(captured.error());
    }

    return pass;
}

}  // namespace
#endif

bool run_compute_graph(
    std::string_view name,
    std::span<const ComputeOutput> outputs,
    const std::function<void()>& record,
    std::string& error,
    ComputeInputs inputs,
    SampledInputExports sampled_exports
) {
    using namespace gl;
#if defined(OPENRAC_VULKAN)
    // Vulkan declares compute dependencies independently of lighting quality switches.
    if (is_vulkan()) {
        vkgl::GraphExecution execution;
        auto pass = compute_pass(execution, name, outputs, inputs);

        // Reject the declaration before invoking the borrowed callback.
        if (!pass) {
            error = pass.error().message;
            return false;
        }

        auto consumers = execution.make_pass(name);
        consumers.name.append(" consumers");
        graph::shader_read_export(consumers, *pass, kShaders);
        if (sampled_exports == SampledInputExports::Continuation) {
            for (auto& image : consumers.images) {
                image.continuation = true;
            }
        }
        execution.add_pass(std::move(*pass));
        execution.add_pass(std::move(consumers));
        // The executor calls this on the render thread before returning.
        const auto result = execution.run([&](unsigned) { record(); });

        // Compilation and recording errors leave the caller on its existing lighting fallback.
        if (!result) {
            error = result.error().message;
            return false;
        }

        return true;
    }
#else
    // Compatibility builds need no native bindings or graph allocations.
    (void)name;
    (void)outputs;
    (void)error;
    (void)inputs;
    (void)sampled_exports;
#endif
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    record();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    return true;
}
}  // namespace openrac::enhanced
