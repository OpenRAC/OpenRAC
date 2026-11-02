// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Local mean/spread decoding shared by CPU asset checks and legacy ASTC upload fallback.
#pragma once
#include "enhanced/lighting/probes.h"

namespace openrac::lighting {
// Reads finite local ranges without relying on host alignment or byte order.
std::array<float, 4> probe_depth_parameters(const ProbeVolume& volume, std::size_t probe);
// Verifies the scalar payload and FP32 decoding ranges before runtime activation.
void validate_probe_depth(const ProbeVolume& volume);
// Expands normalized mean/spread in GPU atlas order; preserves the stable manual-filter representation.
std::vector<Float2> decode_probe_depth(const ProbeVolume& volume);
// Expands world moments and restores seam gutters for the original non-ASTC hardware filter.
std::vector<Float2> decode_probe_depth_moment_atlas(const ProbeVolume& volume);
// Converts one normalized texel to world moments for offline tools, before directional filtering.
Float2 probe_depth_moments(Float2 encoded, const std::array<float, 4>& parameters);
}  // namespace openrac::lighting
