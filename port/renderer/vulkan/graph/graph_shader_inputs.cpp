// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures the input allocations used by graphics and compute descriptor creation.
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"

namespace openrac::vkgl {
namespace {
constexpr auto kGraphics = graph::Stage::VertexShader | graph::Stage::FragmentShader;
}

Result<graph::ImageUse> GraphExecution::read_image(
    std::string_view name, unsigned texture, graph::Stage stages
) {
    const auto* found = detail::g && detail::g->open ? detail::g->textures.find(texture) : nullptr;
    // Explicit reads cannot substitute an immutable dummy for a missing producer.
    if (!found || !found->image) {
        return unexpected(Error{"graph image read requires a texture allocation"});
    }
    return m_recording->sampled_image(name, *found, stages);
}

Result<graph::BufferUse> GraphExecution::read_buffer(
    std::string_view name, unsigned buffer, graph::Stage stages
) {
    using namespace detail;
    auto* found = g && g->open ? g->buffers.find(buffer) : nullptr;

    // Incomplete descriptors use immutable backend fallbacks rather than user allocations.
    if (!found || !found->size) {
        return unexpected(Error{"graph read requires a nonempty buffer"});
    }

    prepare_frame();
    const BufferRef region = resolve(*found);
    constexpr auto shaders =
        graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
    const bool shader_read = stages != graph::Stage::None && (stages | shaders) == shaders;
    const auto handle = m_recording->import_buffer(name, region, true, shader_read);

    // Conflicting aliases or invalid allocation ranges cannot be sampled.
    if (!handle) {
        return unexpected(handle.error());
    }

    return graph::BufferUse{
        *handle, {region.offset, region.size}, {stages, graph::Access::Read, graph::Layout::Buffer}
    };
}

Result<void> GraphExecution::capture_uniform_reads(
    graph::Pass& pass, const detail::Program& program, graph::Stage stages
) {
    using namespace detail;

    // Reflected block bindings track the descriptor path rather than shader-specific slot assumptions.
    for (std::size_t index = 0; index < program.compiled.blocks.size(); ++index) {
        const unsigned binding = program.block_bindings[index];
        const auto& block = program.compiled.blocks[index];
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

Result<graph::ImageUse> GraphExecution::Recording::sampled_image(
    std::string_view name, const detail::Texture& texture, graph::Stage stages
) {
    const auto& image = texture.image;
    // Only an attachment of the open pass waits for its end; the world's layers keep one pass.
    detail::end_rendering_for(image.get());
    const auto handle = import_image(name, image, true);

    // Conflicting aliases or invalid allocation ranges cannot be sampled.
    if (!handle) {
        return unexpected(handle.error());
    }

    return graph::ImageUse{
        *handle,
        detail::sampled_range(*image, texture.base_level, texture.max_level),
        {stages, graph::Access::Read, graph::Layout::Sampled}
    };
}

Result<void> GraphExecution::capture_sampled_reads(
    graph::Pass& pass, const detail::Program& program, graph::Stage stages
) {
    using namespace detail;

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
            // Legacy GL draws substitute the immutable dummy when a texture is
            // also the current attachment. Capture that same effective input.
            if (m_recording->attachment_fallback && stages != graph::Stage::ComputeShader) {
                const auto target = target_of(g->state.draw_framebuffer);
                if (texture->image.get() == target.colour || texture->image.get() == target.depth) {
                    continue;
                }
            }
            const auto read = m_recording->sampled_image(uniform.name, *texture, stages);
            // Sampled images must have one representable whole-image layout and defined contents.
            if (!read) {
                return unexpected(read.error());
            }
            pass.images.push_back(*read);
        }
    }

    return {};
}

Result<void> GraphExecution::capture_graphics_storage_reads(
    graph::Pass& pass, const detail::Program& program
) {
    using namespace detail;
    for (const auto& block : program.compiled.storage) {
        const unsigned name =
            block.binding >= 0 && block.binding < static_cast<int>(kUniformBindings)
                ? g->state.storage_bindings[block.binding]
                : 0;
        const auto* buffer = g->buffers.find(name);
        // Missing bindings use the same immutable dummy descriptor as the native writer.
        if (!buffer || !buffer->size) {
            continue;
        }
        const auto read = read_buffer(block.name, name, kGraphics);
        if (!read) {
            return unexpected(read.error());
        }
        // A reflected read-only block can reuse visibility for stable storage across scopes.
        if (block.read_only && buffer->storage) {
            const auto native = m_recording->bindings[read->buffer.index].buffer.buffer;
            if (native == buffer->own) {
                g->buffer_visibility.try_emplace(native);
            }
        }
        pass.buffers.push_back(*read);
    }
    return {};
}

Result<void> GraphExecution::capture_shader_reads(
    graph::Pass& pass, graph::Stage stages, ComputeInputs inputs
) {
    using namespace detail;
    const auto* program = g && g->open ? g->programs.find(g->state.program) : nullptr;
    const bool graphics = stages == kGraphics;
    if (graphics) {
        if (!program || program->compiled.vertex.empty() || program->compiled.fragment.empty()) {
            return unexpected(Error{"graph graphics capture requires a bound graphics program"});
        }
    } else if (!program || !program->compute_pipeline) {
        return unexpected(Error{"graph input capture requires a bound compute program"});
    }

    auto& reads = m_recording->captured_reads;
    reads.images.clear();
    reads.buffers.clear();

    const auto uniforms = capture_uniform_reads(reads, *program, stages);
    if (!uniforms) {
        return uniforms;
    }

    // Metadata-only branches must not advertise samplers that the selected kernel branch never reads.
    if (inputs == ComputeInputs::All) {
        const auto sampled = capture_sampled_reads(reads, *program, stages);
        if (!sampled) {
            return sampled;
        }
    }
    if (graphics) {
        const auto storage = capture_graphics_storage_reads(reads, *program);
        if (!storage) {
            return storage;
        }
    }
    // Publish only after validation, keeping scratch capacity for the next recording.
    pass.images.insert(pass.images.end(), reads.images.begin(), reads.images.end());
    pass.buffers.insert(pass.buffers.end(), reads.buffers.begin(), reads.buffers.end());
    if (!graphics) {
        pass.compute_metadata_only = inputs == ComputeInputs::UniformBlocks;
    }
    return {};
}

Result<void> GraphExecution::capture_compute_reads(graph::Pass& pass, ComputeInputs inputs) {
    return capture_shader_reads(pass, graph::Stage::ComputeShader, inputs);
}

Result<void> GraphExecution::capture_graphics_reads(graph::Pass& pass) {
    return capture_shader_reads(pass, kGraphics, ComputeInputs::All);
}
}  // namespace openrac::vkgl
