// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Frame-graph imports and canonical allocation identities; compilation lives in compile.cpp.
#include "renderer/graph/frame_graph.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <utility>

#include "renderer/graph/graph_internal.h"

namespace openrac::renderer::graph {
namespace {
std::uint64_t next_owner() {
    static std::atomic<std::uint64_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}

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

bool valid_initial_ranges(const ImageDesc& image) {
    for (std::size_t i = 0; i < image.initial_ranges.size(); ++i) {
        const auto& [range, contents] = image.initial_ranges[i];
        if (!detail::valid_range(image, range)
            || !valid_initial(contents.state, contents.defined, true)
            || !detail::valid_aspects(contents.state.layout, range.aspects)) {
            return false;
        }
        for (const auto& previous : std::span(image.initial_ranges).first(i)) {
            const auto& other = previous.range;
            if ((static_cast<unsigned>(other.aspects) & static_cast<unsigned>(range.aspects))
                && other.first_mip < range.first_mip + range.mip_count
                && range.first_mip < other.first_mip + other.mip_count
                && other.first_layer < range.first_layer + range.layer_count
                && range.first_layer < other.first_layer + other.layer_count) {
                return false;
            }
        }
    }
    return true;
}

// Coalesces exact allocation aliases, rejecting incompatible descriptions or resource types.
template <typename Description, typename Handle>
Result<Handle> import_resource(
    std::vector<Resource>& resources, std::size_t& count, std::string_view name,
    const Description& description, std::uint64_t owner
) {
    // All views of one live allocation must share metadata and initial state.
    for (unsigned index = 0; index < count; ++index) {
        const ResourceKey key =
            std::visit([](const auto& value) { return value.key; }, resources[index].description);

        // A recycled generation is a separate allocation, even when its identity matches.
        if (key != description.key) {
            continue;
        }

        const auto* existing = std::get_if<Description>(&resources[index].description);

        // Conflicting aliases would create two incompatible histories for the same memory.
        if (!existing || *existing != description) {
            return unexpected(Error{std::string("inconsistent alias import: ").append(name)});
        }

        return Handle{index, owner};
    }

    // Public handles must represent every canonical resource index.
    if (count >= std::numeric_limits<unsigned>::max()) {
        return unexpected(Error{"frame graph has too many resources"});
    }

    if (count == resources.size()) {
        // The label may refer to a resource in this vector.
        resources.push_back({std::string(name), description, {}, {}});
    } else {
        auto& resource = resources[count];
        resource.name.assign(name);
        resource.description = description;
    }
    return Handle{static_cast<unsigned>(count++), owner};
}
}  // namespace

Graph::Graph(Graph&& other) noexcept
    : m_resources(std::move(other.m_resources)),
      m_resource_count(std::exchange(other.m_resource_count, 0)),
      m_passes(std::move(other.m_passes)),
      m_spare_passes(std::move(other.m_spare_passes)),
      m_owner(std::exchange(other.m_owner, 0)) {}

Graph& Graph::operator=(Graph&& other) noexcept {
    if (this != &other) {
        m_resources = std::move(other.m_resources);
        m_resource_count = std::exchange(other.m_resource_count, 0);
        m_passes = std::move(other.m_passes);
        m_spare_passes = std::move(other.m_spare_passes);
        m_owner = std::exchange(other.m_owner, 0);
    }
    return *this;
}

Result<ImageHandle> Graph::import_image(std::string_view name, const ImageDesc& description) {
    const unsigned aspects = static_cast<unsigned>(description.aspects);
    const bool depth_stencil = aspects != 0 && (aspects & ~6u) == 0;
    // The three normalised aspect slots must fit in one unsigned 64-bit interval.
    const auto cells = std::uint64_t{description.mips} * description.layers;

    // Reject invalid dimensions and aspect combinations before normalising subresources.
    if (name.empty() || description.key.identity == 0 || !description.mips || !description.layers
        || cells > std::numeric_limits<std::uint64_t>::max() / 3 || (aspects != 1 && !depth_stencil)
        || !valid_initial(description.initial, description.defined, true)
        || !detail::valid_aspects(description.initial.layout, description.aspects)
        || !valid_initial_ranges(description)) {
        return unexpected(Error{std::string("invalid image import: ").append(name)});
    }

    if (!m_owner) {
        m_owner = next_owner();
    }
    return import_resource<ImageDesc, ImageHandle>(
        m_resources, m_resource_count, name, description, m_owner
    );
}

Result<BufferHandle> Graph::import_buffer(std::string_view name, BufferDesc description) {
    // Empty allocations have no valid byte range or canonical resource identity.
    if (name.empty() || description.key.identity == 0 || description.size == 0
        || !valid_initial(description.initial, description.defined, false)) {
        return unexpected(Error{std::string("invalid buffer import: ").append(name)});
    }

    if (!m_owner) {
        m_owner = next_owner();
    }
    return import_resource<BufferDesc, BufferHandle>(
        m_resources, m_resource_count, name, description, m_owner
    );
}

Result<void> Graph::validate_handles(const Pass& pass) const {
    for (const auto& use : pass.images) {
        if (!m_owner || use.image.owner != m_owner || use.image.index >= m_resource_count) {
            return unexpected(Error{pass.name + ": image handle does not belong to this recording"});
        }
    }
    for (const auto& use : pass.buffers) {
        if (!m_owner || use.buffer.owner != m_owner || use.buffer.index >= m_resource_count) {
            return unexpected(Error{pass.name + ": buffer handle does not belong to this recording"});
        }
    }
    return {};
}

Result<void> ResourceRemap::apply(Pass& pass) const {
    for (auto& use : pass.images) {
        if (!rebind(use.image)) {
            return unexpected(Error{pass.name + ": cannot remap image handle from another recording"});
        }
    }
    for (auto& use : pass.buffers) {
        if (!rebind(use.buffer)) {
            return unexpected(Error{pass.name + ": cannot remap buffer handle from another recording"});
        }
    }
    return {};
}

bool Resource::same_shape(const Resource& other) const {
    if (const auto* image = std::get_if<ImageDesc>(&description)) {
        const auto* next = std::get_if<ImageDesc>(&other.description);
        return next && image->key == next->key && image->mips == next->mips
               && image->layers == next->layers && image->aspects == next->aspects;
    }
    const auto& buffer = std::get<BufferDesc>(description);
    const auto* next = std::get_if<BufferDesc>(&other.description);
    return next && buffer.key == next->key && buffer.size == next->size && buffer.kind == next->kind;
}

Pass shader_read_export(std::string name, const Pass& source, Stage stages) {
    Pass result{std::move(name)};
    shader_read_export(result, source, stages);
    return result;
}

void shader_read_export(Pass& result, const Pass& source, Stage stages) {
    result.kind = PassKind::Export;
    result.execution = Stage::None;
    result.cull_if_unused = false;
    result.separate = false;
    result.compute_metadata_only = false;
    result.compute_indexed_storage = false;
    result.images = source.images;
    result.buffers = source.buffers;
    // Attachment operations belong to the producer, never to its later sampling contract.
    for (auto& image : result.images) {
        image.state = {stages, Access::Read, Layout::Sampled};
        image.load = Load::None;
        image.store = Store::Preserve;
        image.partial_write = false;
        image.continuation = false;
    }

    // Read-only exports retain disjoint ranges rather than synchronising unrelated bytes.
    for (auto& buffer : result.buffers) {
        buffer.state = {stages, Access::Read, Layout::Buffer};
    }
}

void Graph::add_pass(Pass pass) {
    m_passes.push_back(std::move(pass));
}

Pass Graph::make_pass(std::string_view name) {
    if (m_spare_passes.empty()) {
        return {std::string(name)};
    }
    auto& spare = m_spare_passes.back();
    spare.name.assign(name);
    auto pass = std::move(spare);
    m_spare_passes.pop_back();
    return pass;
}

void Graph::clear() noexcept {
    if (!m_passes.empty()) {
        m_spare_passes.clear();
        m_spare_passes.swap(m_passes);
        std::ranges::reverse(m_spare_passes);
        for (auto& pass : m_spare_passes) {
            pass.images.clear();
            pass.buffers.clear();
            pass.kind = PassKind::Work;
            pass.execution = Stage::None;
            pass.cull_if_unused = false;
            pass.separate = false;
            pass.compute_metadata_only = false;
            pass.compute_indexed_storage = false;
        }
    }
    m_resource_count = 0;
    m_owner = 0;
}
}  // namespace openrac::renderer::graph
