// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/input.h"

#include <algorithm>
#include <utility>

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

void Input::set_bindings(const Bindings& bindings) {
    if (!differ(bindings, m_bindings)) {
        return;
    }
    for (int port = 0; port < kPadPorts; ++port) {
        const auto p = static_cast<std::size_t>(port);
        latch_held(m_last[p], m_bindings.tuning.trigger_threshold, m_latch[p]);
    }
    latch_held_keys(m_last_keys, m_key_latch);
    m_bindings = bindings;
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
                // Its id is its model's GUID and its serial, which stay the same from one run to
                // the next, where SDL's instance number does not (players.h).
                char guid[33] = {};
                SDL_GUIDToString(SDL_GetGamepadGUIDForID(event.gdevice.which), guid, sizeof guid);
                const char* serial = SDL_GetGamepadSerial(g);
                const char* name = SDL_GetGamepadName(g);
                std::vector<std::string> taken;
                for (const Device& d : m_devices) {
                    taken.push_back(d.id);
                }
                Device device{
                    controller_id(guid, serial != nullptr ? serial : "", taken),
                    name != nullptr ? name : "Controller",
                    -1,
                };
                log::info("gamepad connected: {} ({})", device.name, device.id);
                m_gamepads.push_back({g});
                m_devices.push_back(std::move(device));
                assign_players();
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            const auto it = find(event.gdevice.which);
            if (it != m_gamepads.end()) {
                const auto index = it - m_gamepads.begin();
                log::info(
                    "gamepad disconnected: {}", m_devices[static_cast<std::size_t>(index)].name
                );
                if (static_cast<std::size_t>(index) < m_used) {
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
                m_devices.erase(m_devices.begin() + index);
                assign_players();
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
        move_gamepad(static_cast<std::size_t>(it - m_gamepads.begin()), m_used);
        it = m_gamepads.begin() + static_cast<std::ptrdiff_t>(m_used);
        ++m_used;
        assign_players();
    }
    // Port 0 changes hands when its gamepad has been left alone a while, nothing on it pressed or
    // pushed (a stick held all the way sends nothing more): the player has picked up another
    // controller. Two players playing at once keep their ports.
    const Connected& front = m_gamepads.front();
    if (it != m_gamepads.begin() && ns >= front.active_ns + kHandOverNs && !held(front.gamepad)) {
        move_gamepad(static_cast<std::size_t>(it - m_gamepads.begin()), 0);
        it = m_gamepads.begin();
        assign_players();
        log::info(
            "gamepad on port 0: {}",
            SDL_GetGamepadName(it->gamepad) ? SDL_GetGamepadName(it->gamepad) : "?"
        );
    }
    it->active_ns = ns;
}

void Input::move_gamepad(std::size_t from, std::size_t to) {
    const auto rotate = [&](auto& v) {
        const auto a = v.begin() + static_cast<std::ptrdiff_t>(std::min(from, to));
        const auto b = v.begin() + static_cast<std::ptrdiff_t>(std::max(from, to));
        if (from > to) {
            std::rotate(a, b, b + 1);
        } else {
            std::rotate(a, a + 1, b + 1);
        }
    };
    rotate(m_gamepads);
    rotate(m_devices);
}

SDL_Gamepad* Input::gamepad(int port) const {
    const int index = m_assignment.pad[static_cast<std::size_t>(port)];
    return index >= 0 && static_cast<std::size_t>(index) < m_gamepads.size()
               ? m_gamepads[static_cast<std::size_t>(index)].gamepad
               : nullptr;
}

void Input::set_players(const Players& players) {
    if (players == m_players) {
        return;
    }
    m_players = players;
    assign_players();
}

bool Input::take_devices_changed() {
    return std::exchange(m_devices_changed, false);
}

namespace {

// SDL's reading of one gamepad, every button and axis.
PadInputs read_gamepad(SDL_Gamepad* g) {
    PadInputs inputs;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        inputs.buttons[static_cast<std::size_t>(b)] =
            SDL_GetGamepadButton(g, static_cast<SDL_GamepadButton>(b));
    }
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
        inputs.axes[static_cast<std::size_t>(a)] =
            SDL_GetGamepadAxis(g, static_cast<SDL_GamepadAxis>(a));
    }
    return inputs;
}

// The pad inputs a capture counts as pressed on one gamepad: the buttons, the triggers past
// `threshold`, the sticks past most of the way.
std::vector<Binding> pressed_inputs(const PadInputs& inputs, std::uint8_t threshold) {
    std::vector<Binding> out;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        if (inputs.buttons[static_cast<std::size_t>(b)]) {
            out.push_back({Binding::Kind::PadButton, b, 0});
        }
    }
    for (const auto axis : {SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}) {
        if (trigger_byte(inputs.axes[axis]) >= threshold) {
            out.push_back({Binding::Kind::PadTrigger, axis, 0});
        }
    }
    constexpr int kFar = 24000;  // about three quarters of the way
    for (int axis = SDL_GAMEPAD_AXIS_LEFTX; axis <= SDL_GAMEPAD_AXIS_RIGHTY; ++axis) {
        const int value = inputs.axes[static_cast<std::size_t>(axis)];
        if (value <= -kFar || value >= kFar) {
            out.push_back({Binding::Kind::PadAxis, axis, value < 0 ? -1 : 1});
        }
    }
    return out;
}

}  // namespace

PadInputs Input::read_device(std::size_t index) const {
    return index < m_gamepads.size() ? read_gamepad(m_gamepads[index]) : PadInputs{};
}

void Input::begin_capture() {
    // What is held as it begins does not count until let go: the press that asked for the capture
    // is not what it captures.
    SDL_UpdateGamepads();
    m_capture_held.clear();
    for (SDL_Gamepad* g : m_gamepads) {
        m_capture_held.push_back(
            pressed_inputs(read_gamepad(g), m_bindings.tuning.trigger_threshold)
        );
    }
    m_captured.reset();
    m_capturing = true;
}

void Input::end_capture() {
    m_capturing = false;
    m_capture_held.clear();
}

void Input::capture_step() {
    if (m_capture_held.size() != m_gamepads.size()) {
        // A controller came or went: what each holds now is where the capture starts again.
        begin_capture();
        return;
    }
    for (std::size_t d = 0; d < m_gamepads.size() && !m_captured; ++d) {
        const std::vector<Binding> now =
            pressed_inputs(read_gamepad(m_gamepads[d]), m_bindings.tuning.trigger_threshold);
        std::vector<Binding>& held = m_capture_held[d];
        for (const Binding& b : now) {
            if (std::find(held.begin(), held.end(), b) == held.end()) {
                m_captured = b;
                break;
            }
        }
        // What was held and is let go counts again when pressed again.
        std::erase_if(held, [&now](const Binding& b) {
            return std::find(now.begin(), now.end(), b) == now.end();
        });
    }
}

void Input::assign_players() {
    std::vector<std::string> ids;
    for (const Device& d : m_devices) {
        ids.push_back(d.id);
    }
    const Assignment was = m_assignment;
    m_assignment = assign(m_players, ids);
    for (Device& d : m_devices) {
        d.player = -1;
    }
    for (int p = 0; p < kPlayers; ++p) {
        const int index = m_assignment.pad[static_cast<std::size_t>(p)];
        if (index >= 0) {
            m_devices[static_cast<std::size_t>(index)].player = p;
        }
    }
    if (!(was == m_assignment)) {
        for (int p = 0; p < kPlayers; ++p) {
            const int index = m_assignment.pad[static_cast<std::size_t>(p)];
            log::info(
                "player {}: {}{}",
                p + 1,
                index >= 0 ? m_devices[static_cast<std::size_t>(index)].name
                           : std::string("no controller"),
                m_assignment.keyboard == p ? " and the keyboard" : ""
            );
        }
    }
    m_devices_changed = true;
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
    // The keys as the pad reads them: those held back since the bindings changed read as let go.
    std::array<bool, kKeyCount> held_keys{};
    if (state != nullptr && keyboard_ours) {
        std::copy_n(
            state, std::min(static_cast<std::size_t>(key_count), kKeyCount), held_keys.begin()
        );
    }
    m_last_keys = held_keys;
    hold_back_keys(held_keys, m_key_latch);
    const std::span<const bool> keys(held_keys);

    if (m_capturing) {
        capture_step();
    }

    for (int port = 0; port < kPadPorts; ++port) {
        Pad& pad = m_pads[static_cast<std::size_t>(port)];
        // Fresh state each frame, keeping the motors the game asked for.
        const bool small_motor = pad.small_motor;
        const std::uint8_t large_motor = pad.large_motor;
        pad = Pad{};
        pad.small_motor = small_motor;
        pad.large_motor = large_motor;

        SDL_Gamepad* g = gamepad(port);
        const bool keyboard_here = m_assignment.keyboard == port;
        pad.connected = port == 0 || g != nullptr || keyboard_here;
        // The controllers stand still while another window has the keyboard, if the player asked.
        // While the controls window captures a press, every pad reads as nothing held.
        const bool read = g != nullptr && !m_capturing
                          && (m_bindings.tuning.background || SDL_GetKeyboardFocus() != nullptr);
        if (read) {
            PadInputs inputs = read_gamepad(g);
            // A pad labelled Nintendo's way (A east), read by its labels: A is PadSouth.
            if (m_bindings.tuning.face_labels
                && SDL_GetGamepadButtonLabel(g, SDL_GAMEPAD_BUTTON_SOUTH)
                       == SDL_GAMEPAD_BUTTON_LABEL_B) {
                swap_face_labels(inputs);
            }
            // A stick's gate is what its own gamepad has shown: another one on the port starts
            // afresh, with nothing held back.
            auto& gates = m_gates[static_cast<std::size_t>(port)];
            if (m_gates_of[static_cast<std::size_t>(port)] != g) {
                m_gates_of[static_cast<std::size_t>(port)] = g;
                gates = {};
                m_latch[static_cast<std::size_t>(port)] = {};
            }
            m_last[static_cast<std::size_t>(port)] = inputs;
            hold_back(
                inputs, m_bindings.tuning.trigger_threshold, m_latch[static_cast<std::size_t>(port)]
            );
            apply_pad(m_bindings, inputs, gates, pad);
        } else {
            // Gone, or not read: nothing of it is held, so nothing is held back either.
            m_last[static_cast<std::size_t>(port)] = {};
            m_latch[static_cast<std::size_t>(port)] = {};
        }
        if (keyboard_here && m_keyboard_table && !m_capturing) {
            apply_keys(m_bindings, keys, pad);
        }
        if (port == 0) {
            m_touch.idle(SDL_GetTicks());
            m_touch.apply(pad);
        }
    }
}

}  // namespace openrac::platform
