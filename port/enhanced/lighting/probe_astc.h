// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned ASTC HDR encoder reused across independent tiles and images on one worker.
#pragma once
#include <memory>

#include "enhanced/lighting/probe_compression_tile.h"

namespace openrac::lighting::compression {
class AstcEncoder {
public:
    // Allocates a portable single-worker codec context; callers own scheduling.
    AstcEncoder();
    ~AstcEncoder();
    AstcEncoder(const AstcEncoder&) = delete;
    AstcEncoder& operator=(const AstcEncoder&) = delete;
    // Constrains octahedral borders only for GI, leaving ordinary reflection blocks unrestricted.
    Block encode(const Tile& tile, bool probe_gutters);

private:
    struct Impl;
    std::unique_ptr<Impl>
        m_impl;  // Codec descriptors and search state, owned by this output worker.
};
}  // namespace openrac::lighting::compression
