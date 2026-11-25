// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

namespace openrac::renderer {
enum class EarlyLightingQuality {
    Normal,
    High,
    Count
};
inline constexpr const char* kEarlyLightingQualityKeys[] = {"normal", "high"};
inline constexpr const char* kEarlyLightingQualityNames[] = {"Normal", "High"};
}  // namespace openrac::renderer
