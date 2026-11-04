// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Water lighting, poses and authored UVs use explicit storage access and descriptor-matched sampled inputs.
#include "enhanced/renderer/water_graph.h"

#include <exception>
#include <string>
#include <utility>

#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph_execution.h"
#endif

namespace openrac::enhanced {
#if defined(OPENRAC_VULKAN)
namespace {
namespace graph = renderer::graph;
// Depth, shadow, colour and compute passes all consume the same completed water data.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Builds explicit storage accesses before capturing the bound kernel's sampler and uniform inputs.
Result<graph::Pass> water_pass(
    vkgl::GraphExecution& execution, std::string_view name, std::span<const WaterBufferUse> buffers
) {
    graph::Pass pass{std::string(name)};

    // Read/write kernels preserve metadata or unmodified components of their imported buffer.
    for (const auto& use : buffers) {
        const auto contents = use.access == graph::Access::Write ? vkgl::InitialContents::Discard
                                                                 : vkgl::InitialContents::Preserve;
        const auto buffer = execution.import_buffer(std::string(name), use.buffer, contents);

        // Missing or transient writable storage cannot satisfy the explicit kernel contract.
        if (!buffer) {
            return unexpected(buffer.error());
        }

        pass.buffers.push_back(
            {*buffer,
             {0, use.bytes},
             {graph::Stage::ComputeShader, use.access, graph::Layout::Buffer}}
        );
    }

    const auto captured = execution.capture_compute_reads(pass);

    // Invalid sampler or uniform imports must fail before dispatch.
    if (!captured) {
        return unexpected(captured.error());
    }

    return pass;
}
}  // namespace
#endif

Result<void> water_compute_pass(
    std::string_view name,
    std::span<const WaterBufferUse> buffers,
    const std::function<void()>& record
) {
    // An absent callback cannot produce the declared water data.
    if (!record) {
        return unexpected(Error{"water compute pass requires a callback"});
    }
#if defined(OPENRAC_VULKAN)
    // Vulkan declares the water kernel dependencies before recording.
    if (gl::is_vulkan()) {
        vkgl::GraphExecution execution;
        auto pass = water_pass(execution, name, buffers);

        // Do not publish partially declared water outputs after an input failure.
        if (!pass) {
            return unexpected(pass.error());
        }

        auto consumers =
            graph::shader_read_export(std::string(name) + " consumers", *pass, kShaders);
        execution.add_pass(std::move(*pass));
        execution.add_pass(std::move(consumers));
        // All callbacks finish before the temporary declarations and borrowed bindings are released.
        return execution.run([&](unsigned) { record(); });
    }
#else
    (void)name;
    (void)buffers;
#endif
    // Reference kernels originally used one trailing storage/texel dependency.
    try {
        record();
        gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT | gl::GL_TEXTURE_FETCH_BARRIER_BIT);
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"water compute callback threw an unknown exception"});
    }

    return {};
}
}  // namespace openrac::enhanced
