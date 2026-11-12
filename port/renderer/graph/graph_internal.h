// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private normalisation shared by graph validation and fixed-order compilation.
#pragma once
#include <span>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph::detail {
// One contiguous normalised range; image cells are aspect, layer, then mip, with three aspect slots.
struct Use {
    unsigned resource = 0;          // Canonical resource index.
    std::uint64_t begin = 0;        // First byte or image cell.
    std::uint64_t end = 0;          // Exclusive end, validated without overflow.
    State state;                    // Required access.
    Load load = Load::None;         // Attachment entry operation.
    Store store = Store::Preserve;  // Attachment exit operation.
    bool partial_write = false;
    unsigned pass = 0;             // Index in the compilation's flat work list.
};

// Returns whether access consumes or changes contents.
constexpr bool reads(Access access) {
    return access == Access::Read || access == Access::ReadWrite;
}

constexpr bool writes(Access access) {
    return access == Access::Write || access == Access::ReadWrite;
}

// Checks stage/access/layout compatibility for a nonempty image or buffer access.
bool valid_state(State state, bool image);
// Checks whether an image's selected aspects support the requested attachment layout.
bool valid_aspects(Layout layout, Aspect aspects);
bool valid_range(const ImageDesc& image, const ImageRange& range);

template <typename Function>
void for_each_image_interval(const ImageDesc& image, const ImageRange& range, Function interval) {
    for (unsigned aspect = 0; aspect < 3; ++aspect) {
        if ((static_cast<unsigned>(range.aspects) & (1u << aspect)) == 0) {
            continue;
        }
        for (unsigned layer = range.first_layer; layer < range.first_layer + range.layer_count;
             ++layer) {
            const auto begin =
                (std::uint64_t{aspect} * image.layers + layer) * image.mips + range.first_mip;
            interval(begin, begin + range.mip_count);
        }
    }
}
// Returns the normalised byte/cell extent of a validated descriptor.
std::uint64_t extent(const Resource& resource);
// Appends checked intervals and rejects overlapping writers within the pass.
[[nodiscard]] Result<void> normalise(
    const Pass& pass,
    std::span<const Resource> resources,
    std::vector<Use>& output,
    std::vector<Use>& scratch
);
// Converts a single aspect/layer's interval back to its public range.
Range public_range(const Resource& resource, std::uint64_t begin, std::uint64_t end);
}  // namespace openrac::renderer::graph::detail
