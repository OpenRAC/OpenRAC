// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Window-thread UI and input for the free camera; render jobs receive copies, not shared UI state.
#include <algorithm>
#include <cmath>
#include <utility>

#include "debug/menu_ui.h"
#include "debug/overlay.h"
#include "frontend.h"
#include "platform/input.h"

namespace openrac::frontend::debug_overlay {
namespace {
constexpr float kStickDeadZone = 0.2f;    // of a stick's travel, round
constexpr float kTriggerDeadZone = 0.1f;  // of a trigger's travel
constexpr float kPadLookRate = 900.0f;    // mouse pixels a second, right stick fully pushed
constexpr float kAxisScale = 1.0f / 32767.0f;

// Every connected gamepad's L3 and R3 held together (any one gamepad).
bool pad_chord_held() {
    bool held = false;
    int count = 0;
    if (SDL_JoystickID* ids = SDL_GetGamepads(&count)) {
        for (int i = 0; i < count && !held; ++i) {
            SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
            held = pad != nullptr && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK)
                   && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
        }
        SDL_free(ids);
    }
    return held;
}

// The gamepads fly the freecam (any of them, added up): left stick moves and strafes, right stick
// looks, R2 rises and L2 sinks, R1 held is faster and L1 held slower, as Shift and Ctrl.
void pad_controls(scene::FlyControls& c, float seconds) {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids == nullptr) {
        return;
    }
    const auto axis = [](SDL_Gamepad* pad, SDL_GamepadAxis a) {
        return std::clamp(static_cast<float>(SDL_GetGamepadAxis(pad, a)) * kAxisScale, -1.0f, 1.0f);
    };
    const auto trigger = [&](SDL_Gamepad* pad, SDL_GamepadAxis a) {
        const float t = axis(pad, a);
        return t > kTriggerDeadZone ? (t - kTriggerDeadZone) / (1.0f - kTriggerDeadZone) : 0.0f;
    };
    float look_x = 0.0f;
    float look_y = 0.0f;
    for (int i = 0; i < count; ++i) {
        SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
        if (pad == nullptr) {
            continue;
        }
        float mx = axis(pad, SDL_GAMEPAD_AXIS_LEFTX);
        float my = axis(pad, SDL_GAMEPAD_AXIS_LEFTY);
        scene::stick_past_dead_zone(mx, my, kStickDeadZone);
        c.sideways += mx;
        c.ahead -= my;  // SDL's sticks are positive down
        float lx = axis(pad, SDL_GAMEPAD_AXIS_RIGHTX);
        float ly = axis(pad, SDL_GAMEPAD_AXIS_RIGHTY);
        scene::stick_past_dead_zone(lx, ly, kStickDeadZone);
        // Squared for fine aim near the centre, full speed at the rim.
        const float pushed = std::sqrt(lx * lx + ly * ly);
        look_x += lx * pushed;
        look_y += ly * pushed;
        c.vertical += trigger(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
                      - trigger(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        c.fast = c.fast || SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        c.slow = c.slow || SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    }
    SDL_free(ids);
    // A long frame (the window held, resumed) must not spin the view.
    const float step = kPadLookRate * std::min(seconds, 0.1f);
    c.look_x += std::clamp(look_x, -1.0f, 1.0f) * step;
    c.look_y += std::clamp(look_y, -1.0f, 1.0f) * step;
}
}  // namespace

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

namespace {
std::optional<CameraDebugPlacement> review_placement;
std::uint64_t placement_serial = 0;
}  // namespace

void place_review_camera(int level, scene::Vec3 position, scene::Vec3 forward) {
    s.camera_tools.enabled = true;
    s.camera_tools.mode = CameraDebugMode::FreeLook;
    review_placement = CameraDebugPlacement{level, position, forward, ++placement_serial};
}

// L3+R3 on a gamepad turns freecam on (developer mode only, so a player's stray press does
// not) and off. Checked where the window samples the freecam and before the game reads its pad,
// so whichever comes first in a frame sees it. What is held then (L3 and R3 too) is held back from
// the game until let go: R3 opens the map, L3 shows the health and bolts. R3 alone reaches
// the game as before, with no wait, so an R3 pressed a frame before L3 still opens the map.
void camera_pad_chord() {
    // A player's game (no developer mode, no freecam) reads nothing more.
    if (!s.developer && !s.camera_tools.enabled) {
        s.camera_pad_chord = false;
        return;
    }
    const bool focused = s.window && SDL_GetKeyboardFocus() == s.window;
    // The menu's own button may be L3 or R3: its press opened the menu, the chord closes it.
    const SDL_GamepadButton menu = menu_bar_button();
    const bool menu_ours =
        menu == SDL_GAMEPAD_BUTTON_LEFT_STICK || menu == SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    const bool chord = focused && (!s.menu_open || menu_ours) && pad_chord_held();
    if (chord && !s.camera_pad_chord) {
        if (s.menu_open) {
            set_menu(false);  // holds back what is held, as below
        }
        s.camera_tools.enabled = !s.camera_tools.enabled;
        if (platform::Input* input = frontend::input()) {
            input->hold_back_held();
            input->refresh_pads();  // the pads already read this frame held the chord
        }
        notify({
            .id = "freecam",
            .title = s.camera_tools.enabled ? "Freecam on" : "Freecam off",
            .text = s.camera_tools.enabled ? "L3+R3 gives the view back to the game." : "",
            .seconds = 2.0,
        });
    }
    s.camera_pad_chord = chord;
}

// Samples the display clock and controls once before the scene handoff.
CameraDebugFrame camera_debug_frame() {
    CameraDebugFrame result;
    const bool focused = s.window && SDL_GetKeyboardFocus() == s.window;
    const bool typing = ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
    camera_pad_chord();
    result.settings = s.camera_tools;
    if (!result.settings.enabled) {
        review_placement.reset();
    }
    result.placement = review_placement;
    result.reset = std::exchange(s.camera_reset, false);
    // Ordinary player frames do not sample extra input or update a developer camera clock.
    if (!result.settings.enabled && !result.reset) {
        s.camera_time = 0;
        return result;
    }
    const auto now = SDL_GetTicksNS();
    result.seconds = s.camera_time ? static_cast<float>(double(now - s.camera_time) / 1e9) : 0;
    s.camera_time = now;
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
        // The gamepads move the menu's rows while it is open, not the camera.
        if (!s.menu_open) {
            pad_controls(c, result.seconds);
        }
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
    ImGui::TextWrapped(
        "Gamepad: left stick moves, right stick looks, R2 up, L2 down, R1 faster, L1 slower. "
        "L3+R3 turns freecam off (and on, in developer mode)."
    );
    if (tools.mode == CameraDebugMode::CullingInspection) {
        ImGui::TextWrapped(
            "Yellow: culling frustum. Shadow maps: red, green, blue, purple (near to far). Objects "
            "not submitted by the game cannot be revealed here."
        );
    }
}
}  // namespace openrac::frontend::debug_overlay
