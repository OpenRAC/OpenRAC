// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "renderer/math.h"

namespace openrac::enhanced {
inline bool water_window_same_height(float a, float b) {
    return std::abs(a - b) <= .5f;
}

// Conservative side-plane test, independent of reversed-depth conventions.
inline bool water_window_visible(
    const renderer::Mat4& m, const renderer::Vec3& low, const renderer::Vec3& high
) {
    for (int axis = 0; axis < 2; ++axis) {
        for (float sign : {-1.f, 1.f}) {
            float distance = m[15] + sign * m[12 + axis];
            for (int c = 0; c < 3; ++c) {
                const float n = m[c * 4 + 3] + sign * m[c * 4 + axis];
                distance += n * (n >= 0 ? high[c] : low[c]) + .5f * std::abs(n);
            }
            if (distance < 0) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace openrac::enhanced
