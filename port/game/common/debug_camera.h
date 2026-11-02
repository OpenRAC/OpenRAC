// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned camera-tool inputs cross the render handoff; controller state belongs to its render owner.
#pragma once

#include <cstdint>

#include "renderer/renderer.h"
#include "scene/fly_camera.h"

namespace openrac::frontend {
// Ordinary freecam follows its own visibility; inspection follows the live game camera.
enum class CameraDebugMode : std::uint8_t {
    FreeLook,
    CullingInspection
};

// Developer-only switches, kept out of saved player graphics settings.
struct CameraDebugSettings {
    bool enabled = false;                              // Fly independently of the game camera.
    CameraDebugMode mode = CameraDebugMode::FreeLook;  // Inspection is explicitly selected.
    int culling_target = 0;  // Main camera (0), or shadow cascade 1-4, nearest to farthest.
    float speed = 20;        // World units per second; Shift multiplies this by four.
};

// Window-thread input copied into each bounded scene snapshot, including paused frames.
struct CameraDebugFrame {
    CameraDebugSettings settings;
    scene::FlyControls controls;
    float seconds = 0;   // Display-frame elapsed time, not simulation ticks.
    bool reset = false;  // Reseed from the game's current camera on this frame.
};

// No SDL, ImGui or game RAM access; only the render owner updates these persistent poses.
class DebugCamera {
public:
    // Updates the displayed view and optional game culling camera; level travel resets free flight.
    void apply(renderer::FrameInput& input, const CameraDebugFrame& frame, int level);

private:
    scene::FlyCamera m_fly;  // Shared free-flight movement used by the level viewer.
    bool m_active = false;   // Whether the fly pose was initialised for this activation.
    int m_level = -1;        // Prevents stale poses from surviving level travel.
};
}  // namespace openrac::frontend
