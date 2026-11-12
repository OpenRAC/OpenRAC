// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Validates scoped pipeline bindings and lowers graph draw and dispatch packets.
#include "renderer/vulkan/graph/graph_pipeline.h"

#include <algorithm>
#include <stdexcept>

#include "renderer/vulkan/graph/graph_execution_internal.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;
thread_local const GraphPipelineScope* active_pass = nullptr;
thread_local const ComputeBindingScope* preparing_compute = nullptr;

const graph::Pass* compute_pass() {
    return preparing_compute ? &preparing_compute->pass
           : active_pass     ? &active_pass->pass
                             : nullptr;
}

std::span<const GraphBinding> compute_bindings() {
    return preparing_compute ? preparing_compute->bindings : active_pass->bindings;
}

bool covers(graph::State declared, graph::State required) {
    return declared.layout == required.layout
           && (static_cast<unsigned>(required.stages) & ~static_cast<unsigned>(declared.stages))
                  == 0
           && (static_cast<unsigned>(required.access) & ~static_cast<unsigned>(declared.access))
                  == 0;
}

bool covers(graph::ImageRange declared, graph::ImageRange required) {
    return (static_cast<unsigned>(required.aspects) & ~static_cast<unsigned>(declared.aspects)) == 0
           && required.first_mip >= declared.first_mip
           && required.first_mip - declared.first_mip <= declared.mip_count
           && required.mip_count <= declared.mip_count - (required.first_mip - declared.first_mip)
           && required.first_layer >= declared.first_layer
           && required.first_layer - declared.first_layer <= declared.layer_count
           && required.layer_count
                  <= declared.layer_count - (required.first_layer - declared.first_layer);
}

// Constrains native indirect reads to the pass's declared allocation and byte range.
void validate_indirect_arguments(
    const GraphPipelineScope& scope,
    const graph::DrawIndexedIndirect& args,
    VkBuffer indirect,
    std::optional<graph::BufferRange> generated_arguments
) {
    // The argument declaration constrains both the allocation and byte range.
    bool matches = false;
    constexpr std::uint64_t kArgumentSize = sizeof(VkDrawIndexedIndirectCommand);
    const auto bytes =
        args.count == 0 ? 0 : std::uint64_t{args.count - 1} * args.stride + kArgumentSize;
    // The frame-slot culling graph exports this exact range on the upload stream.
    if (generated_arguments && args.offset >= generated_arguments->offset) {
        const auto offset = args.offset - generated_arguments->offset;
        if (offset <= generated_arguments->size && bytes <= generated_arguments->size - offset) {
            return;
        }
    }
    for (const auto& use : scope.pass.buffers) {
        if (!covers(
                use.state, {graph::Stage::DrawIndirect, graph::Access::Read, graph::Layout::Buffer}
            )) {
            continue;
        }
        const auto& binding = scope.bindings[use.buffer.index].buffer;
        if (indirect == binding.buffer && args.offset >= binding.offset) {
            const auto offset = args.offset - binding.offset;
            matches |= offset >= use.range.offset && offset - use.range.offset <= use.range.size
                       && bytes <= use.range.size - (offset - use.range.offset);
        }
    }
    if (!matches) {
        throw std::runtime_error(
            scope.pass.name + ": indirect arguments are outside their declared range"
        );
    }
}

// Validate the operation's effective target, including accesses omitted from the declaration.
void validate_attachment_access(const Image* image, graph::Aspect aspect, bool write) {
    if (!image) {
        return;
    }
    if (active_pass) {
        const bool colour = aspect == graph::Aspect::Colour;
        const auto stage = colour ? graph::Stage::ColourOutput : graph::Stage::DepthTests;
        for (const auto& use : active_pass->pass.images) {
            const auto& bound = active_pass->bindings[use.image.index].image;
            const bool layout =
                use.state.layout
                    == (colour ? graph::Layout::ColourAttachment : graph::Layout::DepthAttachment)
                || (!colour && !write && use.state.layout == graph::Layout::DepthReadOnly);
            const bool writable = use.state.access == graph::Access::Write
                                  || use.state.access == graph::Access::ReadWrite;
            // A write pass may read contents it cleared or produced earlier in the same pass.
            if (bound && bound->image == image->image && layout && covers(use.range, {aspect})
                && use.state.access != graph::Access::None && (!write || writable)
                && (static_cast<unsigned>(use.state.stages) & static_cast<unsigned>(stage)) != 0) {
                return;
            }
        }
    }
    throw std::runtime_error(
        (active_pass ? active_pass->pass.name : "native work")
        + std::string{": attachment access is outside the owning graph pass declaration"}
    );
}
}  // namespace

bool graph_pipeline_active() {
    return active_pass != nullptr;
}

bool graph_pipeline_records(VkCommandBuffer command) {
    return active_pass
           && command
                  == (active_pass->target != VK_NULL_HANDLE ? active_pass->target : frame().main);
}

ComputeBindingScope::ComputeBindingScope(
    const graph::Pass& declaration, std::span<const GraphBinding> resources
)
    : pass(declaration),
      bindings(resources),
      m_previous(preparing_compute) {
    preparing_compute = this;
}

ComputeBindingScope::~ComputeBindingScope() {
    preparing_compute = m_previous;
}

bool compute_metadata_only() {
    const auto* pass = compute_pass();
    return pass && pass->compute_metadata_only;
}

void validate_compute_sampler(const Sampled& sampled) {
    const auto* pass = compute_pass();
    if (!pass || !sampled.image || sampled.image == g->dummy_image.get()) {
        return;
    }
    const auto bindings = compute_bindings();
    constexpr graph::State required{
        graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Sampled
    };
    const bool declared = std::ranges::any_of(pass->images, [&](const graph::ImageUse& use) {
        return use.image.index < bindings.size()
               && bindings[use.image.index].image.get() == sampled.image
               && covers(use.range, sampled.range) && covers(use.state, required);
    });
    if (!declared) {
        throw std::runtime_error(pass->name + ": dispatch samples an undeclared image or range");
    }
}

BufferRef compute_storage_binding(const ProgramBlock& block, BufferRef buffer) {
    const auto* pass = compute_pass();
    if (!pass || buffer.buffer == g->dummy_buffer) {
        return buffer;
    }
    const auto bindings = compute_bindings();
    constexpr graph::State read{
        graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer
    };
    constexpr graph::State write{
        graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer
    };
    const auto matches = [&](const graph::BufferUse& use) {
        return use.buffer.index < bindings.size()
               && bindings[use.buffer.index].buffer.buffer == buffer.buffer
               && (block.read_only
                       ? covers(use.state, read)
                       : covers(use.state, write)
                             || (pass->compute_indexed_storage && covers(use.state, read)));
    };
    const auto limit = buffer.offset + buffer.size;
    auto first = limit;
    VkDeviceSize end = 0;
    unsigned intervals = 0;
    bool writable = false;
    for (const auto& use : pass->buffers) {
        if (matches(use)) {
            if (use.range.offset < buffer.offset || use.range.offset - buffer.offset >= buffer.size
                || use.range.size == 0
                || use.range.size > buffer.size - (use.range.offset - buffer.offset)) {
                throw std::runtime_error(
                    pass->name + ": storage declaration exceeds its bound buffer"
                );
            }
            first = std::min(first, use.range.offset);
            end = std::max(end, use.range.offset + use.range.size);
            ++intervals;
            writable |= covers(use.state, write);
        }
    }
    if (intervals == 0) {
        throw std::runtime_error(
            pass->name + ": dispatch binds an undeclared storage buffer or lacks matching access"
        );
    }
    if (!block.read_only && !writable) {
        throw std::runtime_error(
            pass->name + ": writable shader storage requires a write declaration"
        );
    }
    if (first != buffer.offset && !pass->compute_indexed_storage) {
        throw std::runtime_error(
            pass->name + ": storage declaration must cover the descriptor base"
        );
    }
    // Join adjacent declarations for kernels that read one interval and write the next.
    if (intervals > 1) {
        auto covered = first;
        while (covered < end) {
            const auto previous = covered;
            for (const auto& use : pass->buffers) {
                if (use.range.offset <= covered && matches(use)) {
                    covered = std::max(covered, use.range.offset + use.range.size);
                }
            }
            if (covered == previous) {
                throw std::runtime_error(
                    pass->name + ": dispatch storage range has an undeclared gap"
                );
            }
        }
    }
    if (end - buffer.offset < static_cast<VkDeviceSize>(std::max(block.size, 1))) {
        throw std::runtime_error(
            pass->name + ": dispatch storage range does not cover its shader block"
        );
    }
    buffer.size = end - buffer.offset;
    return buffer;
}

void validate_compute_outputs(std::span<const BufferRef> storage) {
    const auto* pass = compute_pass();
    if (!pass) {
        return;
    }
    const auto bindings = compute_bindings();
    for (const auto& use : pass->buffers) {
        if (use.state.access == graph::Access::Read || use.state.access == graph::Access::None) {
            continue;
        }
        if (use.buffer.index >= bindings.size()
            || std::ranges::find(
                   storage, bindings[use.buffer.index].buffer.buffer, &BufferRef::buffer
               ) == storage.end()) {
            throw std::runtime_error(
                pass->name + ": dispatch does not bind its declared storage output"
            );
        }
    }
}

void validate_graph_image_access(
    const Image& image, graph::ImageRange range, graph::State state, bool partial_write
) {
    if (active_pass) {
        for (const auto& use : active_pass->pass.images) {
            if (use.image.index >= active_pass->bindings.size()) {
                continue;
            }
            const auto& bound = active_pass->bindings[use.image.index].image;
            if (bound && bound->image == image.image && covers(use.range, range)
                && covers(use.state, state)) {
                if (partial_write && !use.partial_write) {
                    throw std::runtime_error(
                        active_pass->pass.name
                        + ": partial image write requires a partial declaration"
                    );
                }
                return;
            }
        }
    }
    throw std::runtime_error("transfer image access is outside the owning graph pass declaration");
}

void validate_graph_buffer_access(VkBuffer buffer, graph::BufferRange range, graph::State state) {
    if (active_pass) {
        for (const auto& use : active_pass->pass.buffers) {
            if (use.buffer.index >= active_pass->bindings.size()) {
                continue;
            }
            const auto& bound = active_pass->bindings[use.buffer.index].buffer;
            if (bound.buffer != buffer || range.offset < bound.offset
                || !covers(use.state, state)) {
                continue;
            }
            const auto offset = range.offset - bound.offset;
            if (offset >= use.range.offset && offset - use.range.offset <= use.range.size
                && range.size <= use.range.size - (offset - use.range.offset)) {
                return;
            }
        }
    }
    throw std::runtime_error("transfer buffer access is outside the owning graph pass declaration");
}

GraphPipelineScope::GraphPipelineScope(
    const graph::Pass& declaration,
    std::span<const GraphBinding> resources,
    const graph::PlannedPass& scheduled,
    VkCommandBuffer command
)
    : pass(declaration),
      bindings(resources),
      target(command),
      m_commands(scheduled),
      m_previous(active_pass) {
    if (declaration.name != scheduled.name || declaration.kind != scheduled.kind) {
        throw std::logic_error("recording declaration does not match its compiled pass");
    }
    active_pass = this;
}

GraphPipelineScope::~GraphPipelineScope() {
    active_pass = m_previous;
}

void validate_pipeline_draw(const Target& target) {
    if (!active_pass) {
        return;
    }
    for (const auto& use : active_pass->pass.images) {
        const auto layout = use.state.layout;
        if (layout != graph::Layout::ColourAttachment && layout != graph::Layout::DepthAttachment) {
            continue;
        }
        const auto* declared = active_pass->bindings[use.image.index].image.get();
        if (declared
            != (layout == graph::Layout::ColourAttachment ? target.colour : target.depth)) {
            throw std::runtime_error(active_pass->pass.name + ": draw uses a different attachment");
        }
    }
    const auto& state = g->state;
    if (std::ranges::any_of(state.colour_mask, [](bool enabled) { return enabled; })) {
        validate_attachment_access(target.colour, graph::Aspect::Colour, true);
    }
    if (state.depth_test) {
        validate_attachment_access(target.depth, graph::Aspect::Depth, state.depth_mask);
    }
    if (state.stencil_test && target.depth && has_stencil(target.depth->format)) {
        const auto modifies = [](const StencilFace& face) {
            return face.sfail != gl::GL_KEEP || face.dpfail != gl::GL_KEEP
                   || face.dppass != gl::GL_KEEP;
        };
        const bool write = (state.stencil_write_mask & 0xff) != 0
                           && (modifies(state.stencil_front) || modifies(state.stencil_back));
        validate_attachment_access(target.depth, graph::Aspect::Stencil, write);
    }
}

// Clears use attachment stages even when implemented as a native rendering load operation.
void validate_pipeline_clear(const Target& target, VkImageAspectFlags aspects) {
    if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
        validate_attachment_access(target.colour, graph::Aspect::Colour, true);
    }
    if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
        validate_attachment_access(target.depth, graph::Aspect::Depth, true);
    }
    if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
        validate_attachment_access(target.depth, graph::Aspect::Stencil, true);
    }
}

void record_pipeline_command(
    VkCommandBuffer buffer,
    const graph::PipelineCommand& command,
    VkBuffer indirect,
    std::optional<graph::BufferRange> generated_arguments
) {
    if (active_pass) {
        const auto valid = graph::validate_pipeline_command(active_pass->pass, command);
        if (!valid) {
            throw std::runtime_error(valid.error().message);
        }
        if (const auto* args = std::get_if<graph::DrawIndexedIndirect>(&command)) {
            validate_indirect_arguments(*active_pass, *args, indirect, generated_arguments);
        }
    }
    NativeCommandEncoder encoder{buffer};
    if (const auto* draw = std::get_if<graph::Draw>(&command)) {
        encoder.draw(draw->vertices, draw->instances, draw->first_vertex, draw->first_instance);
    } else if (const auto* indexed = std::get_if<graph::DrawIndexed>(&command)) {
        encoder.draw_indexed(
            indexed->indices,
            indexed->instances,
            indexed->first_index,
            indexed->vertex_offset,
            indexed->first_instance
        );
    } else if (const auto* args = std::get_if<graph::DrawIndexedIndirect>(&command)) {
        if (!indirect) {
            throw std::runtime_error("indirect draw requires a native argument buffer");
        }
        encoder.draw_indexed_indirect(indirect, args->offset, args->count, args->stride);
    } else {
        const auto& groups = std::get<graph::Dispatch>(command).groups;
        encoder.dispatch(groups[0], groups[1], groups[2]);
    }
}
}  // namespace openrac::vkgl::detail
