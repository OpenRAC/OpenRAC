// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Applies graph barriers to frame-owned resources and updates renderer tracking and counters.
#include <algorithm>
#include <bit>

#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/graph/graph_barriers.h"

namespace openrac::vkgl::detail {
namespace graph = renderer::graph;

namespace {
constexpr std::array kAspects{graph::Aspect::Colour, graph::Aspect::Depth, graph::Aspect::Stencil};

std::size_t cell(const Image& image, graph::Aspect aspect, unsigned mip, unsigned layer) {
    const auto slot = std::countr_zero(static_cast<unsigned>(aspect));
    return (std::size_t(slot) * image.layers + layer) * image.levels + mip;
}

template <typename Function>
void visit_range(const graph::ImageRange& range, Function visit) {
    for (auto aspect : kAspects) {
        if (!(static_cast<unsigned>(range.aspects) & static_cast<unsigned>(aspect))) {
            continue;
        }
        for (unsigned layer = range.first_layer; layer < range.first_layer + range.layer_count;
             ++layer) {
            for (unsigned mip = range.first_mip; mip < range.first_mip + range.mip_count; ++mip) {
                visit(aspect, mip, layer);
            }
        }
    }
}

template <typename Function>
void update_range(Image& image, const graph::ImageRange& range, Function update) {
    if (image.subresource_contents.empty()) {
        auto updated = image.uniform_contents;
        update(updated);
        if (updated == image.uniform_contents) {
            return;
        }
        if (range == whole_image(image)) {
            image.uniform_contents = updated;
            image.layout = layout_of(updated.state.layout);
            return;
        }
        image.subresource_contents
            .resize(3 * std::size_t(image.levels) * image.layers, image.uniform_contents);
    }

    visit_range(range, [&](auto aspect, auto mip, auto layer) {
        update(image.subresource_contents[cell(image, aspect, mip, layer)]);
    });

    const auto first_aspect = static_cast<graph::Aspect>(image.aspect & (~image.aspect + 1));
    const auto first = image.subresource_contents[cell(image, first_aspect, 0, 0)];
    bool uniform = true;
    bool uniform_layout = true;
    visit_range(whole_image(image), [&](auto aspect, auto mip, auto layer) {
        const auto& contents = image.subresource_contents[cell(image, aspect, mip, layer)];
        uniform &= contents == first;
        uniform_layout &= contents.state.layout == first.state.layout;
    });
    image.layout = uniform_layout ? layout_of(first.state.layout) : VK_IMAGE_LAYOUT_MAX_ENUM;
    if (uniform) {
        image.uniform_contents = first;
        image.subresource_contents.clear();
    }
}

constexpr graph::Stage kShaderStages =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
}  // namespace

bool tracks_subresources(const Image& image) {
    return image.levels > 1 || image.layers > 1;
}

graph::ImageRange whole_image(const Image& image) {
    return {
        static_cast<graph::Aspect>(image.aspect),
        0,
        static_cast<unsigned>(image.levels),
        0,
        image.layers
    };
}

graph::ImageContents image_contents(
    const Image& image, graph::Aspect aspect, unsigned mip, unsigned layer
) {
    return image.subresource_contents.empty()
               ? image.uniform_contents
               : image.subresource_contents[cell(image, aspect, mip, layer)];
}

bool image_has_layout(const Image& image, const graph::ImageRange& range, VkImageLayout layout) {
    if (image.layout != VK_IMAGE_LAYOUT_MAX_ENUM) {
        return image.layout == layout;
    }
    bool matches = true;
    visit_range(range, [&](auto aspect, auto mip, auto layer) {
        matches &= layout_of(image_contents(image, aspect, mip, layer).state.layout) == layout;
    });
    return matches;
}

void set_whole_image_state(Image& image, graph::State state) {
    update_range(image, whole_image(image), [state](auto& contents) {
        contents.state = state;
        if (state.layout == graph::Layout::Undefined) {
            contents.defined = false;
        }
    });
}

void discard_image_contents(Image& image, const graph::ImageRange& range) {
    if (tracks_subresources(image)) {
        update_range(image, range, [](auto& contents) { contents.defined = false; });
    }
}

void record_graph_image_access(Image& image, const graph::ImageUse& use, bool completed) {
    if (!tracks_subresources(image)) {
        if (use.state.access != graph::Access::Write
            && use.state.access != graph::Access::ReadWrite) {
            return;
        }
        const auto aspects = static_cast<VkImageAspectFlags>(use.range.aspects);
        if (!completed || use.store == graph::Store::Discard
            || (use.partial_write && use.load == graph::Load::Discard)) {
            // The deferred pass still needs its original load state. Discard at the store so
            // closing the pass cannot publish an output that the graph considers undefined.
            if (g->rendering && g->pass.deferred && g->rendering_target.colour == &image) {
                g->pass.drop_colour |= aspects;
            } else if (g->rendering && g->pass.deferred && g->rendering_target.depth == &image) {
                g->pass.drop_depth |= aspects;
            } else {
                image.discarded |= aspects;
            }
        } else if (use.state.layout != graph::Layout::ColourAttachment
                   && use.state.layout != graph::Layout::DepthAttachment && !use.partial_write) {
            // Native rendering owns successful attachment writes and depth resolves.
            image.discarded &= ~aspects;
        }
        return;
    }
    update_range(image, use.range, [&](auto& contents) {
        auto state = use.state;
        if (contents.state.layout == state.layout && contents.state.access == graph::Access::Read
            && state.access == graph::Access::Read) {
            state.stages = state.stages | contents.state.stages;
        }
        if (state.access == graph::Access::Write || state.access == graph::Access::ReadWrite) {
            contents.defined = completed && use.store != graph::Store::Discard
                               && (use.load == graph::Load::Clear || !use.partial_write
                                   || (use.load != graph::Load::Discard && contents.defined));
        }
        contents.state = state;
    });
}

void record_sampled_image_access(const Sampled& sampled, graph::Stage stages) {
    record_graph_image_access(
        *sampled.image, {{}, sampled.range, {stages, graph::Access::Read, graph::Layout::Sampled}}
    );
}

graph::ImageRange sampled_range(const Image& image, int base, int top) {
    base = std::clamp(base, 0, image.levels - 1);
    top = std::clamp(top, base, image.levels - 1);
    auto range = whole_image(image);
    if (image.aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        range.aspects = graph::Aspect::Depth;
    }
    range.first_mip = static_cast<unsigned>(base);
    range.mip_count = static_cast<unsigned>(top - base + 1);
    return range;
}

namespace {
bool overlaps(std::uint64_t a, std::uint64_t a_size, std::uint64_t b, std::uint64_t b_size) {
    return a <= b ? b - a < a_size : a - b < b_size;
}

bool overlaps(const VkImageSubresourceRange& a, const VkImageSubresourceRange& b) {
    return (a.aspectMask & b.aspectMask) != 0
           && overlaps(a.baseMipLevel, a.levelCount, b.baseMipLevel, b.levelCount)
           && overlaps(a.baseArrayLayer, a.layerCount, b.baseArrayLayer, b.layerCount);
}
}  // namespace

GraphBarrierBatch::~GraphBarrierBatch() {
    flush();
}

void GraphBarrierBatch::flush() {
    if (m_image_count == 0 && m_buffer_count == 0 && m_writes.sType == 0) {
        return;
    }
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = m_writes.sType != 0 ? 1u : 0u;
    dependency.pMemoryBarriers = &m_writes;
    dependency.bufferMemoryBarrierCount = m_buffer_count;
    dependency.pBufferMemoryBarriers = m_buffers.data();
    dependency.imageMemoryBarrierCount = m_image_count;
    dependency.pImageMemoryBarriers = m_images.data();
    NativeCommandEncoder{m_command}.pipeline_barrier2(dependency);
    count_barrier(m_sources, m_destinations);
    m_image_count = m_buffer_count = 0;
    m_writes = {};
    m_sources = m_destinations = 0;
}

void GraphBarrierBatch::add(const BufferWritesBarrier& writes) {
    if (!g->synchronization2) {
        NativeCommandEncoder{m_command}.pipeline_barrier(
            writes.source, writes.destination, 0, 1, &writes.memory, 0, nullptr, 0, nullptr
        );
        count_barrier(writes.source, writes.destination, "buffer writes");
        return;
    }
    m_writes =
        {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
         nullptr,
         writes.source,
         writes.memory.srcAccessMask,
         writes.destination,
         writes.memory.dstAccessMask};
    m_sources |= writes.source;
    m_destinations |= writes.destination;
}

void GraphBarrierBatch::add(
    VkPipelineStageFlags source,
    VkPipelineStageFlags destination,
    const VkImageMemoryBarrier& barrier
) {
    if (!g->synchronization2) {
        NativeCommandEncoder{m_command}
            .pipeline_barrier(source, destination, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        count_barrier(source, destination);
        return;
    }
    // Coupled depth/stencil aspects can produce successive transitions for the same subresources.
    if (m_image_count == m_images.size()
        || std::any_of(
            m_images.begin(), m_images.begin() + m_image_count, [&](const auto& pending) {
                return pending.image == barrier.image
                       && overlaps(pending.subresourceRange, barrier.subresourceRange);
            }
        )) {
        flush();
    }
    m_images[m_image_count++] =
        {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
         nullptr,
         source,
         barrier.srcAccessMask,
         destination,
         barrier.dstAccessMask,
         barrier.oldLayout,
         barrier.newLayout,
         barrier.srcQueueFamilyIndex,
         barrier.dstQueueFamilyIndex,
         barrier.image,
         barrier.subresourceRange};
    m_sources |= source;
    m_destinations |= destination;
}

void GraphBarrierBatch::add(
    VkPipelineStageFlags source,
    VkPipelineStageFlags destination,
    const VkBufferMemoryBarrier& barrier
) {
    if (!g->synchronization2) {
        NativeCommandEncoder{m_command}
            .pipeline_barrier(source, destination, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        count_barrier(source, destination);
        return;
    }
    if (m_buffer_count == m_buffers.size()
        || std::any_of(
            m_buffers.begin(), m_buffers.begin() + m_buffer_count, [&](const auto& pending) {
                return pending.buffer == barrier.buffer
                       && overlaps(pending.offset, pending.size, barrier.offset, barrier.size);
            }
        )) {
        flush();
    }
    m_buffers[m_buffer_count++] =
        {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
         nullptr,
         source,
         barrier.srcAccessMask,
         destination,
         barrier.dstAccessMask,
         barrier.srcQueueFamilyIndex,
         barrier.dstQueueFamilyIndex,
         barrier.buffer,
         barrier.offset,
         barrier.size};
    m_sources |= source;
    m_destinations |= destination;
}

void record_graph_image_barrier(
    VkCommandBuffer command,
    const graph::Barrier& transition,
    Image& image,
    VkPipelineStageFlags external_source,
    GraphBarrierBatch* batch
) {
    const auto& range = std::get<graph::ImageRange>(transition.range);
    const bool tracked = tracks_subresources(image);
    const bool packed = has_stencil(image.format);
    if (tracked && packed && (range.mip_count > 1 || range.layer_count > 1)) {
        // Prior aspect transitions may have left different physical layouts in this range.
        visit_range(range, [&](auto aspect, auto mip, auto layer) {
            auto single = transition;
            single.range = graph::ImageRange{aspect, mip, 1, layer};
            record_graph_image_barrier(command, single, image, external_source, batch);
        });
        return;
    }
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = access_of(transition.before);
    barrier.dstAccessMask = access_of(transition.after);
    barrier.oldLayout = layout_of(transition.before.layout);
    barrier.newLayout = layout_of(transition.after.layout);
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    // Whole-image imports share the legacy layout, including stencil beside sampled depth.
    const auto aspects = packed ? image.aspect : static_cast<VkImageAspectFlags>(range.aspects);
    barrier.subresourceRange =
        {aspects, range.first_mip, range.mip_count, range.first_layer, range.layer_count};
    VkPipelineStageFlags source =
        external_source != 0 ? external_source : stages_of(transition.before.stages);
    // Combined depth/stencil layouts move together. A barrier for the second
    // aspect must preserve the first aspect's transition rather than replaying
    // its old (possibly UNDEFINED) layout over both aspects.
    if (packed) {
        const auto old_layout =
            tracked ? layout_of(image_contents(
                                    image, graph::Aspect::Depth, range.first_mip, range.first_layer
                      )
                                    .state.layout)
                    : image.layout;
        if (barrier.oldLayout != old_layout) {
            barrier.oldLayout = old_layout;
            if (const auto actual = imported_state(old_layout)) {
                // Keep the logical producer and include accesses allowed by the actual layout
                // after the other aspect's transition.
                barrier.srcAccessMask |= access_of(*actual);
                source |= stages_of(actual->stages);
            } else {
                barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                source = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            }
        }
    }
    // The queue has no stage after handing an acquired image back to presentation.
    auto after = transition.after;
    // A later graph may sample the same layout in a shader stage absent from this plan.
    if (tracked && after.access == graph::Access::Read) {
        if (after.layout == graph::Layout::Sampled || after.layout == graph::Layout::General) {
            after.stages = after.stages | kShaderStages;
        } else if (after.layout == graph::Layout::DepthReadOnly) {
            after.stages = after.stages | kShaderStages | graph::Stage::DepthTests;
        }
        barrier.dstAccessMask = access_of(after);
    }
    VkPipelineStageFlags destination = after.layout == graph::Layout::Present
                                           ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
                                           : stages_of(after.stages);
    if (packed && barrier.oldLayout != barrier.newLayout) {
        // A layout change also synchronizes the other aspect's accesses, including its writes.
        constexpr auto memory = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.srcAccessMask = barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : memory;
        barrier.dstAccessMask = memory;
        source = destination = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    if (batch) {
        batch->add(source, destination, barrier);
    } else {
        NativeCommandEncoder{command}
            .pipeline_barrier(source, destination, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        count_barrier(source, destination);
    }
    if (!tracked) {
        image.layout = barrier.newLayout;
        return;
    }
    auto transitioned = range;
    transitioned.aspects = static_cast<graph::Aspect>(aspects);
    update_range(image, transitioned, [&](auto& contents) {
        if (contents.state.layout != after.layout) {
            contents.state = {graph::Stage::None, graph::Access::None, after.layout};
        }
    });
}

void record_coupled_image_layout(
    VkCommandBuffer command, Image& image, const graph::ImageUse& use
) {
    // The CPU planner keeps aspect histories separate. A partner transition can change the
    // actual layout even when this aspect's logical history needs no barrier.
    if (!tracks_subresources(image)) {
        const auto old = imported_state(image.layout).value();
        record_graph_image_barrier(command, {0, use.range, old, use.state}, image);
        return;
    }
    const auto layout = layout_of(use.state.layout);
    auto range = use.range;
    range.aspects = graph::Aspect::Depth;
    visit_range(range, [&](auto aspect, auto mip, auto layer) {
        const auto old = image_contents(image, aspect, mip, layer).state;
        if (layout_of(old.layout) != layout) {
            record_graph_image_barrier(
                command, {0, graph::ImageRange{aspect, mip, 1, layer}, old, use.state}, image
            );
        }
    });
}

bool graph_buffer_reads_visible(VkBuffer buffer, const graph::Barrier& transition) {
    if (!transition.from_import || transition.after.access != graph::Access::Read) {
        return false;
    }
    const auto found = g->buffer_visibility.find(buffer);
    if (found == g->buffer_visibility.end() || found->second.recording != g->recording) {
        return false;
    }
    const auto& visibility = found->second;
    const auto& bytes = std::get<graph::BufferRange>(transition.range);
    const auto stages = static_cast<unsigned>(transition.after.stages);
    return std::any_of(
        visibility.ranges.begin(),
        visibility.ranges.begin() + visibility.count,
        [&](const auto& range) {
            return (static_cast<unsigned>(range.stages) & stages) == stages
                   && range.bytes.offset <= bytes.offset
                   && range.bytes.offset + range.bytes.size >= bytes.offset + bytes.size;
        }
    );
}

void invalidate_graph_buffer_reads(VkBuffer buffer, graph::BufferRange bytes) {
    const auto found = g->buffer_visibility.find(buffer);
    if (found == g->buffer_visibility.end() || found->second.recording != g->recording) {
        return;
    }
    auto& visibility = found->second;
    const auto end = bytes.offset + bytes.size;
    auto remaining = visibility;
    remaining.count = 0;
    const auto keep = [&](graph::BufferRange range, graph::Stage stages) {
        if (remaining.count < remaining.ranges.size()) {
            remaining.ranges[remaining.count++] = {range, stages};
        }
    };
    for (unsigned i = 0; i < visibility.count; ++i) {
        const auto& range = visibility.ranges[i];
        const auto first = range.bytes.offset;
        const auto last = first + range.bytes.size;
        if (last <= bytes.offset || first >= end) {
            keep(range.bytes, range.stages);
            continue;
        }
        if (first < bytes.offset) {
            keep({first, bytes.offset - first}, range.stages);
        }
        if (last > end) {
            keep({end, last - end}, range.stages);
        }
    }
    visibility = remaining;
}

namespace {
void publish_graph_buffer_reads(VkBuffer buffer, const graph::Barrier& transition) {
    constexpr auto shaders =
        graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
    const auto stages = static_cast<unsigned>(transition.after.stages);
    if ((transition.before.access != graph::Access::Write
         && transition.before.access != graph::Access::ReadWrite)
        || transition.after.access != graph::Access::Read
        || (stages & ~static_cast<unsigned>(shaders)) != 0) {
        return;
    }
    const auto found = g->buffer_visibility.find(buffer);
    if (found == g->buffer_visibility.end()) {
        return;
    }
    auto& visibility = found->second;
    if (visibility.recording != g->recording) {
        visibility.recording = g->recording;
        visibility.count = 0;
    }
    const auto& bytes = std::get<graph::BufferRange>(transition.range);
    for (unsigned i = 0; i < visibility.count; ++i) {
        auto& range = visibility.ranges[i];
        if (range.bytes.offset == bytes.offset && range.bytes.size == bytes.size) {
            range.stages = range.stages | transition.after.stages;
            return;
        }
    }
    if (visibility.count < visibility.ranges.size()) {
        visibility.ranges[visibility.count++] = {bytes, transition.after.stages};
    }
}
}  // namespace

void record_graph_buffer_barrier(
    GraphBarrierBatch& batch,
    const graph::Barrier& transition,
    const BufferRef& buffer,
    graph::BufferKind kind
) {
    const bool tight = kind == graph::BufferKind::Data
                       && renderer::render_opt::enabled(renderer::render_opt::Opt::VkTightSync);
    const auto barrier =
        graph_buffer_barrier(transition, buffer.buffer, buffer.offset, kind, tight);
    const VkPipelineStageFlags source = graph_buffer_source_stages(transition, tight);
    const VkPipelineStageFlags destination = stages_of(transition.after.stages);
    batch.add(source, destination, barrier);
    if (!g->buffer_visibility.empty() && batch.command() == frame().main) {
        publish_graph_buffer_reads(buffer.buffer, transition);
    }
}

}  // namespace openrac::vkgl::detail
