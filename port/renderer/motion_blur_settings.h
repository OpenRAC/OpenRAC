// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

namespace openrac::renderer {
// Shared player tuning for gameplay and cinematic reconstruction. Defaults retain the JFA preset.
struct MotionBlurSettings {
    int max_radius = 64;             // Display pixels, 1..128.
    int samples = 16;                // Reconstruction samples, 8..64.
    float min_motion = 1.f;          // Ignore exposed motion below this many display pixels.
    float spread = 0.5f;             // JFA perpendicular allowance relative to trail length, 0..1.
    float depth_tolerance = 0.009f;  // Relative receiver-distance tolerance, .001..0.05.
    float cutscene_multiplier = 1;   // Multiplier on strength in real-time cutscenes, 0..2.
};
}  // namespace openrac::renderer
