// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements raster_pass.h for OpenGL/ES and the Vulkan colour executor.
#include "renderer/frame/raster_pass.h"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "renderer/frame/colour_graph_backend.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"
#include "renderer/graph/recording_scope.h"
#include "renderer/renderer.h"

namespace openrac::renderer {
namespace {

// Disjoint namespaces for GL names; default-framebuffer attachments also have nonzero keys.
constexpr std::uint64_t kColour = std::uint64_t{1} << 32;
constexpr std::uint64_t kDepth = std::uint64_t{1} << 33;
constexpr std::uint64_t kBuffer = std::uint64_t{1} << 34;

// Imports the preserved window or offscreen attachment with its allocated aspects.
void append_attachment(
    graph::Graph& graph,
    graph::Pass& pass,
    std::uint64_t key,
    graph::Aspect aspect,
    graph::State state,
    graph::Load load = graph::Load::Preserve
) {
    const bool preserve = load != graph::Load::Discard;
    const auto attachment = graph.import_image(
        "raster attachment",
        {.key = {key},
         .aspects = aspect,
         .initial = preserve ? state : graph::State{},
         .defined = preserve}
    );
    if (!attachment) {
        throw std::runtime_error(attachment.error().message);
    }
    // Writing depth can cover only part of the existing attachment.
    const bool partial = aspect == graph::Aspect::Depth && state.access == graph::Access::Write;
    pass.images.push_back({*attachment, {aspect}, state, load, graph::Store::Preserve, partial});
}

// Imports the textures and geometry before issuing any part of the GL raster pass.
void append_inputs(graph::Graph& graph, graph::Pass& pass, RasterInputs inputs) {
    constexpr graph::State sampled{
        graph::Stage::VertexShader | graph::Stage::FragmentShader,
        graph::Access::Read,
        graph::Layout::Sampled
    };
    for (const RasterTexture& texture : inputs.textures) {
        if (!texture.name) {
            continue;
        }
        const auto handle = graph.import_image(
            "raster texture",
            {.key = {texture.name}, .mips = texture.mip_levels, .initial = sampled, .defined = true}
        );
        if (!handle) {
            throw std::runtime_error(handle.error().message);
        }
        pass.images.push_back({*handle, {graph::Aspect::Colour, 0, texture.mip_levels}, sampled});
    }
    constexpr graph::State vertex{
        graph::Stage::VertexInput, graph::Access::Read, graph::Layout::Buffer
    };
    for (const RasterBuffer& buffer : inputs.geometry) {
        const auto handle = graph.import_buffer(
            "raster geometry", {{kBuffer + buffer.name}, buffer.size, vertex, true}
        );
        if (!handle) {
            throw std::runtime_error(handle.error().message);
        }
        pass.buffers.push_back({*handle, {0, buffer.size}, vertex});
    }
}

// Records an ordered GL pass on allocations supplied by the caller.
void record_gl(
    std::string_view name,
    const RenderTarget& target,
    RasterInputs inputs,
    const std::function<void()>& record
) {
    graph::Graph graph;
    auto pass = graph.make_pass(name);
    const bool overwrite = inputs.attachments == ColourAttachments::ColourOverwrite;
    append_attachment(
        graph,
        pass,
        kColour + target.framebuffer,
        graph::Aspect::Colour,
        {graph::Stage::ColourOutput,
         overwrite ? graph::Access::Write : graph::Access::ReadWrite,
         graph::Layout::ColourAttachment},
        overwrite ? graph::Load::Discard : graph::Load::Preserve
    );
    if (inputs.attachments != ColourAttachments::Colour && !overwrite) {
        auto access = graph::Access::ReadWrite;
        if (inputs.attachments == ColourAttachments::ColourDepthRead) {
            access = graph::Access::Read;
        } else if (inputs.attachments == ColourAttachments::ColourDepthWrite) {
            access = graph::Access::Write;
        }
        const auto aspect = inputs.attachments == ColourAttachments::ColourDepthStencil
                                ? graph::Aspect::Depth | graph::Aspect::Stencil
                                : graph::Aspect::Depth;
        append_attachment(
            graph,
            pass,
            kDepth + target.framebuffer,
            aspect,
            {graph::Stage::DepthTests, access, graph::Layout::DepthAttachment},
            access == graph::Access::Write ? graph::Load::None : graph::Load::Preserve
        );
    }
    append_inputs(graph, pass, inputs);
    graph.add_pass(std::move(pass));
    const auto plan = graph.compile();
    if (!plan) {
        throw std::runtime_error(plan.error().message);
    }
    const graph::RecordingScope scope{plan->passes.front()};
    record();
}

// Resolves Vulkan's native views and buffers through the colour executor.
void record_vulkan(
    std::string_view name,
    const RenderTarget& target,
    RasterInputs inputs,
    const std::function<void()>& record
) {
    std::vector<unsigned> textures;
    for (const RasterTexture& texture : inputs.textures) {
        if (texture.name) {
            textures.push_back(texture.name);
        }
    }
    std::vector<unsigned> buffers;
    for (const RasterBuffer& buffer : inputs.geometry) {
        buffers.push_back(buffer.name);
    }
    // End deferred work before borrowing the callback's prepared state.
    const auto result = run_surface_setup([&] {
        const auto drawn = colour_pass(
            name,
            target,
            record,
            inputs.attachments,
            {{}, textures, ColourRecording::Prepared, buffers, inputs.bound_shader_reads}
        );
        if (!drawn) {
            throw std::runtime_error(drawn.error().message);
        }
    });
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

void raster_pass(
    std::string_view name,
    const RenderTarget& target,
    RasterInputs inputs,
    const std::function<void()>& record
) {
    if (gl::is_vulkan()) {
        record_vulkan(name, target, inputs, record);
        return;
    }
    record_gl(name, target, inputs, record);
}

Result<void> raster_reads(std::string_view name, RasterInputs inputs) {
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan()) {
        return backend::prepare_raster_reads(name, inputs);
    }
#else
    (void)name;
    (void)inputs;
#endif
    return {};
}

}  // namespace openrac::renderer
