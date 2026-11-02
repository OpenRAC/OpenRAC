// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "enhanced/lighting/probes.h"

namespace openrac::lighting {
// Offline conversion only: canonical 64-tile-wide rows, including gutters and
// unused tail tiles. Planes are R11G11B10F, RG32F and RGBA16F respectively.
std::vector<std::uint8_t> pack_probe_planes(const ProbeVolume& volume);
// Packs R11G11B10F colour followed by RGBA16F metadata without allocating raw depth staging.
std::vector<std::uint8_t> pack_probe_colour_metadata(const ProbeVolume& volume);
// Serializes a validated volume identity with explicit physical plane-format tags, without a payload.
std::vector<std::uint8_t> encode_probe_header(
    const ProbeVolume& volume, ProbeTextureFormat colour, ProbeDepthFormat depth
);
void read_probe_planes(ProbeVolume& volume, bool expand_linear_maps);
}  // namespace openrac::lighting
