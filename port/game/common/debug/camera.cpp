// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Window-thread UI and input for the free camera; render jobs receive copies, not shared UI state.
#include <algorithm>
#include <utility>

#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {
// Routes gameplay input away from the game while the window owns a free camera.
bool free_camera_enabled() {
    return s.camera_tools.enabled;
}

// Alt or right-click owns relative mouse capture, which SDL hides and releases with window focus.
bool camera_debug_capturing() {
    const bool typing = ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
    return s.camera_tools.enabled && (s.camera_alt != 0 || s.camera_right_mouse) && !typing
           && s.window && SDL_GetKeyboardFocus() == s.window;
}

// Modifier releases and focus loss relinquish capture even when UI controls consume them.
void camera_debug_event(const SDL_Event& event) {
    if (!s.camera_tools.enabled || event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        s.camera_alt = 0;
        s.camera_right_mouse = false;
        s.camera_look_x = s.camera_look_y = 0;
        return;
    }
    const bool key_event = event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP;
    // Releasing either input must preserve capture while the other remains held.
    if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        && event.button.button == SDL_BUTTON_RIGHT) {
        s.camera_right_mouse = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        return;
    }
    if (key_event && (event.key.key == SDLK_LALT || event.key.key == SDLK_RALT)) {
        const unsigned bit = event.key.key == SDLK_LALT ? 1u : 2u;
        if (event.type == SDL_EVENT_KEY_UP) {
            s.camera_alt &= ~bit;
        } else {
            s.camera_alt |= bit;
        }
        return;
    }
    // Captured look takes precedence over UI hover, but never over focus or text entry.
    if (event.type == SDL_EVENT_MOUSE_MOTION && camera_debug_capturing()
        && SDL_GetWindowRelativeMouseMode(s.window)) {
        s.camera_look_x += event.motion.xrel;
        s.camera_look_y += event.motion.yrel;
    }
}

// Samples the display clock and controls once before the scene handoff.
CameraDebugFrame camera_debug_frame() {
    CameraDebugFrame result;
    result.settings = s.camera_tools;
    result.reset = std::exchange(s.camera_reset, false);
    // Ordinary player frames do not sample extra input or update a developer camera clock.
    if (!result.settings.enabled && !result.reset) {
        s.camera_time = 0;
        return result;
    }
    const auto now = SDL_GetTicksNS();
    result.seconds = s.camera_time ? static_cast<float>(double(now - s.camera_time) / 1e9) : 0;
    s.camera_time = now;
    const bool focused = s.window && SDL_GetKeyboardFocus() == s.window;
    const bool typing = ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
    // Inactive windows and UI text entry must never move the fly camera.
    if (result.settings.enabled && focused && !typing) {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        auto& c = result.controls;
        c.forward = keys[SDL_SCANCODE_W];
        c.back = keys[SDL_SCANCODE_S];
        c.left = keys[SDL_SCANCODE_A];
        c.right = keys[SDL_SCANCODE_D];
        c.up = keys[SDL_SCANCODE_E];
        c.down = keys[SDL_SCANCODE_Q];
        c.fast = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
        c.slow = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
        c.look_x = s.camera_look_x;
        c.look_y = s.camera_look_y;
    }
    s.camera_look_x = s.camera_look_y = 0;
    return result;
}

// Shares transient developer switches between the menu and renderer panel.
void camera_tools_controls() {
    auto& tools = s.camera_tools;
    ImGui::Checkbox("Enable Freecam", &tools.enabled);
    int mode = static_cast<int>(tools.mode);
    if (ImGui::Combo("Mode", &mode, "Free look\0Culling inspection\0")) {
        tools.mode = static_cast<CameraDebugMode>(mode);
    }
    if (tools.mode == CameraDebugMode::CullingInspection) {
        ImGui::Combo(
            "Inspect",
            &tools.culling_target,
            "Main camera\0Shadow cascade 1 (nearest)\0Shadow cascade 2\0Shadow cascade 3\0Shadow "
            "cascade 4 (farthest)\0"
        );
        ImGui::TextWrapped(
            "Culling follows the gameplay or cutscene camera. Fly aside to inspect it."
        );
        ImGui::TextWrapped("Shadow cascades require active enhanced shadows.");
    }
    ImGui::SliderFloat("Fly speed", &tools.speed, 1, 200, "%.0f units/s");
    if (ImGui::Button("Reset to game camera")) {
        s.camera_reset = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit Freecam")) {
        tools = {};
        s.camera_alt = 0;
        s.camera_right_mouse = false;
    }
    ImGui::TextWrapped(
        "W/S follow the view, A/D strafe, E up, Q down, Shift faster, Ctrl slower. Hold Alt or "
        "right-click to look (cursor hidden). F5 pauses the game; freecam still moves."
    );
    if (tools.mode == CameraDebugMode::CullingInspection) {
        ImGui::TextWrapped(
            "Yellow: culling frustum. Shadow maps: red, green, blue, purple (near to far). Objects "
            "not submitted by the game cannot be revealed here."
        );
    }
}
}  // namespace openrac::frontend::debug_overlay
