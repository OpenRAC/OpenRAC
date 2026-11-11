// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Scoped surface graph composition: canonical imports and original resource-range remapping.
#include <algorithm>
#include <type_traits>

#include "renderer/vulkan/surface_batch_internal.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;

// Normalizes only the observed entry contract; one allocation cannot change its shape within a
// chain.
Result<graph::Resource> continuation(
    const graph::Resource& next, std::span<const graph::Resource> previous
) {
    auto result = next;
    const auto key = std::visit([](const auto& value) { return value.key; }, next.description);
    const auto found = std::find_if(previous.begin(), previous.end(), [&](const auto& value) {
        return std::visit(
            [&](const auto& description) { return description.key == key; }, value.description
        );
    });
    // The first import supplies the external state for the entire chain.
    if (found == previous.end()) {
        return result;
    }
    // One native identity cannot represent both a buffer and an image.
    if (found->description.index() != result.description.index()) {
        return unexpected(Error{"surface chain allocation type changed"});
    }
    std::visit(
        [&](auto& description) {
            using Description = std::decay_t<decltype(description)>;
            const auto& first = std::get<Description>(found->description);
            description.initial = first.initial;
            description.defined = first.defined;
        },
        result.description
    );
    // Only entry state may differ between declarations of the same allocation.
    if (found->description != result.description) {
        return unexpected(Error{"surface chain allocation shape changed: " + result.name});
    }
    return result;
}

// Retains typed planner validation when importing a continuation's canonical allocation descriptor.
Result<unsigned> import_resource(graph::Graph& target, const graph::Resource& resource) {
    return std::visit(
        [&](const auto& description) -> Result<unsigned> {
            using Description = std::decay_t<decltype(description)>;
            // Preserve typed image and buffer contracts when canonicalising native identities.
            if constexpr (std::is_same_v<Description, graph::ImageDesc>) {
                const auto imported = target.import_image(resource.name, description);
                // Preserve the planner error instead of publishing an unbound handle.
                if (!imported) {
                    return unexpected(imported.error());
                }
                return imported->index;
            } else {
                const auto imported = target.import_buffer(resource.name, description);
                // Preserve the planner error instead of publishing an unbound handle.
                if (!imported) {
                    return unexpected(imported.error());
                }
                return imported->index;
            }
        },
        resource.description
    );
}
}  // namespace

Result<std::vector<unsigned>> SurfaceBatch::import(const GraphExecution::Recording& source) {
    auto& target = *m_execution.m_recording;
    std::vector<unsigned> handles;
    // Every source binding is pinned by the shared executor before its local wrapper is destroyed.
    for (unsigned index = 0; index < source.graph.resources().size(); ++index) {
        const auto description =
            continuation(source.graph.resources()[index], target.graph.resources());
        // Contradictory repeated imports invalidate the whole chain.
        if (!description) {
            return unexpected(description.error());
        }
        const auto imported = import_resource(target.graph, *description);
        // Preserve the planner error instead of publishing an unbound handle.
        if (!imported) {
            return unexpected(imported.error());
        }
        target.bindings.resize(std::max(target.bindings.size(), std::size_t(*imported) + 1));
        target.bindings[*imported] = source.bindings.at(index);
        handles.push_back(*imported);
    }
    // The chain's passes check what the local graph's trusted buffer reads relied on.
    target.trusted_reads = target.trusted_reads || source.trusted_reads;
    return handles;
}

Result<renderer::graph::Pass> SurfaceBatch::remap(
    const renderer::graph::Pass& pass, const std::vector<unsigned>& handles
) {
    auto result = pass;
    // Source graph-local indices must resolve before they can become shared plan handles.
    for (auto& use : result.images) {
        // A local image index must name an imported allocation.
        if (use.image.index >= handles.size()) {
            return unexpected(Error{"unbound surface image"});
        }
        use.image.index = handles[use.image.index];
    }
    // Exact allocation offsets keep shortlist prefixes and mask tails independently ordered.
    for (auto& use : result.buffers) {
        // A local buffer index must name an imported allocation.
        if (use.buffer.index >= handles.size()) {
            return unexpected(Error{"unbound surface buffer"});
        }
        use.buffer.index = handles[use.buffer.index];
    }
    return result;
}
}  // namespace openrac::vkgl::detail
