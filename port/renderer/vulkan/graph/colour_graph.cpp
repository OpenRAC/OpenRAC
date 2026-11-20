// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements renderer/frame/colour_graph_backend.h: preserved world attachments and shared graphics read
// dependencies.
#include "renderer/frame/colour_graph.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>

#include "renderer/frame/colour_graph_backend.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::renderer::backend {
namespace {
struct ColourLabel {
    std::string_view name;
    std::array<char, 96> storage;
    std::string overflow{};

    std::string_view with_suffix(std::string_view suffix) {
        if (suffix.size() > storage.size() || name.size() > storage.size() - suffix.size()) {
            overflow.assign(name);
            return overflow.append(suffix);
        }
        const auto end = std::ranges::copy(suffix, std::ranges::copy(name, storage.begin()).out).out;
        return {storage.data(), static_cast<std::size_t>(end - storage.begin())};
    }
};

// Joins stable graphics reads to their producers before any deferred colour commands execute.
Result<void> colour_inputs(
    vkgl::GraphExecution& execution, graph::Pass& pass, ColourInputs inputs
) {
    constexpr auto kShaders = graph::Stage::VertexShader | graph::Stage::FragmentShader;
    // Buffer imports retain the native descriptor's actual allocation range.
    for (unsigned buffer : inputs.buffers) {
        const auto read = execution.read_buffer("shader input", buffer, kShaders);
        // Missing storage must reject preparation rather than omit a dependency.
        if (!read) {
            return unexpected(read.error());
        }
        pass.buffers.push_back(*read);
    }
    // Geometry is resolved before the draw so the plan sees its native allocation range.
    for (unsigned buffer : inputs.geometry) {
        const auto read = execution.read_buffer("vertex input", buffer, graph::Stage::VertexInput);
        // Missing geometry cannot produce a valid raster command.
        if (!read) {
            return unexpected(read.error());
        }
        pass.buffers.push_back(*read);
    }
    // Refraction reads must follow their complete colour/depth snapshot producers.
    for (unsigned texture : inputs.textures) {
        const auto image = execution.read_image("sampled input", texture, kShaders);
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
    ColourLabel label{name, {}};
    const bool overwrite = attachments == ColourAttachments::ColourOverwrite;
    const auto colour = execution.import_attachment(
        label.with_suffix(" colour"),
        target.framebuffer,
        graph::Aspect::Colour,
        overwrite ? vkgl::InitialContents::Discard : vkgl::InitialContents::Preserve
    );
    // Partial geometry and blending require the incoming colour image.
    if (!colour) {
        return unexpected(colour.error());
    }
    auto pass = execution.make_pass(name);
    const bool has_depth = attachments != ColourAttachments::Colour && !overwrite;
    pass.images.reserve(1 + has_depth + inputs.textures.size());
    pass.buffers.reserve(inputs.buffers.size() + inputs.geometry.size());
    pass.images.push_back(
        {*colour,
         {},
         {graph::Stage::ColourOutput,
          overwrite ? graph::Access::Write : graph::Access::ReadWrite,
          graph::Layout::ColourAttachment},
         overwrite ? graph::Load::Discard : graph::Load::Preserve}
    );
    // Depth-free sky must not require initialized depth or advertise a depth writer.
    if (has_depth) {
        const auto depth = execution.import_attachment(
            label.with_suffix(" depth"), target.framebuffer, graph::Aspect::Depth
        );
        // World colour retains the depth established by its prepass.
        if (!depth) {
            return unexpected(depth.error());
        }
        auto access = graph::Access::ReadWrite;
        if (attachments == ColourAttachments::ColourDepthRead) {
            access = graph::Access::Read;
        } else if (attachments == ColourAttachments::ColourDepthWrite) {
            access = graph::Access::Write;
        }
        // An unconditional depth write can change only part of the attachment.
        pass.images.push_back(
            {*depth,
             {attachments == ColourAttachments::ColourDepthStencil
                  ? graph::Aspect::Depth | graph::Aspect::Stencil
                  : graph::Aspect::Depth},
             {graph::Stage::DepthTests, access, graph::Layout::DepthAttachment},
             access == graph::Access::Write ? graph::Load::None : graph::Load::Preserve,
             graph::Store::Preserve,
             access == graph::Access::Write}
        );
    }
    const auto declared = colour_inputs(execution, pass, inputs);
    // A rejected input prevents the enclosing plan from issuing any deferred commands.
    if (!declared) {
        return unexpected(declared.error());
    }
    // Prepared diagnostic shaders can borrow their already bound probe data.
    if (inputs.bound_shader_reads) {
        const auto captured = execution.capture_graphics_reads(pass);
        if (!captured) {
            return unexpected(captured.error());
        }
    }

    execution.add_pass(std::move(pass));
    // Synchronous recording can borrow the callback through run().
    if (!backend::collecting_surface_chain()) {
        return execution.run([&record](unsigned) { record(); });
    }
    // Full bucket recorders preserve state inherited between ordered draws, including their outgoing state.
    if (inputs.recording == ColourRecording::Ordered) {
        return execution.run_ordered(record);
    }
    // Low-level draws retain the bindings prepared specifically for this callback.
    return execution.run_prepared(record);
}

Result<void> prepare_colour_reads(std::string_view name) {
    vkgl::GraphExecution execution;
    auto reads = execution.make_pass(name);
    const auto captured = execution.capture_graphics_reads(reads);

    // A malformed shared descriptor cannot be used by later colour draws.
    if (!captured) {
        return unexpected(captured.error());
    }

    execution.add_pass(std::move(reads));
    // The work here is visibility preparation; geometry follows with the same bound inputs.
    return execution.run_prepared([](unsigned) {});
}

Result<void> prepare_raster_reads(std::string_view name, RasterInputs inputs) {
    vkgl::GraphExecution execution;
    auto pass = execution.make_pass(name);
    pass.kind = graph::PassKind::Export;
    pass.images.reserve(inputs.textures.size());
    pass.buffers.reserve(inputs.geometry.size());

    // Visibility preparation has selected the exact vertex allocations used by the draw.
    for (const auto& geometry : inputs.geometry) {
        const auto read = execution.read_buffer(
            "vertex input", geometry.name, graph::Stage::VertexInput, geometry.instanced
        );
        if (!read) {
            return unexpected(read.error());
        }
        pass.buffers.push_back(*read);
    }
    constexpr auto shaders = graph::Stage::VertexShader | graph::Stage::FragmentShader;
    for (const auto& texture : inputs.textures) {
        if (!texture.name) {
            continue;
        }
        const auto read = execution.read_image("sampled input", texture.name, shaders);
        if (!read) {
            return unexpected(read.error());
        }
        pass.images.push_back(*read);
    }

    execution.add_pass(std::move(pass));
    return execution.run();
}

Result<void> record_surface_prepass(
    unsigned framebuffer, bool normals, bool multisample, const std::function<void()>& record
) {
    namespace graph = renderer::graph;
    const auto* target_depth = vkgl::detail::target_of(framebuffer).depth;

    // The default depth image can exist while detached from the active framebuffer.
    if (!target_depth) {
        return unexpected(Error{"depth prepass requires an attached depth image"});
    }

    vkgl::GraphExecution execution;
    constexpr auto discard = vkgl::InitialContents::Discard;
    const auto depth =
        execution.import_attachment("prepass depth", framebuffer, graph::Aspect::Depth, discard);

    if (!depth) {
        return unexpected(depth.error());
    }

    auto pass = execution.make_pass(normals ? "depth/normal prepass" : "depth prepass");
    pass.images.push_back(
        {*depth,
         {static_cast<graph::Aspect>(target_depth->aspect)},
         {graph::Stage::DepthTests, graph::Access::Write, graph::Layout::DepthAttachment},
         graph::Load::Clear}
    );

    std::optional<graph::ImageHandle> normals_image;
    // Normal output has a separate colour attachment on the prepass framebuffer.
    if (normals) {
        const auto normal = execution.import_attachment(
            "prepass normals", framebuffer, graph::Aspect::Colour, discard
        );

        if (!normal) {
            return unexpected(normal.error());
        }

        pass.images.push_back(
            {*normal,
             {},
             {graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment},
             graph::Load::Clear}
        );
        normals_image = *normal;
    }

    execution.add_pass(std::move(pass));

    // Lighting samples single-sample normals. Multisample attachments are resolved first.
    if (normals_image && !multisample) {
        auto consumers = execution.make_pass("prepass consumers");
        consumers.images.push_back(
            {*normals_image,
             {},
             {graph::Stage::ComputeShader | graph::Stage::FragmentShader,
              graph::Access::Read,
              graph::Layout::Sampled}}
        );
        consumers.images.back().continuation = true;
        consumers.kind = graph::PassKind::Export;
        execution.add_pass(std::move(consumers));
    }

    return execution.run_prepared(record);
}
}  // namespace openrac::renderer::backend
