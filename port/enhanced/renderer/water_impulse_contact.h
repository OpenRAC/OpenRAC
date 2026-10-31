// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <algorithm>
#include <optional>
#include <span>

#include "renderer/water_impulse.h"

namespace openrac::enhanced {
struct WaterImpulseContact {
    int body;
    renderer::Vec3 point;
};

// Conservative coarse-cell query: activation only. Exact swept contact stays on the GPU.
inline std::optional<WaterImpulseContact> water_impulse_contact(
    std::span<const std::array<float, 4>> bodies,
    std::span<const std::array<float, 4>> coast,
    const renderer::WaterImpulse& impulse
) {
    if (!renderer::valid_water_impulse(impulse)) {
        return std::nullopt;
    }
    renderer::Vec3 delta{};
    float length_squared = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        delta[axis] = impulse.to[axis] - impulse.from[axis];
        length_squared += delta[axis] * delta[axis];
    }
    const int steps =
        std::clamp(int(std::ceil(std::max(std::abs(delta[0]), std::abs(delta[1])) / .5f)), 1, 128);
    const int footprint = int(std::ceil(impulse.radius / .5f)) + 1;
    const float reach =
        impulse.radius + .36f;  // Half a coarse cell's diagonal; never misses thin sweeps.
    for (std::size_t b = 0; b < bodies.size() / 5; ++b) {
        const auto* body = bodies.data() + b * 5;
        const int width = int(body[1][3]), height = int(body[2][3]);
        bool outside = false;
        for (unsigned axis = 0; axis < 2; ++axis) {
            const float lo = body[0][axis] - reach;
            const float hi = body[0][axis] + float(axis == 0 ? width : height) * .5f + reach;
            outside |= std::max(impulse.from[axis], impulse.to[axis]) < lo
                       || std::min(impulse.from[axis], impulse.to[axis]) > hi;
        }
        if (outside) {
            continue;
        }
        for (int step = 0; step <= steps; ++step) {
            const float t = float(step) / float(steps);
            const int x = int(std::floor((impulse.from[0] + delta[0] * t - body[0][0]) / .5f));
            const int y = int(std::floor((impulse.from[1] + delta[1] * t - body[0][1]) / .5f));
            for (int cy = std::max(y - footprint, 0); cy <= std::min(y + footprint, height - 1);
                 ++cy) {
                for (int cx = std::max(x - footprint, 0); cx <= std::min(x + footprint, width - 1);
                     ++cx) {
                    const auto at = std::size_t(body[3][3]) + std::size_t(cy * width + cx);
                    if (at >= coast.size() || coast[at][0] < .5f) {
                        continue;
                    }
                    const renderer::Vec3 point{
                        body[0][0] + (float(cx) + .5f) * .5f,
                        body[0][1] + (float(cy) + .5f) * .5f,
                        coast[at][0] - 512.f
                    };
                    float along = 0;
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        along += (point[axis] - impulse.from[axis]) * delta[axis];
                    }
                    along =
                        length_squared > 1e-6f ? std::clamp(along / length_squared, 0.f, 1.f) : 0;
                    float distance = 0;
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        const float d = point[axis] - impulse.from[axis] - delta[axis] * along;
                        distance += d * d;
                    }
                    if (distance <= reach * reach) {
                        return WaterImpulseContact{int(b), point};
                    }
                }
            }
        }
    }
    return std::nullopt;
}

}  // namespace openrac::enhanced
