// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

namespace openrac::renderer {
// Medium halves sun map dimensions; Low quarters them. Ratchet keeps a separate resolution.
enum class ShadowQuality {
    Low,
    Medium,
    High,
    Count
};
inline constexpr const char* kShadowQualityKeys[] = {"low", "medium", "high"};
inline constexpr const char* kShadowQualityNames[] = {"Low", "Medium", "High"};

constexpr int shadow_resolution_divisor(ShadowQuality quality) {
    return quality == ShadowQuality::Low ? 4 : quality == ShadowQuality::Medium ? 2 : 1;
}

constexpr int hero_shadow_resolution(ShadowQuality quality) {
    return quality == ShadowQuality::Low ? 256 : 512;
}
}  // namespace openrac::renderer
