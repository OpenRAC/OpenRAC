// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Vulkan execution scopes and image transitions shared by frame-graph consumers.
#include <array>
#include <stdexcept>
#include <utility>

#include "renderer/vulkan/graph_barriers.h"

namespace openrac::vkgl::detail {
namespace graph = renderer::graph;

namespace {
// External shader-read layouts can be consumed by any shader stage.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Buffer access flags vary by consuming stage, independently of image layouts.
struct AccessScope {
    VkAccessFlags read = 0;   // Read scopes supported by the participating stages.
    VkAccessFlags write = 0;  // Write scopes supported by the participating stages.
};

// Tests whether a combined execution scope contains any requested stage.
bool has_stage(graph::Stage stages, graph::Stage mask) {
    return (static_cast<unsigned>(stages) & static_cast<unsigned>(mask)) != 0;
}

// Translates storage, transfer and GPU-driven buffer consumers without invalid cross-stage access flags.
AccessScope buffer_scope(graph::Stage stages) {
    AccessScope scope;
    // Shader storage and texel views share shader memory access flags.
    if (has_stage(stages, kShaders)) {
        scope.read |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
        scope.write |= VK_ACCESS_SHADER_WRITE_BIT;
    }
    // Transfer stages use copy access, even though buffers have no image layout transitions.
    if (has_stage(stages, graph::Stage::Transfer)) {
        scope.read |= VK_ACCESS_TRANSFER_READ_BIT;
        scope.write |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    // Vertex and index fetches are read-only consumers of GPU-produced buffers.
    if (has_stage(stages, graph::Stage::VertexInput)) {
        scope.read |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
    }
    // Indirect arguments are read outside the shader stages.
    if (has_stage(stages, graph::Stage::DrawIndirect)) {
        scope.read |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    return scope;
}

// Resolves depth tests, depth resolves and read-only shader consumers into valid Vulkan access flags.
AccessScope depth_scope(graph::State state) {
    AccessScope scope;
    // Depth tests and shader sampling can share a read-only layout but have distinct access flags.
    if (has_stage(state.stages, graph::Stage::DepthTests)) {
        scope.read |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        scope.write |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    // Resolve writes use the legacy colour-output scope alongside depth tests.
    if ((static_cast<unsigned>(state.stages) & static_cast<unsigned>(graph::Stage::ColourOutput))
        != 0) {
        scope.write |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // A read-only depth import can include shader sampling alongside depth tests.
    if (state.layout == graph::Layout::DepthReadOnly && has_stage(state.stages, kShaders)) {
        scope.read |= VK_ACCESS_SHADER_READ_BIT;
    }
    return scope;
}
}  // namespace

// The legacy tracker provides layout, so imports conservatively retain its possible prior accesses.
Result<graph::State> imported_state(VkImageLayout layout) {
    // Legacy images retain one layout; use its full possible memory scope at the graph boundary.
    switch (layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return graph::State{};
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return graph::State{kShaders, graph::Access::Read, graph::Layout::Sampled};
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return graph::State{
                graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource
            };
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return graph::State{
                graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
            };
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return graph::State{
                graph::Stage::ColourOutput,
                graph::Access::ReadWrite,
                graph::Layout::ColourAttachment
            };
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Depth resolves also run in the colour-output stage.
            return graph::State{
                graph::Stage::DepthTests | graph::Stage::ColourOutput,
                graph::Access::ReadWrite,
                graph::Layout::DepthAttachment
            };
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return graph::State{
                kShaders | graph::Stage::DepthTests,
                graph::Access::Read,
                graph::Layout::DepthReadOnly
            };
        case VK_IMAGE_LAYOUT_GENERAL:
            return graph::State{
                graph::Stage::All, graph::Access::ReadWrite, graph::Layout::General
            };
        default:
            return unexpected(Error{"unsupported graph import image layout"});
    }
}

// Maps the planner's shader, attachment and transfer stages to the existing Vulkan queue.
VkPipelineStageFlags stages_of(graph::Stage stages) {
    // External GENERAL imports can have accesses outside the first migrated compute pass.
    if (stages == graph::Stage::All) {
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    constexpr std::array<std::pair<graph::Stage, VkPipelineStageFlags>, 8> kStages{
        {{graph::Stage::Transfer, VK_PIPELINE_STAGE_TRANSFER_BIT},
         {graph::Stage::VertexInput, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
         {graph::Stage::VertexShader, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
         {graph::Stage::FragmentShader, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
         {graph::Stage::ComputeShader, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
         {graph::Stage::ColourOutput, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
         {graph::Stage::DepthTests,
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
         {graph::Stage::DrawIndirect, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT}}
    };
    VkPipelineStageFlags result = 0;

    // Combined scopes retain every participating stage from earlier readers.
    for (const auto& [stage, flags] : kStages) {
        // Ignore stages absent from this transition.
        if ((static_cast<unsigned>(stages) & static_cast<unsigned>(stage)) != 0) {
            result |= flags;
        }
    }

    return result ? result : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

// The graph distinguishes reads/writes even when an image remains in GENERAL.
VkAccessFlags access_of(graph::State state) {
    // Presentation is an external consumer, not a queue memory access.
    if (state.layout == graph::Layout::Present) {
        return 0;
    }
    VkAccessFlags read = VK_ACCESS_SHADER_READ_BIT;
    VkAccessFlags write = VK_ACCESS_SHADER_WRITE_BIT;

    // Match imported legacy access scopes while keeping graph compute barriers specific.
    if (state.stages == graph::Stage::All) {
        read = VK_ACCESS_MEMORY_READ_BIT;
        write = VK_ACCESS_MEMORY_WRITE_BIT;
    } else if (state.layout == graph::Layout::Buffer) {
        const auto scope = buffer_scope(state.stages);
        read = scope.read;
        write = scope.write;
    } else if (state.layout == graph::Layout::TransferSource
               || state.layout == graph::Layout::TransferDestination) {
        read = VK_ACCESS_TRANSFER_READ_BIT;
        write = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (state.layout == graph::Layout::ColourAttachment) {
        read = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        write = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (state.layout == graph::Layout::DepthAttachment
               || state.layout == graph::Layout::DepthReadOnly) {
        const auto scope = depth_scope(state);
        read = scope.read;
        write = scope.write;
    }

    const bool reads =
        state.access == graph::Access::Read || state.access == graph::Access::ReadWrite;
    const bool writes =
        state.access == graph::Access::Write || state.access == graph::Access::ReadWrite;
    return (reads ? read : 0u) | (writes ? write : 0u);
}

// Every layout the shadow imports or exports has a direct Vulkan equivalent.
VkImageLayout layout_of(graph::Layout layout) {
    // Unsupported image layouts are programming errors in the private executor.
    switch (layout) {
        case graph::Layout::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case graph::Layout::General:
            return VK_IMAGE_LAYOUT_GENERAL;
        case graph::Layout::Sampled:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case graph::Layout::ColourAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case graph::Layout::DepthAttachment:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case graph::Layout::DepthReadOnly:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case graph::Layout::TransferSource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case graph::Layout::TransferDestination:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case graph::Layout::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case graph::Layout::Buffer:
            break;
    }

    throw std::runtime_error("buffer layout in image graph");
}

void record_graph_image_barrier(
    VkCommandBuffer command,
    const graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source
) {
    const auto& range = std::get<graph::ImageRange>(transition.range);
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = access_of(transition.before);
    barrier.dstAccessMask = access_of(transition.after);
    barrier.oldLayout = layout_of(transition.before.layout);
    barrier.newLayout = layout_of(transition.after.layout);
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    // Whole-image imports share the legacy layout, including stencil beside sampled depth.
    barrier.subresourceRange =
        {image.aspect, range.first_mip, range.mip_count, range.first_layer, range.layer_count};
    const VkPipelineStageFlags source =
        external_source != 0 ? external_source : stages_of(transition.before.stages);
    // The queue has no stage after handing an acquired image back to presentation.
    const VkPipelineStageFlags destination = transition.after.layout == graph::Layout::Present
                                                 ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
                                                 : stages_of(transition.after.stages);
    vkCmdPipelineBarrier(command, source, destination, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    count_barrier(source, destination);
    image.layout = barrier.newLayout;
}

VkBufferMemoryBarrier graph_buffer_barrier(
    const graph::Barrier& transition, const BufferRef& buffer
) {
    const auto& range = std::get<graph::BufferRange>(transition.range);
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask = access_of(transition.before);
    barrier.dstAccessMask = access_of(transition.after);
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer.buffer;
    barrier.offset = buffer.offset + range.offset;
    barrier.size = range.size;
    return barrier;
}

void record_graph_buffer_barrier(
    VkCommandBuffer command, const graph::Barrier& transition, const BufferRef& buffer
) {
    const auto barrier = graph_buffer_barrier(transition, buffer);
    const VkPipelineStageFlags source = stages_of(transition.before.stages);
    const VkPipelineStageFlags destination = stages_of(transition.after.stages);
    vkCmdPipelineBarrier(command, source, destination, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    count_barrier(source, destination);
}

graph::State attachment_copy_state(bool depth_resolve, graph::Access access) {
    // Dynamic-rendering depth/stencil resolves execute in attachment stages.
    if (depth_resolve) {
        return {
            graph::Stage::DepthTests | graph::Stage::ColourOutput,
            access,
            graph::Layout::DepthAttachment
        };
    }
    // Colour resolves and same-sample image copies use native transfer commands.
    return {
        graph::Stage::Transfer,
        access,
        access == graph::Access::Read ? graph::Layout::TransferSource
                                      : graph::Layout::TransferDestination
    };
}

}  // namespace openrac::vkgl::detail
