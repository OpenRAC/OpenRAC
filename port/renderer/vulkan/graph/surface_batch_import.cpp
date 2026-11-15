// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Scoped surface graph composition: canonical imports and original resource-range remapping.
#include <algorithm>
#include <type_traits>

#include "renderer/vulkan/graph/surface_batch_internal.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;
using ResourceDescription = decltype(graph::Resource::description);

// Normalizes only the observed entry contract; one allocation cannot change its shape within a
// chain.
Result<const ResourceDescription*> continuation(
    const graph::Resource& next, std::span<const graph::Resource> previous
) {
    const auto key = std::visit([](const auto& value) { return value.key; }, next.description);
    const auto found = std::find_if(previous.begin(), previous.end(), [&](const auto& value) {
        return std::visit(
            [&](const auto& description) { return description.key == key; }, value.description
        );
    });
    // The first import supplies the external state for the entire chain.
    if (found == previous.end()) {
        return &next.description;
    }
    // One native identity cannot represent both a buffer and an image.
    if (found->description.index() != next.description.index()) {
        return unexpected(Error{"surface chain allocation type changed"});
    }
    // Only entry state may differ between declarations of the same allocation.
    if (!found->same_shape(next)) {
        return unexpected(Error{"surface chain allocation shape changed: " + next.name});
    }
    return &found->description;
}

// Retains typed planner validation when importing a continuation's canonical allocation descriptor.
Result<unsigned> import_resource(
    graph::Graph& target, std::string_view name, const ResourceDescription& resource
) {
    return std::visit(
        [&](const auto& description) -> Result<unsigned> {
            using Description = std::decay_t<decltype(description)>;
            // Preserve typed image and buffer contracts when canonicalising native identities.
            if constexpr (std::is_same_v<Description, graph::ImageDesc>) {
                const auto imported = target.import_image(name, description);
                // Preserve the planner error instead of publishing an unbound handle.
                if (!imported) {
                    return unexpected(imported.error());
                }
                return imported->index;
            } else {
                const auto imported = target.import_buffer(name, description);
                // Preserve the planner error instead of publishing an unbound handle.
                if (!imported) {
                    return unexpected(imported.error());
                }
                return imported->index;
            }
        },
        resource
    );
}
}  // namespace

Result<void> SurfaceBatch::import(
    const GraphExecution::Recording& source, std::span<unsigned> handles
) {
    auto& target = *m_execution.m_recording;
    // Every source binding is pinned by the shared executor before its local wrapper is destroyed.
    for (unsigned index = 0; index < source.graph.resources().size(); ++index) {
        const auto& resource = source.graph.resources()[index];
        const auto description = continuation(resource, target.graph.resources());
        // Contradictory repeated imports invalidate the whole chain.
        if (!description) {
            return unexpected(description.error());
        }
        const auto imported = import_resource(target.graph, resource.name, **description);
        // Preserve the planner error instead of publishing an unbound handle.
        if (!imported) {
            return unexpected(imported.error());
        }
        target.bindings.resize(std::max(target.bindings.size(), std::size_t(*imported) + 1));
        auto& binding = target.bindings[*imported];
        if (!binding.image && !binding.buffer.buffer) {
            binding = source.bindings.at(index);
        }
        handles[index] = *imported;
    }
    // The chain's passes check what the local graph's trusted buffer reads relied on.
    target.trusted_reads = target.trusted_reads || source.trusted_reads;
    return {};
}

}  // namespace openrac::vkgl::detail
