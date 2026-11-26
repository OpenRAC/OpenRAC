// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Declares framebuffer clears and buffer transfers.
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

// Clears preserve the aspects with existing contents.
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
