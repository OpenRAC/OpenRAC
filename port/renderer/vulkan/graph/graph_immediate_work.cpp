// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures compatibility draws, dispatches and buffer uploads as graph work.
#include "renderer/vulkan/graph/graph_immediate_work.h"

#include "common/profiler.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/graph_pipeline.h"

namespace openrac::vkgl::detail {
namespace graph = renderer::graph;

namespace {
template <class T>
T checked(Result<T> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return std::move(*result);
}

void checked(Result<void> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

// Draws and clears share attachment preservation and discard semantics.
void GraphImmediateWork::capture_attachment(
    GraphExecution& execution, graph::Pass& pass, Image* image, graph::Aspect aspect
) {
    if (!image) {
        return;
    }
    auto& label = execution.m_recording->attachment_label;
    label.assign(pass.name).append(" attachment");
    const auto handle = checked(execution.import_attachment(
        label,
        g->state.draw_framebuffer,
        aspect == graph::Aspect::Colour ? graph::Aspect::Colour : graph::Aspect::Depth
    ));
    const bool defined = image->layout != VK_IMAGE_LAYOUT_UNDEFINED
                         && !(image->discarded & static_cast<VkImageAspectFlags>(aspect));
    pass.images.push_back(
        {handle,
         {aspect},
         {aspect == graph::Aspect::Colour ? graph::Stage::ColourOutput : graph::Stage::DepthTests,
          defined ? graph::Access::ReadWrite : graph::Access::Write,
          aspect == graph::Aspect::Colour ? graph::Layout::ColourAttachment
                                          : graph::Layout::DepthAttachment},
         defined ? graph::Load::Preserve : graph::Load::Discard}
    );
}

void GraphImmediateWork::draw(
    const std::function<void()>& record, GpuPage* page, unsigned indirect
) {
    const profiling::Zone timing("Immediate graph raster");
    const auto* program = g->programs.find(g->state.program);
    if (!program || !program->linked) {
        return;
    }
    prepare_frame();
    const auto target = target_of(g->state.draw_framebuffer);
    if ((!target.colour && !target.depth) || target.width <= 0 || target.height <= 0
        || g->state.viewport[2] <= 0 || g->state.viewport[3] <= 0) {
        return;
    }
    // Flush an earlier packet before establishing this pass's resource entry states.
    gpu_flush_draws();
    GraphExecution execution;
    execution.m_recording->attachment_fallback = true;
    graph::Pass pass{"immediate raster"};
    pass.execution = graph::Stage::VertexShader | graph::Stage::FragmentShader;
    checked(execution.capture_graphics_reads(pass));
    capture_attachment(execution, pass, target.colour, graph::Aspect::Colour);
    capture_attachment(execution, pass, target.depth, graph::Aspect::Depth);
    if (target.depth && has_stencil(target.depth->format) && g->state.stencil_test) {
        capture_attachment(execution, pass, target.depth, graph::Aspect::Stencil);
    }
    auto& recording = *execution.m_recording;
    const auto input = [&](std::string_view name, BufferRef region, graph::Stage stage) {
        const auto handle = checked(recording.import_buffer(name, region, true));
        pass.buffers.push_back(
            {handle,
             {region.offset, region.size},
             {stage, graph::Access::Read, graph::Layout::Buffer}}
        );
    };
    const auto& array = vertex_array();
    for (const auto& attribute : program->compiled.attributes) {
        if (attribute.location < 0 || attribute.location >= kAttributes) {
            continue;
        }
        const auto& bound = array.attributes[attribute.location];
        auto* buffer = bound.enabled ? g->buffers.find(bound.buffer) : nullptr;
        if (!buffer || !buffer->size || bound.format == VK_FORMAT_UNDEFINED) {
            continue;
        }
        input(
            "raster vertices",
            page && bound.divisor ? BufferRef{page->output.buffer, 0, page->output.size}
                                  : resolve(*buffer, bound.divisor == 0 || buffer->storage),
            graph::Stage::VertexInput
        );
    }
    if (auto* elements = g->buffers.find(array.elements); elements && elements->size) {
        input("raster indices", resolve(*elements), graph::Stage::VertexInput);
    }
    if (page) {
        input(
            "raster indirect arguments",
            {page->arguments.buffer, 0, page->arguments.size},
            graph::Stage::DrawIndirect
        );
    } else if (auto* arguments = g->buffers.find(indirect); arguments && arguments->size) {
        input("raster indirect arguments", resolve(*arguments), graph::Stage::DrawIndirect);
    }
    execution.add_pass(std::move(pass));
    checked(execution.run([&](unsigned) { record(); }));
}

void GraphImmediateWork::dispatch(const std::function<void()>& record) {
    const profiling::Zone timing("Immediate graph compute");
    const auto* program = g->programs.find(g->state.program);
    if (!program || !program->linked || !program->compute_pipeline) {
        return;
    }
    prepare_frame();
    end_rendering(PassEnd::Compute);
    GraphExecution execution;
    graph::Pass pass{"immediate compute"};
    pass.execution = graph::Stage::ComputeShader;
    checked(execution.capture_compute_reads(pass));
    for (const auto& block : program->compiled.storage) {
        if (block.binding < 0 || block.binding >= static_cast<int>(kUniformBindings)) {
            continue;
        }
        const auto name = g->state.storage_bindings[block.binding];
        const auto* buffer = g->buffers.find(name);
        if (!buffer || !buffer->size) {
            continue;
        }
        const auto handle = checked(execution.import_buffer("compute storage", name));
        // Compatibility compute reflection does not expose read/write qualifiers.
        // One combined access preserves the entire buffer conservatively.
        std::erase_if(pass.buffers, [&](const graph::BufferUse& use) {
            return use.buffer.index == handle.index;
        });
        pass.buffers.push_back(
            {handle,
             {0, buffer->size},
             {graph::Stage::ComputeShader, graph::Access::ReadWrite, graph::Layout::Buffer}}
        );
    }
    execution.add_pass(std::move(pass));
    checked(execution.run([&](unsigned) { record(); }));
}

void GraphImmediateWork::copy_buffer(
    VkCommandBuffer command,
    BufferRef source,
    BufferRef destination,
    VkBufferCopy region,
    bool export_read
) {
    if (graph_pipeline_records(command)) {
        validate_graph_buffer_access(
            source.buffer,
            {region.srcOffset, region.size},
            {graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer}
        );
        validate_graph_buffer_access(
            destination.buffer,
            {region.dstOffset, region.size},
            {graph::Stage::Transfer, graph::Access::Write, graph::Layout::Buffer}
        );
        NativeCommandEncoder{command}.copy_buffer(source.buffer, destination.buffer, 1, &region);
        count_command(Command::Copy);
        return;
    }
    const profiling::Zone timing("Immediate graph buffer transfer");
    GraphExecution execution;
    auto& recording = *execution.m_recording;
    recording.target = command;
    const auto from = checked(recording.import_buffer("copy source", source, true));
    const auto to = checked(recording.import_buffer(
        "copy destination", destination, source.buffer == destination.buffer
    ));
    graph::CommandBuffer commands;
    commands.copy_buffer(from, {region.srcOffset, region.size}, to, region.dstOffset);
    auto pass = commands.declarations("buffer transfer");
    checked(execution.add_pass(std::move(pass), std::move(commands)));
    if (export_read) {
        execution.add_pass(
            {"consume immutable buffer upload",
             {},
             {{to,
               {region.dstOffset, region.size},
               {graph::Stage::All, graph::Access::Read, graph::Layout::Buffer}}},
             graph::PassKind::Export}
        );
    }
    checked(execution.run());
}
}  // namespace openrac::vkgl::detail
