// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Render-owned free flight and live game-camera culling, applied after the captured camera guard.
#include "game/common/debug_camera.h"

#include <algorithm>
#include <cmath>

namespace openrac::frontend {
// Changes only the submitted camera snapshot, keeping game memory and the UI thread untouched.
void DebugCamera::apply(renderer::FrameInput& input, const CameraDebugFrame& frame, int level) {
    // Level travel and explicit resets reseed free flight from the current game camera.
    if (level != m_level || frame.reset) {
        m_active = false;
        m_level = level;
    }
    const auto& settings = frame.settings;
    const bool inspection = settings.enabled && settings.mode == CameraDebugMode::CullingInspection;
    input.culling_camera.reset();
    // Preserve the live gameplay or cutscene camera before replacing the displayed view.
    if (inspection) {
        input.culling_camera = input.camera;
    }
    if (settings.enabled) {
        if (!m_active) {
            m_fly.position = input.camera.position;
            const auto& view = input.camera.view;
            m_fly.yaw = std::atan2(-view[6], -view[2]);
            m_fly.pitch = std::asin(std::clamp(-view[10], -1.0f, 1.0f));
        }
        m_fly.speed = settings.speed;
        // A suspended window must not jump the free camera on resuming.
        auto controls = frame.controls;
        controls.normalise_movement = true;
        m_fly.update(controls, std::clamp(frame.seconds, 0.0f, .1f));
        input.camera.view = m_fly.view();
        input.camera.position = m_fly.position;
        input.camera.target.reset();
        input.camera.target_id = 0;
    }
    m_active = settings.enabled;
    input.culling_inspection.reset();
    // The UI has five choices: the main view followed by four shadow cascades.
    if (inspection) {
        input.culling_inspection = static_cast<unsigned>(std::clamp(settings.culling_target, 0, 4));
    }
}
}  // namespace openrac::frontend
