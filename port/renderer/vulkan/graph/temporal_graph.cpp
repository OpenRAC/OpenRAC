// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements temporal_graph.h raster and blit phases on the existing recording, with synchronous SDK boundaries.
#include "renderer/vulkan/graph/temporal_graph.h"

#include <exception>
#include <string>
#include <utility>

#include "renderer/frame/surface_chain.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/surface_batch.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace {
namespace graph = renderer::graph;
// Temporal intermediates can feed pixel shaders and native upscaler kernels.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Exports only the written attachment, retaining depth/stencil's common physical layout tracking.
graph::Pass output_consumer(
    std::string name, graph::ImageHandle image, graph::Aspect aspect, TemporalConsumer consumer
) {
    const bool depth = aspect == graph::Aspect::Depth;
    graph::State state{kShaders, graph::Access::Read, graph::Layout::Sampled};

    // Display outputs must remain attachment-ready for later world overlays and native HUD draws.
    if (consumer == TemporalConsumer::Attachment) {
        state =
            {depth ? graph::Stage::DepthTests : graph::Stage::ColourOutput,
             graph::Access::Read,
             depth ? graph::Layout::DepthAttachment : graph::Layout::ColourAttachment};
    }

    return {std::move(name), {{image, {aspect}, state}}, {}, graph::PassKind::Export};
}

// Declares sampled input images before any raster work can overwrite its destination.
Result<void> sampled_inputs(
    GraphExecution& execution, graph::Pass& pass, std::span<const unsigned> inputs
) {
    // These phases have a small, explicit shared-image interface rather than per-material
    // descriptors.
    for (unsigned texture : inputs) {
        const auto input = execution.import_image("temporal sampled input", texture);

        // Invalid sampled inputs must fail before a destination can be overwritten.
        if (!input) {
            return unexpected(input.error());
        }

        // Depth texture names still denote depth aspects even when their sampled values are scalar
        // floats.
        const auto& image = detail::g->textures.find(texture)->image;
        const auto aspect = image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth
                                                                      : graph::Aspect::Colour;
        pass.images.push_back(
            {*input, {aspect}, {kShaders, graph::Access::Read, graph::Layout::Sampled}}
        );
    }

    return {};
}

// Declares one complete or preserved output attachment without introducing a draw or changing its
// allocation.
Result<graph::Pass> raster_output(
    GraphExecution& execution, std::string_view name, const TemporalTarget& target
) {
    const bool preserve = target.contents == TemporalContents::Preserve;
    const auto output = execution.import_attachment(
        std::string(name),
        target.framebuffer,
        target.aspect,
        preserve ? InitialContents::Preserve : InitialContents::Discard
    );

    // A missing output attachment cannot receive the declared complete or preserved draw.
    if (!output) {
        return unexpected(output.error());
    }

    const bool depth = target.aspect == graph::Aspect::Depth;
    return graph::Pass{
        std::string(name),
        {{*output,
          {target.aspect},
          {depth ? graph::Stage::DepthTests : graph::Stage::ColourOutput,
           preserve ? graph::Access::ReadWrite : graph::Access::Write,
           depth ? graph::Layout::DepthAttachment : graph::Layout::ColourAttachment},
          preserve ? graph::Load::Preserve : graph::Load::Discard}}
    };
}

// Object motion borrows the world's depth only for testing, never as a simultaneous sampled image.
Result<void> depth_input(
    GraphExecution& execution, graph::Pass& pass, const TemporalTarget& target
) {
    // Fullscreen passes without depth tests must not import an unrelated or absent depth
    // attachment.
    if (!target.depth_test) {
        return {};
    }

    const auto tested = execution.import_attachment(
        "temporal depth test", target.framebuffer, graph::Aspect::Depth
    );

    // Object overlays cannot test undefined borrowed world depth.
    if (!tested) {
        return unexpected(tested.error());
    }

    pass.images.push_back(
        {*tested,
         {graph::Aspect::Depth},
         {graph::Stage::DepthTests, graph::Access::Read, graph::Layout::DepthAttachment},
         graph::Load::Preserve}
    );
    return {};
}

// Compiles all raster accesses before the synchronous draw can modify any output contents.
Result<void> graph_raster(
    std::string_view name,
    const TemporalTarget& target,
    std::span<const unsigned> inputs,
    const std::function<void()>& record
) {
    GraphExecution execution;
    auto pass = raster_output(execution, name, target);

    // Output validation finishes before any input capture or draw recording.
    if (!pass) {
        return unexpected(pass.error());
    }

    const auto captured = sampled_inputs(execution, *pass, inputs);

    // Unwritten or missing sampled inputs cannot feed a completed temporal output.
    if (!captured) {
        return unexpected(captured.error());
    }

    const auto tested = depth_input(execution, *pass, target);

    // Object overlays cannot test undefined borrowed world depth.
    if (!tested) {
        return unexpected(tested.error());
    }

    const auto output = pass->images.front().image;
    execution.add_pass(std::move(*pass));
    execution.add_pass(
        output_consumer(std::string(name) + " consumers", output, target.aspect, target.consumer)
    );
    // All draws remain synchronous on the render thread, with no allocation changes during the
    // callback.
    return execution.run_raster([record](unsigned) { record(); });
}

// Models native colour resolves as transfers, and single-sample broadcasts into MSAA as fullscreen
// draws.
Result<void> graph_blit(
    std::string_view name,
    unsigned source,
    unsigned destination,
    TemporalConsumer consumer,
    const std::function<void()>& record
) {
    GraphExecution execution;
    const auto from =
        execution.import_attachment("temporal copy source", source, graph::Aspect::Colour);
    const auto to = execution.import_attachment(
        "temporal copy destination", destination, graph::Aspect::Colour, InitialContents::Discard
    );

    // Both physical colour allocations must exist before the native copy callback.
    if (!from || !to) {
        return unexpected(!from ? from.error() : to.error());
    }

    const auto source_target = detail::target_of(source);
    const auto destination_target = detail::target_of(destination);
    const bool broadcast = source_target.colour->samples == VK_SAMPLE_COUNT_1_BIT
                           && destination_target.colour->samples != VK_SAMPLE_COUNT_1_BIT;
    graph::State read{graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource};
    graph::State write{
        graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
    };

    // Vulkan cannot transfer a single-sample image into MSAA, so the backend broadcasts using a
    // shader.
    if (broadcast) {
        read = {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Sampled};
        write = {graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment};
    }

    execution.add_pass({std::string(name), {{*from, {}, read}, {*to, {}, write}}});
    execution.add_pass(
        output_consumer(std::string(name) + " consumers", *to, graph::Aspect::Colour, consumer)
    );
    // Native blit commands retain their original coordinates, filtering and sample-conversion
    // behavior.
    return execution.run_prepared(record);
}
}  // namespace

Result<void> temporal_chain(const std::function<void()>& prepare) {
    // An empty scope cannot define a temporal section.
    if (!prepare) {
        return unexpected(Error{"temporal chain requires preparation"});
    }
    // Nested temporal work finishes before history state is committed.
    try {
        if (active() && !detail::collecting_surface_chain()) {
            return renderer::backend::run_surface_chain(prepare);
        }
        prepare();
        // An enclosing world chain must finish before its caller commits temporal history.
        return detail::flush_surface_chain();
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"temporal chain callback threw an unknown exception"});
    }
}

Result<void> temporal_raster(
    std::string_view name,
    const TemporalTarget& target,
    std::span<const unsigned> inputs,
    const std::function<void()>& record
) {
    // A missing callback cannot satisfy the declared temporal work on either path.
    if (!record) {
        return unexpected(Error{"temporal raster requires a callback"});
    }

    // Driver and callback failures stay inside this synchronous renderer interface.
    try {
        return graph_raster(name, target, inputs, record);
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"temporal raster callback threw an unknown exception"});
    }
}

Result<void> temporal_blit(
    std::string_view name,
    unsigned source,
    unsigned destination,
    TemporalConsumer consumer,
    const std::function<void()>& record
) {
    // A missing callback cannot satisfy the declared temporal work on either path.
    if (!record) {
        return unexpected(Error{"temporal blit requires a callback"});
    }

    // Callback failures remain inside the renderer interface.
    try {
        // Window-directed copies may be deferred and composed directly into the acquired image
        // later.
        if (destination != 0) {
            return graph_blit(name, source, destination, consumer, record);
        }

        record();
        return {};
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"temporal blit callback threw an unknown exception"});
    }
}
}  // namespace openrac::vkgl
