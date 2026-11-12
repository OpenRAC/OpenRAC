// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <algorithm>
#include <cmath>

#include "renderer/math.h"

namespace openrac::renderer {
inline constexpr unsigned kWaterImpulseLimit = 16;

// A swept pressure impulse, in world units. A zero-length sweep is an explosion.
struct WaterImpulse {
    Vec3 from{}, to{};
    float radius = .25f;
    float strength = .5f;
};

inline bool valid_water_impulse(const WaterImpulse& impulse) {
    return std::all_of(
               impulse.from.begin(),
               impulse.from.end(),
               [](float x) { return std::isfinite(x) && std::abs(x) < 100000; }
           )
           && std::all_of(
               impulse.to.begin(),
               impulse.to.end(),
               [](float x) { return std::isfinite(x) && std::abs(x) < 100000; }
           )
           && std::isfinite(impulse.radius) && impulse.radius > 0 && impulse.radius <= 4
           && std::isfinite(impulse.strength) && impulse.strength > 0 && impulse.strength <= 4;
}
}  // namespace openrac::renderer
