// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Frame-graph imports and canonical allocation identities; compilation lives in compile.cpp.
#include "renderer/graph/frame_graph.h"

#include <limits>
#include <utility>

#include "renderer/graph/graph_internal.h"

namespace openrac::renderer::graph {
namespace {
// Rejects empty or contradictory external state before any pass declarations are compiled.
bool valid_initial(State state, bool defined, bool image) {
    // Invalid enum values must not reach diagnostic lookup tables or an executor.
    if (static_cast<unsigned>(state.layout) > static_cast<unsigned>(Layout::Present)) {
        return false;
    }

    // Undefined layout discards image contents at the API boundary.
    if (defined && state.layout == Layout::Undefined) {
        return false;
    }

    // A prior access needs an execution scope, and an empty scope cannot perform memory access.
    if ((state.stages == Stage::None) != (state.access == Access::None)) {
        return false;
    }

    // Imports obey the same access contract as graph passes.
    if (state.access != Access::None && !detail::valid_state(state, image)) {
        return false;
    }

    return image ? state.layout != Layout::Buffer : state.layout == Layout::Buffer;
}

// Coalesces exact allocation aliases, rejecting incompatible descriptions or resource types.
template <typename Description, typename Handle>
Result<Handle> import_resource(
    std::vector<Resource>& resources, std::string name, Description description
) {
    // All views of one live allocation must share metadata and initial state.
    for (unsigned index = 0; index < resources.size(); ++index) {
        const ResourceKey key =
            std::visit([](const auto& value) { return value.key; }, resources[index].description);

        // A recycled generation is a separate allocation, even when its identity matches.
        if (key != description.key) {
            continue;
        }

        const auto* existing = std::get_if<Description>(&resources[index].description);

        // Conflicting aliases would create two incompatible histories for the same memory.
        if (!existing || *existing != description) {
            return unexpected(Error{"inconsistent alias import: " + name});
        }

        return Handle{index};
    }

    // Public handles must represent every canonical resource index.
    if (resources.size() >= std::numeric_limits<unsigned>::max()) {
        return unexpected(Error{"frame graph has too many resources"});
    }

    const unsigned index = static_cast<unsigned>(resources.size());
    resources.push_back({std::move(name), description, {}, {}});
    return Handle{index};
}
}  // namespace

Result<ImageHandle> Graph::import_image(std::string name, ImageDesc description) {
    const unsigned aspects = static_cast<unsigned>(description.aspects);
    const bool depth_stencil = aspects != 0 && (aspects & ~6u) == 0;
    // The three normalised aspect slots must fit in one unsigned 64-bit interval.
    const auto cells = std::uint64_t{description.mips} * description.layers;

    // Reject invalid dimensions and aspect combinations before normalising subresources.
    if (name.empty() || description.key.identity == 0 || !description.mips || !description.layers
        || cells > std::numeric_limits<std::uint64_t>::max() / 3 || (aspects != 1 && !depth_stencil)
        || !valid_initial(description.initial, description.defined, true)
        || !detail::valid_aspects(description.initial.layout, description.aspects)) {
        return unexpected(Error{"invalid image import: " + name});
    }

    return import_resource<ImageDesc, ImageHandle>(m_resources, std::move(name), description);
}

Result<BufferHandle> Graph::import_buffer(std::string name, BufferDesc description) {
    // Empty allocations have no valid byte range or canonical resource identity.
    if (name.empty() || description.key.identity == 0 || description.size == 0
        || !valid_initial(description.initial, description.defined, false)) {
        return unexpected(Error{"invalid buffer import: " + name});
    }

    return import_resource<BufferDesc, BufferHandle>(m_resources, std::move(name), description);
}

Pass shader_read_export(std::string name, const Pass& source, Stage stages) {
    Pass result{std::move(name), source.images, source.buffers, PassKind::Export};

    // Attachment operations belong to the producer, never to its later sampling contract.
    for (auto& image : result.images) {
        image.state = {stages, Access::Read, Layout::Sampled};
        image.load = Load::None;
        image.store = Store::Preserve;
    }

    // Read-only exports retain disjoint ranges rather than synchronising unrelated bytes.
    for (auto& buffer : result.buffers) {
        buffer.state = {stages, Access::Read, Layout::Buffer};
    }

    return result;
}

void Graph::add_pass(Pass pass) {
    m_passes.push_back(std::move(pass));
}
}  // namespace openrac::renderer::graph
