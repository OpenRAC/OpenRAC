// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures the same shader input allocations as descriptor creation, including ring-backed uniform
// views.
#include <iterator>
#include <utility>

#include "renderer/vulkan/graph_barriers.h"
#include "renderer/vulkan/graph_execution_internal.h"

namespace openrac::vkgl {
Result<graph::ImageUse> GraphExecution::read_image(
    std::string name, unsigned texture, graph::Stage stages
) {
    const auto* found = detail::g && detail::g->open ? detail::g->textures.find(texture) : nullptr;
    // Explicit reads cannot substitute an immutable dummy for a missing producer.
    if (!found || !found->image) {
        return unexpected(Error{"graph image read requires a texture allocation"});
    }
    graph::Pass reads{name};
    const auto declared = m_recording->sampled_image(reads, name, found->image, stages);
    // Preserve the original allocation diagnostic before exposing the typed use.
    if (!declared) {
        return unexpected(declared.error());
    }
    return reads.images.front();
}

Result<graph::BufferUse> GraphExecution::read_buffer(
    std::string name, unsigned buffer, graph::Stage stages
) {
    using namespace detail;
    auto* found = g && g->open ? g->buffers.find(buffer) : nullptr;

    // Incomplete descriptors use immutable backend fallbacks rather than user allocations.
    if (!found || !found->size) {
        return unexpected(Error{"graph read requires a nonempty buffer"});
    }

    prepare_frame();
    const BufferRef region = resolve(*found);
    constexpr auto kShaders =
        graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
    const bool shaders_only = (stages | kShaders) == kShaders;
    const auto handle = m_recording->import_buffer(std::move(name), region, true, shaders_only);

    // Conflicting aliases or invalid allocation ranges cannot be sampled.
    if (!handle) {
        return unexpected(handle.error());
    }

    return graph::BufferUse{
        *handle, {region.offset, region.size}, {stages, graph::Access::Read, graph::Layout::Buffer}
    };
}

Result<void> GraphExecution::capture_uniform_reads(graph::Pass& pass, graph::Stage stages) {
    using namespace detail;
    const auto* program = g && g->open ? g->programs.find(g->state.program) : nullptr;

    // Compute capture requires a compute pipeline; graphics capture validates its linked stages
    // before calling here.
    if (!program || (stages == graph::Stage::ComputeShader && !program->compute_pipeline)) {
        return unexpected(Error{"graph input capture requires a bound compute program"});
    }

    // Reflected block bindings track the descriptor path rather than shader-specific slot assumptions.
    for (std::size_t index = 0; index < program->compiled.blocks.size(); ++index) {
        const unsigned binding = program->block_bindings[index];
        const auto& block = program->compiled.blocks[index];
        const unsigned name = binding < kUniformBindings ? g->state.uniform_bindings[binding] : 0;
        const auto* buffer = g->buffers.find(name);

        // Padded or absent CPU uniform blocks are immutable uploads created by the descriptor path.
        if (!buffer || buffer->data.empty()
            || buffer->size < static_cast<std::size_t>(block.size)) {
            continue;
        }

        auto use = read_buffer(block.name, name, stages);

        // Preserve the allocation error before appending any invalid read declaration.
        if (!use) {
            return unexpected(use.error());
        }

        use->range.size = static_cast<std::uint64_t>(block.size);
        pass.buffers.push_back(*use);
    }

    return {};
}

Result<void> GraphExecution::Recording::sampled_image(
    graph::Pass& pass, const std::string& name, const detail::ImagePtr& image, graph::Stage stages
) {
    // Compute imports retain their single-level contract; graphics reads cover the complete
    // shared-layout mip chain.
    if (image->levels != 1 && stages == graph::Stage::ComputeShader) {
        return unexpected(Error{"graph compute input has multiple mip levels: " + name});
    }

    // Only an attachment of the open pass waits for its end; the world's layers keep one pass.
    detail::end_rendering_for(image.get());
    const auto handle = import_image(name, image, true);

    // Conflicting aliases or invalid allocation ranges cannot be sampled.
    if (!handle) {
        return unexpected(handle.error());
    }

    const auto aspect =
        image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth : graph::Aspect::Colour;
    pass.images.push_back(
        {*handle,
         {aspect, 0, static_cast<unsigned>(image->levels)},
         {stages, graph::Access::Read, graph::Layout::Sampled}}
    );
    return {};
}

Result<void> GraphExecution::capture_sampled_reads(graph::Pass& pass, graph::Stage stages) {
    using namespace detail;
    const auto& program = *g->programs.find(g->state.program);

    // Reflected samplers include only bindings used by the linked shader.
    for (std::size_t index = 0; index < program.compiled.uniforms.size(); ++index) {
        const auto& uniform = program.compiled.uniforms[index];

        // Loose uniforms are uploaded by descriptor creation; only sampler-backed allocations are shared GPU inputs.
        if (!uniform.sampler || uniform.binding < 0) {
            continue;
        }

        const auto* texture = sampled_texture(program, index);

        // An absent texture uses an immutable backend dummy rather than a graph allocation.
        if (!texture) {
            continue;
        }

        // Texel buffers use allocation ranges; image samplers require subresource layouts.
        if (uniform.kind == SamplerKind::Buffer) {
            const auto* buffer = g->buffers.find(texture->buffer);

            // Empty texel views use the immutable, numeric-type-correct dummy descriptor.
            if (!buffer || !buffer->size) {
                continue;
            }

            const auto use = read_buffer(uniform.name, texture->buffer, stages);

            // Preserve the allocation error before appending any invalid read declaration.
            if (!use) {
                return unexpected(use.error());
            }

            pass.buffers.push_back(*use);
        } else if (texture->image) {
            const auto read =
                m_recording->sampled_image(pass, uniform.name, texture->image, stages);
            // Sampled images must have one representable whole-image layout and defined contents.
            if (!read) {
                return unexpected(read.error());
            }
        }
    }

    return {};
}

Result<void> GraphExecution::capture_compute_reads(graph::Pass& pass, ComputeInputs inputs) {
    graph::Pass reads;
    const auto uniforms = capture_uniform_reads(reads, graph::Stage::ComputeShader);

    // Invalid uniform imports must not leave partial reads in the caller's declaration.
    if (!uniforms) {
        return unexpected(uniforms.error());
    }

    // Metadata-only branches must not advertise samplers that the selected kernel branch never reads.
    if (inputs == ComputeInputs::All) {
        const auto sampled = capture_sampled_reads(reads, graph::Stage::ComputeShader);

        // Sampler validation must complete before any captured reads are appended.
        if (!sampled) {
            return unexpected(sampled.error());
        }
    }

    // Commit the captured declarations only after every input allocation has passed validation.
    pass.images.insert(
        pass.images.end(),
        std::make_move_iterator(reads.images.begin()),
        std::make_move_iterator(reads.images.end())
    );
    pass.buffers.insert(
        pass.buffers.end(),
        std::make_move_iterator(reads.buffers.begin()),
        std::make_move_iterator(reads.buffers.end())
    );

    return {};
}
}  // namespace openrac::vkgl
