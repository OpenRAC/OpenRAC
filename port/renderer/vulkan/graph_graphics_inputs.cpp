// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GraphExecution's descriptor capture for read-only graphics phases, including shader-storage
// inputs.
#include <iterator>
#include <utility>

#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/graph_execution_internal.h"

namespace openrac::vkgl {
namespace {
// Graphics descriptors can be shared between both linked stages without losing either reader scope.
constexpr graph::Stage kGraphics = graph::Stage::VertexShader | graph::Stage::FragmentShader;
}  // namespace

Result<void> GraphExecution::capture_graphics_storage_reads(graph::Pass& reads) {
    using namespace detail;
    const auto& program = *g->programs.find(g->state.program);

    // The caller promises these graphics storage blocks are read-only for the recorded phase.
    for (const auto& block : program.compiled.storage) {
        const unsigned name =
            block.binding >= 0 && block.binding < static_cast<int>(kUniformBindings)
                ? g->state.storage_bindings[block.binding]
                : 0;
        const auto* buffer = g->buffers.find(name);

        // Missing bindings use the same immutable dummy descriptor as the native descriptor writer.
        if (!buffer || !buffer->size) {
            continue;
        }

        const auto read = read_buffer(block.name, name, kGraphics);

        // Preserve the allocation error before appending a potentially invalid range.
        if (!read) {
            return unexpected(read.error());
        }

        reads.buffers.push_back(*read);
    }

    return {};
}

Result<void> GraphExecution::capture_graphics_reads(graph::Pass& pass) {
    using namespace detail;
    const auto* program = g && g->open ? g->programs.find(g->state.program) : nullptr;

    // Reflection must belong to a linked graphics program before any descriptor lookup.
    if (!program || program->compiled.vertex.empty() || program->compiled.fragment.empty()) {
        return unexpected(Error{"graph graphics capture requires a bound graphics program"});
    }

    graph::Pass reads;
    const auto uniforms = capture_uniform_reads(reads, kGraphics);

    // Failed captures do not modify the caller's existing declarations.
    if (!uniforms) {
        return unexpected(uniforms.error());
    }

    const auto sampled = capture_sampled_reads(reads, kGraphics);

    // Sampled image validation finishes before committing any captured input.
    if (!sampled) {
        return unexpected(sampled.error());
    }

    const auto storage = capture_graphics_storage_reads(reads);

    // Storage can alias texel-buffer inputs; the graph retains their actual allocation ranges.
    if (!storage) {
        return unexpected(storage.error());
    }

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
