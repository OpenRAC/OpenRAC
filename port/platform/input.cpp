// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/input.h"

#include <algorithm>

#include <SDL3/SDL.h>

#include "common/log.h"
#include "platform/bindings.h"

namespace openrac::platform {
namespace {

// A button of the gamepad down, a stick or a trigger well out (a trigger only pressed in: some
// rest below zero).
bool held(SDL_Gamepad* g) {
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        if (SDL_GetGamepadButton(g, static_cast<SDL_GamepadButton>(b))) {
            return true;
        }
    }
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
        const auto axis = static_cast<SDL_GamepadAxis>(a);
        const int value = SDL_GetGamepadAxis(g, axis);
        const bool trigger =
            axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
        if (value > 16000 || (!trigger && value < -16000)) {
            return true;
        }
    }
    return false;
}

}  // namespace

void apply_keyboard(Pad& pad, std::span<const bool> keys) {
    static const Bindings defaults = default_bindings();
    apply_keys(defaults, keys, pad);
}

Input::~Input() {
    for (const Connected& c : m_gamepads) {
        SDL_CloseGamepad(c.gamepad);
    }
}

void Input::handle_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED: {
            SDL_Gamepad* g = SDL_OpenGamepad(event.gdevice.which);
            if (g != nullptr) {
                const char* name = SDL_GetGamepadName(g);
                log::info(
                    "gamepad connected: {} ({:04x}:{:04x})",
                    name != nullptr ? name : "?",
                    SDL_GetGamepadVendor(g),
                    SDL_GetGamepadProduct(g)
                );
                m_gamepads.push_back({g});
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            const auto it = find(event.gdevice.which);
            if (it != m_gamepads.end()) {
                log::info("gamepad disconnected");
                if (static_cast<std::size_t>(it - m_gamepads.begin()) < m_used) {
                    --m_used;
                }
                // A gamepad opened later at the same address is another one: its sticks start afresh.
                std::replace(
                    m_gates_of.begin(),
                    m_gates_of.end(),
                    it->gamepad,
                    static_cast<SDL_Gamepad*>(nullptr)
                );
                SDL_CloseGamepad(it->gamepad);
                m_gamepads.erase(it);
            }
            break;
        }
        // The on-screen pad: real fingers only (SDL can report the mouse as a finger too).
        case SDL_EVENT_FINGER_DOWN:
            if (event.tfinger.touchID != SDL_MOUSE_TOUCHID) {
                m_touch.finger_down(event.tfinger.fingerID, event.tfinger.x, event.tfinger.y);
            }
            break;
        case SDL_EVENT_FINGER_MOTION:
            if (event.tfinger.touchID != SDL_MOUSE_TOUCHID) {
                m_touch.finger_move(event.tfinger.fingerID, event.tfinger.x, event.tfinger.y);
            }
            break;
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            m_touch.finger_up(event.tfinger.fingerID);
            break;
        // Another device in use: the on-screen pad is put away until the screen is touched again.
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat && static_cast<std::size_t>(event.key.scancode) < kKeyCount) {
                m_went_down[static_cast<std::size_t>(event.key.scancode)] = 1;
            }
            m_touch.hide();
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            pressed(event.gbutton.which, event.gbutton.timestamp);
            m_touch.hide();
            break;
        // The mouse, for the mods (KeyboardMouse): gathered until the next update(). A touch screen's
        // fingers that SDL also reports as the mouse are not the mouse.
        case SDL_EVENT_MOUSE_MOTION:
            if (event.motion.which != SDL_TOUCH_MOUSEID) {
                m_dx += event.motion.xrel;
                m_dy += event.motion.yrel;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.which != SDL_TOUCH_MOUSEID && event.button.button >= 1
                && event.button.button <= 5) {
                m_mouse_went_down |= 1u << (event.button.button - 1);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL: {
            const float flip = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
            m_wheel_x += event.wheel.x * flip;
            m_wheel_y += event.wheel.y * flip;
            break;
        }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            // A stick pushed well past its dead zone, not a resting stick's drift.
            if (event.gaxis.value > 16000 || event.gaxis.value < -16000) {
                if (const auto it = find(event.gaxis.which); it != m_gamepads.end()) {
                    it->active_ns = event.gaxis.timestamp;
                }
                m_touch.hide();
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_DID_ENTER_BACKGROUND:
            m_touch.release_all();
            break;
        default:
            break;
    }
}

std::vector<Input::Connected>::iterator Input::find(std::uint32_t which) {
    return std::find_if(m_gamepads.begin(), m_gamepads.end(), [&](const Connected& c) {
        return SDL_GetGamepadID(c.gamepad) == which;
    });
}

void Input::pressed(std::uint32_t which, std::uint64_t ns) {
    auto it = find(which);
    if (it == m_gamepads.end()) {
        return;
    }
    // Its first press puts it behind the gamepads used before it and ahead of those never used: a
    // device that only looks like a gamepad (a USB-C dock's or hub's, a keyboard's arrow keys) and
    // was found before the controller no longer holds port 0. A button only: such a device may
    // rest off centre.
    if (static_cast<std::size_t>(it - m_gamepads.begin()) >= m_used) {
        const auto to = m_gamepads.begin() + static_cast<std::ptrdiff_t>(m_used);
        std::rotate(to, it, it + 1);
        it = to;
        ++m_used;
    }
    // Port 0 changes hands when its gamepad has been left alone a while, nothing on it pressed or
    // pushed (a stick held all the way sends nothing more): the player has picked up another
    // controller. Two players playing at once keep their ports.
    const Connected& front = m_gamepads.front();
    if (it != m_gamepads.begin() && ns >= front.active_ns + kHandOverNs && !held(front.gamepad)) {
        std::rotate(m_gamepads.begin(), it, it + 1);
        it = m_gamepads.begin();
        log::info(
            "gamepad on port 0: {}",
            SDL_GetGamepadName(it->gamepad) ? SDL_GetGamepadName(it->gamepad) : "?"
        );
    }
    it->active_ns = ns;
}

SDL_Gamepad* Input::gamepad(int port) const {
    const auto index = static_cast<std::size_t>(port);
    return index < m_gamepads.size() ? m_gamepads[index].gamepad : nullptr;
}

void Input::update() {
    int key_count = 0;
    const bool* state = SDL_GetKeyboardState(&key_count);
    // The keyboard counts only while the window has it, and not while Cmd/Win is held: a system
    // shortcut (Cmd+Shift+4 takes a screenshot) is not the game's L1, and the key-up of a key held
    // when another window took the keyboard never comes.
    const bool keyboard_ours =
        SDL_GetKeyboardFocus() != nullptr && (SDL_GetModState() & SDL_KMOD_GUI) == 0;
    const std::span<const bool> keys(
        state, state != nullptr && keyboard_ours ? static_cast<std::size_t>(key_count) : 0
    );

    // The keyboard and mouse for the mods: what is held now, and what went down since the last frame.
    ++m_km.tick;
    m_km.focused = keyboard_ours;
    for (std::size_t k = 0; k < kKeyCount; ++k) {
        const bool held = k < keys.size() && keys[k];
        m_km.keys[k] =
            static_cast<std::uint8_t>((held ? 1 : 0) | (keyboard_ours && m_went_down[k] ? 2 : 0));
    }
    m_went_down.fill(0);
    float mx = 0.0f;
    float my = 0.0f;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    const bool mouse_ours = SDL_GetMouseFocus() != nullptr || m_km.captured;
    m_km.mouse_held =
        keyboard_ours && mouse_ours ? static_cast<std::uint32_t>(buttons) & 0x1Fu : 0u;
    m_km.mouse_pressed = keyboard_ours ? m_mouse_went_down & 0x1Fu : 0u;
    m_km.mouse_dx = keyboard_ours ? m_dx : 0.0f;
    m_km.mouse_dy = keyboard_ours ? m_dy : 0.0f;
    m_km.wheel_x = keyboard_ours ? m_wheel_x : 0.0f;
    m_km.wheel_y = keyboard_ours ? m_wheel_y : 0.0f;
    m_mouse_went_down = 0;
    m_dx = m_dy = m_wheel_x = m_wheel_y = 0.0f;

    refresh_pads();
    for (int port = 0; port < kPadPorts; ++port) {
        // The large motor has a speed, the small one is on or off: they
        // drive SDL's low- and high-frequency rumble. Renewed each frame,
        // so a short duration stops it when the game stops asking.
        const Pad& pad = m_pads[static_cast<std::size_t>(port)];
        if (SDL_Gamepad* g = gamepad(port); g != nullptr) {
            const auto low = static_cast<Uint16>(pad.large_motor * 257);
            const auto high = static_cast<Uint16>(pad.small_motor ? 0xFFFF : 0);
            SDL_RumbleGamepad(g, low, high, 100);
        }
    }
}

void Input::refresh_pads() {
    // SDL brings the gamepads' state up to date as it reads the events; the events themselves
    // stay queued for the next read of the window's events.
    SDL_UpdateGamepads();
    int key_count = 0;
    const bool* state = SDL_GetKeyboardState(&key_count);
    const bool keyboard_ours =
        SDL_GetKeyboardFocus() != nullptr && (SDL_GetModState() & SDL_KMOD_GUI) == 0;
    const std::span<const bool> keys(
        state, state != nullptr && keyboard_ours ? static_cast<std::size_t>(key_count) : 0
    );

    for (int port = 0; port < kPadPorts; ++port) {
        Pad& pad = m_pads[static_cast<std::size_t>(port)];
        // Fresh state each frame, keeping the motors the game asked for.
        const bool small_motor = pad.small_motor;
        const std::uint8_t large_motor = pad.large_motor;
        pad = Pad{};
        pad.small_motor = small_motor;
        pad.large_motor = large_motor;

        SDL_Gamepad* g = gamepad(port);
        pad.connected = port == 0 || g != nullptr;
        if (g != nullptr) {
            PadInputs inputs;
            for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
                inputs.buttons[static_cast<std::size_t>(b)] =
                    SDL_GetGamepadButton(g, static_cast<SDL_GamepadButton>(b));
            }
            for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
                inputs.axes[static_cast<std::size_t>(a)] =
                    SDL_GetGamepadAxis(g, static_cast<SDL_GamepadAxis>(a));
            }
            // A stick's gate is what its own gamepad has shown: another one on the port starts
            // afresh.
            auto& gates = m_gates[static_cast<std::size_t>(port)];
            if (m_gates_of[static_cast<std::size_t>(port)] != g) {
                m_gates_of[static_cast<std::size_t>(port)] = g;
                gates = {};
            }
            apply_pad(m_bindings, inputs, gates, pad);
        }
        if (port == 0) {
            if (m_keyboard_table) {
                apply_keys(m_bindings, keys, pad);
            }
            m_touch.idle(SDL_GetTicks());
            m_touch.apply(pad);
        }
    }
}

}  // namespace openrac::platform
