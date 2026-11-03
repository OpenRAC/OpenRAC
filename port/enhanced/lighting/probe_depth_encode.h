// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline local mean/spread packing; native RG16 UNORM channels are the default.
#pragma once
#include "enhanced/lighting/probe_texture_encode.h"

namespace openrac::lighting {
// Physical depth, optional spread and FP32 range planes; byte-packed layouts interleave both scalars in depth.
using EncodedProbeDepth = std::array<std::vector<std::uint8_t>, 3>;
// Fits local mean/spread ranges and encodes disjoint probe tiles with the caller's worker budget.
EncodedProbeDepth encode_probe_depth(
    const ProbeVolume& volume,
    const BakeExecution& execution,
    ProbeDepthFormat format = ProbeDepthFormat::Rg16
);
}  // namespace openrac::lighting
