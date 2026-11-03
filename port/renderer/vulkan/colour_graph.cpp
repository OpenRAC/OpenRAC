// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements renderer/colour_graph_backend.h: preserved world attachments and shared graphics read
// dependencies.
#include "renderer/colour_graph.h"

#include <string>
#include <utility>

#include "renderer/colour_graph_backend.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/graph_execution.h"

namespace openrac::renderer::backend {
namespace {
// Joins stable graphics reads to their producers before any deferred colour commands execute.
Result<void> colour_inputs(
    vkgl::GraphExecution& execution, graph::Pass& pass, ColourInputs inputs
) {
    constexpr auto kShaders = graph::Stage::VertexShader | graph::Stage::FragmentShader;
    // Buffer imports retain the native descriptor's actual allocation range.
    for (unsigned buffer : inputs.buffers) {
        const auto read = execution.read_buffer(pass.name + " input", buffer, kShaders);
        // Missing storage must reject preparation rather than omit a dependency.
        if (!read) {
            return unexpected(read.error());
        }
        pass.buffers.push_back(*read);
    }
    // Refraction reads must follow their complete colour/depth snapshot producers.
    for (unsigned texture : inputs.textures) {
        const auto image = execution.read_image(pass.name + " sampled input", texture, kShaders);
        // Invalid snapshots cannot be sampled by a later draw callback.
        if (!image) {
            return unexpected(image.error());
        }
        pass.images.push_back(*image);
    }
    return {};
}
}  // namespace

// Validates preserved attachment contents before invoking the unchanged world draw callback.
Result<void> record_colour(
    std::string_view name,
    const RenderTarget& target,
    const std::function<void()>& record,
    ColourAttachments attachments,
    ColourInputs inputs
) {
    namespace graph = renderer::graph;
    vkgl::GraphExecution execution;
    const auto colour = execution.import_attachment(
        std::string(name) + " colour", target.framebuffer, graph::Aspect::Colour
    );
    // Partial geometry and blending require the incoming colour image.
    if (!colour) {
        return unexpected(colour.error());
    }
    graph::Pass pass{
        std::string(name),
        {{*colour,
          {},
          {graph::Stage::ColourOutput, graph::Access::ReadWrite, graph::Layout::ColourAttachment},
          graph::Load::Preserve}}
    };
    // Depth-free sky must not require initialized depth or advertise a depth writer.
    if (attachments == ColourAttachments::ColourDepth) {
        const auto depth = execution.import_attachment(
            std::string(name) + " depth", target.framebuffer, graph::Aspect::Depth
        );
        // World colour retains the depth established by its prepass.
        if (!depth) {
            return unexpected(depth.error());
        }
        pass.images.push_back(
            {*depth,
             {graph::Aspect::Depth},
             {graph::Stage::DepthTests, graph::Access::ReadWrite, graph::Layout::DepthAttachment},
             graph::Load::Preserve}
        );
    }
    const auto declared = colour_inputs(execution, pass, inputs);
    // A rejected input prevents the enclosing plan from issuing any deferred commands.
    if (!declared) {
        return unexpected(declared.error());
    }

    execution.add_pass(std::move(pass));
    // Full bucket recorders preserve state inherited between ordered draws, including their outgoing state.
    if (inputs.recording == ColourRecording::Ordered) {
        return execution.run_ordered([record](unsigned) { record(); });
    }
    // Low-level draws retain the bindings prepared specifically for this callback.
    return execution.run_prepared([record](unsigned) { record(); });
}

Result<void> prepare_colour_reads(std::string_view name) {
    vkgl::GraphExecution execution;
    graph::Pass reads{std::string(name)};
    const auto captured = execution.capture_graphics_reads(reads);

    // A malformed shared descriptor cannot be used by later colour draws.
    if (!captured) {
        return unexpected(captured.error());
    }

    execution.add_pass(std::move(reads));
    // The work here is visibility preparation; geometry follows with the same bound inputs.
    return execution.run_prepared([](unsigned) {});
}
}  // namespace openrac::renderer::backend
