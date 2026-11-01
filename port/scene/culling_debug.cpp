// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Camera and shadow clip-volume outlines, drawn from the inspection view after world objects.
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "common/log.h"
#include "scene/level_renderer.h"

namespace openrac::scene {
// Uses one clip matrix for wireframes, CPU rejection and GPU instance compaction.
std::optional<renderer::Mat4> LevelScene::inspection_clip(const renderer::FrameInput& input) const {
    if (!input.culling_inspection) {
        return std::nullopt;
    }
    const unsigned target = *input.culling_inspection;
    if (target == 0) {
        const auto& camera = input.culling_camera ? *input.culling_camera : input.camera;
        return renderer::multiply(camera.projection, camera.view);
    }
    // A missing or invalid cascade cannot provide a matching visibility volume.
    if (!m_sun_ready || target > kSunCascades) {
        return std::nullopt;
    }
    return m_sun_cascades.matrices[target - 1];
}

// Shows the visibility decision volumes from the independently displayed camera.
void LevelScene::draw_culling_debug(
    const renderer::FrameInput& input, renderer::RenderState& state
) {
    // Default player frames create no geometry or diagnostic GPU resources.
    if (m_culling_lines_failed || !input.culling_inspection) {
        return;
    }
    const auto clip = inspection_clip(input);
    if (!clip) {
        return;
    }
    // Main camera yellow; four shadow cascades run red, green, blue, purple from near to far.
    constexpr std::array<renderer::Vec3, kSunCascades + 1> kColours{
        renderer::Vec3{1, .85f, .1f}, {1, .25f, .2f}, {.2f, 1, .3f}, {.2f, .6f, 1}, {.9f, .3f, 1}
    };
    std::vector<renderer::DebugLineVertex> vertices;
    const bool added =
        renderer::append_debug_frustum(vertices, *clip, kColours[*input.culling_inspection]);
    // Invalid clip geometry must not upload an incomplete wireframe.
    if (!added) {
        return;
    }
    if (!m_culling_lines) {
        m_culling_lines = std::make_unique<renderer::DebugLines>();
    }
    m_culling_lines->set_vertices(std::move(vertices));
    std::string error;
    if (!m_culling_lines->draw(input, state, error)) {
        log::warn("culling debug: {}", error);
        m_culling_lines_failed = true;
    }
}
}  // namespace openrac::scene
