// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <algorithm>

#include "game/common/profile.h"
#include "renderer/water_impulse.h"

namespace openrac::frontend {
// CPU capture only: the worker never dereferences game objects or borrows game RAM.
inline void water_projectile_impulses(
    const host::GameProfile& profile,
    std::span<const host::LiveMoby> current,
    std::span<const host::LiveMoby> previous,
    std::vector<renderer::WaterImpulse>& impulses
) {
    for (const auto& moby : current) {
        if (impulses.size() >= renderer::kWaterImpulseLimit) {
            break;
        }
        const auto parameters = profile.water_projectile(moby.class_id);
        if (!parameters) {
            continue;
        }
        const auto old = std::find_if(previous.begin(), previous.end(), [&](const auto& before) {
            return before.address == moby.address && before.class_id == moby.class_id;
        });
        if (old == previous.end()) {
            continue;  // No sweep from an unrelated spawn/slot.
        }
        renderer::WaterImpulse impulse;
        for (unsigned axis = 0; axis < 3; ++axis) {
            impulse.from[axis] = old->matrix[12 + axis];
            impulse.to[axis] = moby.matrix[12 + axis];
        }
        impulse.radius = (*parameters)[0];
        impulse.strength = (*parameters)[1];
        if (!renderer::valid_water_impulse(impulse)) {
            continue;
        }
        float moved = 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float d = impulse.to[axis] - impulse.from[axis];
            moved += d * d;
        }
        if (moved < 1e-6f || moved > 4096) {
            continue;  // Stationary poses and teleports do not kick water.
        }
        impulse.strength *= std::min(1.f, std::sqrt(moved) / impulse.radius);
        impulses.push_back(impulse);
    }
}
}  // namespace openrac::frontend
