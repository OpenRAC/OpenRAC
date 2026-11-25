// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

#include "enhanced/lighting/probes.h"

namespace openrac::lighting::bake_detail {
struct SpatialLink;

// Scratch storage for link records, with a compact index in global insertion order.
class TiledSpatialLinks {
public:
    explicit TiledSpatialLinks(double tile_size, std::size_t cache_bytes);
    ~TiledSpatialLinks();
    TiledSpatialLinks(const TiledSpatialLinks&) = delete;
    TiledSpatialLinks& operator=(const TiledSpatialLinks&) = delete;

    void append(SpatialLink link, Float3 source);
    SpatialLink link(unsigned index) const;
    std::size_t size() const;
    std::size_t cache_capacity() const;
    static std::size_t cache_size(std::uint64_t graph_budget);

private:
    struct Storage;
    std::unique_ptr<Storage> m_storage;
};
}  // namespace openrac::lighting::bake_detail
