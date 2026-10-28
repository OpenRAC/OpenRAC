// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

namespace openrac::renderer {
// Medium halves both dimensions of every enhanced shadow map.
enum class ShadowQuality {
    Medium,
    High,
    Count
};
inline constexpr const char* kShadowQualityKeys[] = {"medium", "high"};
inline constexpr const char* kShadowQualityNames[] = {"Medium", "High"};

constexpr int shadow_resolution_divisor(ShadowQuality quality) {
    return quality == ShadowQuality::Medium ? 2 : 1;
}
}  // namespace openrac::renderer
