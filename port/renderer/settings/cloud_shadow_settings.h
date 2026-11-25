// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Per-level defaults and live tuning for projected sunlight attenuation.

#pragma once

namespace openrac::renderer {

struct CloudShadowSettings {
    bool enabled = false;
    float speed = .16f;      // Noise-space units per second.
    float direction = 35.f;  // Degrees from +X towards +Y in the ground plane.
    float undulation = .08f;
    float scale = .035f;  // Inverse world units.
    float minimum = 0.f;
    float maximum = 1.f;
    float strength = .9f;

    float effective_strength() const { return enabled ? strength : 0.f; }
};
}  // namespace openrac::renderer
