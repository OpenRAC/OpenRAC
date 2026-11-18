// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Transform anchors for captured effects, including objects drawn entirely by callbacks.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "renderer/math.h"

namespace openrac::host {
struct LiveMoby;
}

namespace openrac::renderer {
struct EffectQuad;
}

namespace openrac::frontend {

// Both source ticks and the displayed transform of an effect's anchor.
struct EffectParent {
    renderer::Mat4 previous;  // Source of the delayed display list's vertices.
    renderer::Mat4 current;   // Source of the directly captured quads.
    renderer::Mat4 drawn;     // Transform used to display the anchor this frame.
};

// Resolves `address` across captured ticks without requiring a visible mesh. Missing, reused or
// discontinuous anchors return no transform; `alpha` is the displayed fraction between ticks.
[[nodiscard]] std::optional<EffectParent> captured_effect_parent(
    std::span<const host::LiveMoby> previous,
    std::span<const host::LiveMoby> current,
    std::uint32_t address,
    float alpha
);

// Moves a current-tick quad to its displayed anchor. Rigid sprites follow position alone,
// preserving their world offset and shape even when the anchor turns; other quads follow its axes.
void follow_effect_parent(renderer::EffectQuad& quad, const EffectParent& parent);

// Blends retained world-space quad history, preserving rigid sprites' current shape.
// Displacements of four world units or more are treated as reused slots and left unchanged.
void blend_effect_quad(
    renderer::EffectQuad& quad, const std::array<std::array<float, 4>, 4>& previous, float alpha
);

}  // namespace openrac::frontend
