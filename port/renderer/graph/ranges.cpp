// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Range and access validation, independent of graphics APIs or GPU allocation.
#include <algorithm>
#include <tuple>

#include "renderer/graph/graph_internal.h"

namespace openrac::renderer::graph::detail {
namespace {
// Checks that every requested stage belongs to the permitted scope, including explicit external imports.
bool within(Stage stages, Stage allowed) {
    return stages == Stage::All
           || (static_cast<unsigned>(stages) & ~static_cast<unsigned>(allowed)) == 0;
}

}  // namespace

// Restricts layout and stage combinations to access expressible by the initial executors.
bool valid_state(State state, bool image) {
    constexpr Stage kShaders = Stage::VertexShader | Stage::FragmentShader | Stage::ComputeShader;

    // A declared use must identify both its execution scope and its memory access.
    if (state.stages == Stage::None || state.access == Access::None
        || static_cast<unsigned>(state.access) > static_cast<unsigned>(Access::ReadWrite)) {
        return false;
    }

    // Vertex and indirect inputs are read-only; shader and transfer buffers can also be written.
    if (!image) {
        const Stage writable = kShaders | Stage::Transfer | Stage::Host | Stage::AccelerationBuild;
        const Stage allowed = writable | Stage::VertexInput | Stage::DrawIndirect;
        const auto writers = static_cast<unsigned>(state.stages) & static_cast<unsigned>(writable);
        // A read/write scope can include read-only stages alongside its writers.
        return state.layout == Layout::Buffer && within(state.stages, allowed)
               && (!writes(state.access)
                   || (writers && (reads(state.access) || within(state.stages, writable))));
    }

    // Layouts restrict access types as well as the participating pipeline stages.
    switch (state.layout) {
        case Layout::General:
            return within(state.stages, kShaders);
        case Layout::Sampled:
            return !writes(state.access) && within(state.stages, kShaders);
        case Layout::ColourAttachment:
            return within(state.stages, Stage::ColourOutput);
        case Layout::DepthAttachment:
            // Colour output can write a depth resolve, but depth reads require depth tests.
            return within(state.stages, Stage::DepthTests | Stage::ColourOutput)
                   && (!reads(state.access) || state.stages != Stage::ColourOutput);
        case Layout::DepthReadOnly:
            return !writes(state.access) && within(state.stages, kShaders | Stage::DepthTests);
        case Layout::TransferSource:
            return state.access == Access::Read && within(state.stages, Stage::Transfer);
        case Layout::TransferDestination:
            return state.access == Access::Write && within(state.stages, Stage::Transfer);
        case Layout::Present:
            return state.access == Access::Read && state.stages == Stage::All;
        case Layout::Undefined:
        case Layout::Buffer:
            return false;
    }

    return false;
}

bool valid_aspects(Layout layout, Aspect aspects) {
    const bool depth = layout == Layout::DepthAttachment || layout == Layout::DepthReadOnly;
    const bool colour =
        (static_cast<unsigned>(aspects) & static_cast<unsigned>(Aspect::Colour)) != 0;
    const bool colour_only = layout == Layout::ColourAttachment || layout == Layout::Present;
    return !(depth && colour) && (!colour_only || aspects == Aspect::Colour);
}

bool valid_range(const ImageDesc& image, const ImageRange& range) {
    const unsigned aspects = static_cast<unsigned>(range.aspects);
    return range.mip_count && range.layer_count && aspects
           && (aspects & ~static_cast<unsigned>(image.aspects)) == 0 && range.first_mip < image.mips
           && range.mip_count <= image.mips - range.first_mip && range.first_layer < image.layers
           && range.layer_count <= image.layers - range.first_layer;
}

namespace {
// Checks attachment operations and exports before changing any tracked contents.
bool valid_operations(const ImageUse& use, PassKind kind) {
    // Unknown operations cannot describe attachment contents reliably.
    if (static_cast<unsigned>(use.load) > static_cast<unsigned>(Load::Discard)
        || static_cast<unsigned>(use.store) > static_cast<unsigned>(Store::Discard)) {
        return false;
    }

    const bool attachment = use.state.layout == Layout::ColourAttachment
                            || use.state.layout == Layout::DepthAttachment
                            || use.state.layout == Layout::DepthReadOnly;

    // Load/store operations belong to attachments, not storage or sampled images.
    if ((use.load != Load::None || use.store == Store::Discard) && !attachment) {
        return false;
    }

    // Clear and discard change the contents and require an explicit write declaration.
    if ((use.load == Load::Clear || use.load == Load::Discard || use.store == Store::Discard)
        && !writes(use.state.access)) {
        return false;
    }
    if (use.partial_write && !writes(use.state.access)) {
        return false;
    }

    // Preserving attachment contents implies reading the previous value.
    if (use.load == Load::Preserve && !reads(use.state.access)) {
        return false;
    }

    // Discarded entry contents cannot also be read by the same pass.
    if (use.load == Load::Discard && reads(use.state.access)) {
        return false;
    }

    return kind != PassKind::Export
           || (use.load == Load::None && use.store == Store::Preserve && !writes(use.state.access));
}

// Appends one layer at a time so barriers never describe a nonrectangular mip/layer range.
Result<void> image_uses(
    const ImageUse& use, std::span<const Resource> resources, std::vector<Use>& output
) {
    // Handles are builder-local indices, checked before reading their descriptors.
    if (use.image.index >= resources.size()) {
        return unexpected(Error{"invalid image handle"});
    }

    const auto* image = std::get_if<ImageDesc>(&resources[use.image.index].description);
    const auto& range = use.range;
    if (!image || !valid_range(*image, range)) {
        return unexpected(Error{"invalid image subresource range"});
    }

    // Colour and depth/stencil attachment layouts are not interchangeable.
    if (!valid_aspects(use.state.layout, range.aspects)) {
        return unexpected(Error{"attachment layout does not match image aspects"});
    }

    for_each_image_interval(*image, range, [&](std::uint64_t begin, std::uint64_t end) {
        output.push_back(
            {use.image.index, begin, end, use.state, use.load, use.store, use.partial_write}
        );
    });

    return {};
}

bool range_order(const Use& a, const Use& b) {
    return std::tie(a.resource, a.begin) < std::tie(b.resource, b.begin);
}

// On sorted input all active intervals must already be compatible with each other.
bool conflicting_ranges(std::span<const Use> sorted) {
    const Use* furthest = nullptr;
    for (const auto& use : sorted) {
        if (!furthest || furthest->resource != use.resource || furthest->end <= use.begin) {
            furthest = &use;
            continue;
        }
        if (writes(furthest->state.access) || writes(use.state.access)
            || furthest->state.layout != use.state.layout) {
            return true;
        }
        if (use.end > furthest->end) {
            furthest = &use;
        }
    }
    return false;
}
}  // namespace

std::uint64_t extent(const Resource& resource) {
    // Image cells reserve all three aspect slots, although only declared aspects can be used.
    if (const auto* image = std::get_if<ImageDesc>(&resource.description)) {
        return std::uint64_t{3} * image->mips * image->layers;
    }

    return std::get<BufferDesc>(resource.description).size;
}

Result<void> normalise(
    const Pass& pass,
    std::span<const Resource> resources,
    std::vector<Use>& output,
    std::vector<Use>& scratch
) {
    const auto first = output.size();

    // Validate operations before adding ranges to the compilation work list.
    for (const auto& use : pass.images) {
        // Layout and attachment operations must agree with the declared access.
        if (!valid_state(use.state, true) || !valid_operations(use, pass.kind)) {
            return unexpected(Error{"invalid image access or attachment operation"});
        }

        // Keep an invalid subresource from reaching interval arithmetic.
        if (auto added = image_uses(use, resources, output); !added) {
            return unexpected(added.error());
        }
    }

    // Buffer accesses track byte intervals instead of image cells.
    for (const auto& use : pass.buffers) {
        const auto* buffer = use.buffer.index < resources.size()
                                 ? std::get_if<BufferDesc>(&resources[use.buffer.index].description)
                                 : nullptr;

        // Exports cannot write, and subtracting the checked offset avoids overflow.
        if (!buffer || !use.range.size || use.range.offset >= buffer->size
            || use.range.size > buffer->size - use.range.offset || !valid_state(use.state, false)
            || (pass.kind == PassKind::Export && writes(use.state.access))) {
            return unexpected(Error{"invalid buffer range or access"});
        }

        output.push_back(
            {use.buffer.index, use.range.offset, use.range.offset + use.range.size, use.state}
        );
    }

    // Validate in range order without changing the order in which barriers are emitted.
    std::span<const Use> sorted = std::span(output).subspan(first);
    if (!std::is_sorted(sorted.begin(), sorted.end(), range_order)) {
        scratch.assign(sorted.begin(), sorted.end());
        std::sort(scratch.begin(), scratch.end(), range_order);
        sorted = scratch;
    }
    if (conflicting_ranges(sorted)) {
        return unexpected(Error{"overlapping writes or incompatible reads within one pass"});
    }

    return {};
}

Range public_range(const Resource& resource, std::uint64_t begin, std::uint64_t end) {
    // Image intervals never cross a layer or aspect boundary after normalisation.
    if (const auto* image = std::get_if<ImageDesc>(&resource.description)) {
        const auto layer = begin / image->mips;
        return ImageRange{
            static_cast<Aspect>(1u << (layer / image->layers)),
            static_cast<unsigned>(begin % image->mips),
            static_cast<unsigned>(end - begin),
            static_cast<unsigned>(layer % image->layers),
            1
        };
    }

    return BufferRange{begin, end - begin};
}
}  // namespace openrac::renderer::graph::detail
