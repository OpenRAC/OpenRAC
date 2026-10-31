// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline HDR encoding; encoded planes remain directly uploadable by the game.
#pragma once

#include "enhanced/lighting/bake_workers.h"
#include "enhanced/lighting/environment.h"
#include "enhanced/lighting/probes.h"

namespace openrac::lighting {
// Separates codec constraints from the caller's bounded execution budget.
struct ProbeTextureEncoding {
    bool probe_gutters = false;  // Keeps each bordered GI tile within one encoding task.
    BakeExecution execution{1};  // Serial unless a controller supplies its bake workers.
};

// Encodes GPU-order HDR texels; probe tiles preserve paired octahedral gutters within 0.5% linear RGB.
std::vector<std::uint8_t> encode_probe_texture(
    ProbeTextureFormat format,
    unsigned width,
    unsigned height,
    std::span<const Float3> pixels,
    const ProbeTextureEncoding& options = {}
);
// Compresses only the irradiance plane, preserving the exact distance and relocation bytes.
std::vector<std::uint8_t> encode_baked_probes(
    const ProbeVolume& volume, ProbeTextureFormat format, const BakeExecution& execution = {1}
);
// Compresses prefiltered radiance while retaining the exact BRDF lookup in its separate plane.
std::vector<std::uint8_t> encode_baked_environment(
    EnvironmentAsset& asset, ProbeTextureFormat format, const BakeExecution& execution = {1}
);
}  // namespace openrac::lighting
