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

// How the sun's shadow edges are filtered where the opaque world is lit. Soft: a wide kernel, and a
// blend between cascades. Fast: four taps and one cascade at a time, for weak GPUs.
enum class ShadowFilter {
    Fast,
    Soft,
    Count
};
inline constexpr const char* kShadowFilterKeys[] = {"fast", "soft"};
inline constexpr const char* kShadowFilterNames[] = {"Fast", "Soft"};

constexpr int shadow_resolution_divisor(ShadowQuality quality) {
    return quality == ShadowQuality::Low ? 4 : quality == ShadowQuality::Medium ? 2 : 1;
}

constexpr int hero_shadow_resolution(ShadowQuality quality) {
    return quality == ShadowQuality::Low ? 256 : 512;
}
}  // namespace openrac::renderer
