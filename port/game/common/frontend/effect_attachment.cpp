// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captured effect anchors (effect_attachment.h), independent of mesh selection and live RAM.
#include "game/common/frontend/effect_attachment.h"

#include <algorithm>
#include <cstddef>

#include "game/common/frontend/geometry.h"
#include "game/common/profile.h"
#include "renderer/renderer.h"

namespace openrac::frontend {
namespace {

// Maximum per-tick displacement accepted as retained particle history, in world units.
constexpr float kMaximumEffectStep = 4.0f;

}  // namespace

std::optional<EffectParent> captured_effect_parent(
    std::span<const host::LiveMoby> previous,
    std::span<const host::LiveMoby> current,
    std::uint32_t address,
    float alpha
) {
    // Only the same game address can carry geometry across captured ticks.
    const auto matches = [address](const host::LiveMoby& moby) {
        return moby.address == address;
    };
    const auto from = std::find_if(previous.begin(), previous.end(), matches);
    const auto to = std::find_if(current.begin(), current.end(), matches);

    // First appearances and deleted anchors have no adjacent transform pair.
    if (from == previous.end() || to == current.end()) {
        return std::nullopt;
    }

    // Reused slots and teleports cannot move an old display list with the new occupant.
    if (from->class_id != to->class_id || !same_moby(from->matrix, to->matrix)) {
        return std::nullopt;
    }

    return EffectParent{from->matrix, to->matrix, blend_matrix(from->matrix, to->matrix, alpha)};
}

void follow_effect_parent(renderer::EffectQuad& quad, const EffectParent& parent) {
    // Sprite offsets are world-space additions to the anchor's position, not local mesh vertices.
    if (quad.rigid) {
        // One displacement for every corner preserves sprite size and orientation.
        for (auto& corner : quad.corner) {
            // Only XYZ follows the anchor; the captured homogeneous coordinate remains unchanged.
            for (std::size_t axis = 0; axis < 3; ++axis) {
                corner[axis] += parent.drawn[12 + axis] - parent.current[12 + axis];
            }
        }

        return;
    }

    // Shaped callback geometry retains its local placement through the parent's displayed axes.
    for (auto& corner : quad.corner) {
        follow_moby(parent.current, parent.drawn, corner);
    }
}

void blend_effect_quad(
    renderer::EffectQuad& quad, const std::array<std::array<float, 4>, 4>& previous, float alpha
) {
    renderer::Vec3 step{};

    // Opposite strip corners give each quad's centre without depending on sprite orientation.
    for (std::size_t axis = 0; axis < step.size(); ++axis) {
        step[axis] =
            (quad.corner[0][axis] + quad.corner[3][axis] - previous[0][axis] - previous[3][axis])
            * 0.5f;
    }

    // A freed slot can name an unrelated particle on its next appearance.
    if (!(renderer::dot(step, step) < kMaximumEffectStep * kMaximumEffectStep)) {
        return;
    }

    // Random sprite rotations must retain their current shape instead of shrinking between angles.
    if (quad.rigid) {
        for (auto& corner : quad.corner) {
            for (std::size_t axis = 0; axis < step.size(); ++axis) {
                corner[axis] -= step[axis] * (1.0f - alpha);
            }
        }

        return;
    }

    // Shaped effects interpolate all four homogeneous coordinates as the captured quad did.
    for (std::size_t corner = 0; corner < previous.size(); ++corner) {
        for (std::size_t axis = 0; axis < previous[corner].size(); ++axis) {
            quad.corner[corner][axis] =
                previous[corner][axis]
                + (quad.corner[corner][axis] - previous[corner][axis]) * alpha;
        }
    }
}

}  // namespace openrac::frontend
