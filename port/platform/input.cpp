// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/input.h"

#include <algorithm>
#include <cmath>
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

void Input::set_bindings(const std::array<Bindings, kPadPorts>& bindings) {
    for (int port = 0; port < kPadPorts; ++port) {
        const auto p = static_cast<std::size_t>(port);
        if (!differ(bindings[p], m_bindings[p])) {
            continue;
        }
        latch_held(m_last[p], m_bindings[p].tuning.trigger_threshold, m_latch[p]);
        if (m_assignment.keyboard == port) {
            latch_held_keys(m_last_keys, m_key_latch);
        }
        m_bindings[p] = bindings[p];
    }
}

Input::~Input() {
    stop_rumble();
    if (m_vibrator != nullptr) {
        SDL_CloseHaptic(m_vibrator);
    }
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
                // Nothing more is sent to it; the port it played on starts afresh.
                for (Rumble& r : m_rumble) {
                    if (r.gamepad == it->gamepad) {
                        r = Rumble{};
                    }
                }
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
            if (const auto it = find(event.gbutton.which);
                it != m_gamepads.end() && event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT) {
                it->tapped[event.gbutton.button] = kTapReads;
            }
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
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            // Back from another window: what the pad and the keys held over there (an Alt+Tab, a
            // button held while switching) is not a press in the game until let go.
            hold_back_held();
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
    return index < m_gamepads.size() ? read_gamepad(m_gamepads[index].gamepad) : PadInputs{};
}

void Input::taps_read(int port) {
    m_menu_chord[static_cast<std::size_t>(port)].read();
    SDL_Gamepad* g = gamepad(port);
    for (Connected& c : m_gamepads) {
        if (g != nullptr && c.gamepad == g) {
            c.tapped = {};
        }
    }
}

void Input::set_menu_chord(bool on) {
    if (on != m_menu_chord_on) {
        m_menu_chord_on = on;
        m_menu_chord = {};
    }
}

bool Input::take_menu_chord() {
    bool fired = false;
    for (MenuChord& chord : m_menu_chord) {
        fired = chord.take_fired() || fired;
    }
    return fired;
}

void Input::hold_back_held() {
    SDL_UpdateGamepads();
    for (MenuChord& chord : m_menu_chord) {
        chord.reset();
    }
    for (Connected& c : m_gamepads) {
        c.tapped = {};  // a tap before this was not the game's either
    }
    for (int port = 0; port < kPadPorts; ++port) {
        const auto p = static_cast<std::size_t>(port);
        if (SDL_Gamepad* g = gamepad(port)) {
            latch_held(read_gamepad(g), m_bindings[p].tuning.trigger_threshold, m_latch[p]);
        }
    }
    int key_count = 0;
    if (const bool* state = SDL_GetKeyboardState(&key_count)) {
        std::array<bool, kKeyCount> keys{};
        std::copy_n(state, std::min(static_cast<std::size_t>(key_count), kKeyCount), keys.begin());
        latch_held_keys(keys, m_key_latch);
    }
}

void Input::begin_capture() {
    // What is held as it begins does not count until let go: the press that asked for the capture
    // is not what it captures.
    SDL_UpdateGamepads();
    m_capture_held.clear();
    for (Connected& c : m_gamepads) {
        c.tapped = {};
    }
    for (const Connected& c : m_gamepads) {
        m_capture_held.push_back(
            pressed_inputs(read_gamepad(c.gamepad), m_bindings[0].tuning.trigger_threshold)
        );
    }
    m_captured.reset();
    m_capturing = true;
}

void Input::end_capture() {
    m_capturing = false;
    m_capture_held.clear();
}

void Input::poll_capture() {
    if (m_capturing) {
        SDL_UpdateGamepads();
        capture_step();
    }
}

void Input::capture_step() {
    if (m_capture_held.size() != m_gamepads.size()) {
        // A controller came or went: what each holds now is where the capture starts again.
        begin_capture();
        return;
    }
    for (std::size_t d = 0; d < m_gamepads.size() && !m_captured; ++d) {
        const std::vector<Binding> now =
            pressed_inputs(read_gamepad(m_gamepads[d].gamepad), m_bindings[0].tuning.trigger_threshold);
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
    // The keyboard moved (a controller came or went, and the keyboard was a fallback): what is held
    // on it waits for its release, so a key held for one player does not press for the other, or
    // press at all the moment a controller leaves.
    if (was.keyboard != m_assignment.keyboard) {
        latch_held_keys(m_last_keys, m_key_latch);
    }
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
    play_rumble();
}

void Input::set_motors(int port, const MotorCommand& command) {
    if (port < 0 || port >= kPadPorts) {
        return;
    }
    m_motors[static_cast<std::size_t>(port)] = {command, SDL_GetTicksNS()};
}

SDL_Haptic* Input::device_vibrator() {
    if (m_vibrator_tried) {
        return m_vibrator;
    }
    m_vibrator_tried = true;
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) {
        log::info("rumble: no haptic devices ({})", SDL_GetError());
        return nullptr;
    }
    // The first haptic device that is not a controller's: a phone's or a tablet's own vibrator.
    int count = 0;
    SDL_HapticID* ids = SDL_GetHaptics(&count);
    for (int i = 0; ids != nullptr && i < count && m_vibrator == nullptr; ++i) {
        SDL_Haptic* h = SDL_OpenHaptic(ids[i]);
        if (h == nullptr) {
            continue;
        }
        const char* name = SDL_GetHapticName(h);
        if (SDL_InitHapticRumble(h)) {
            log::info("rumble: the device's vibrator ({})", name != nullptr ? name : "unnamed");
            m_vibrator = h;
        } else {
            SDL_CloseHaptic(h);
        }
    }
    SDL_free(ids);
    return m_vibrator;
}

void Input::play_rumble() {
    const std::uint64_t now = SDL_GetTicksNS();
    const float seconds =
        m_rumble_ns != 0 ? static_cast<float>(now - m_rumble_ns) / 1'000'000'000.0f : 0.0f;
    m_rumble_ns = now;

    // Where each port plays: its gamepad, or for player 1 on touch controls, the phone. When a
    // port's device changes, it starts from rest and the old pad stops, unless another port
    // uses it now.
    std::array<SDL_Gamepad*, kPadPorts> now_on{};
    std::array<bool, kPadPorts> vibrator_on{};
    for (int port = 0; port < kPadPorts; ++port) {
        const auto p = static_cast<std::size_t>(port);
        const RumbleTuning& tuning = m_bindings[p].tuning.rumble;
        now_on[p] = gamepad(port);
        vibrator_on[p] = now_on[p] == nullptr && port == 0 && tuning.phone && m_touch.visible()
                         && tuning.strength > 0;
    }
    for (std::size_t p = 0; p < m_rumble.size(); ++p) {
        Rumble& r = m_rumble[p];
        if (r.gamepad == now_on[p] && r.vibrator == vibrator_on[p]) {
            continue;
        }
        const bool kept = std::find(now_on.begin(), now_on.end(), r.gamepad) != now_on.end();
        if (r.gamepad != nullptr && !kept) {
            SDL_RumbleGamepad(r.gamepad, 0, 0, 0);
            SDL_RumbleGamepadTriggers(r.gamepad, 0, 0, 0);
        }
        if (r.vibrator && m_vibrator != nullptr) {
            SDL_StopHapticRumble(m_vibrator);
        }
        r = Rumble{};
        r.gamepad = now_on[p];
        r.vibrator = vibrator_on[p];
    }

    for (int port = 0; port < kPadPorts; ++port) {
        const auto p = static_cast<std::size_t>(port);
        const RumbleTuning& tuning = m_bindings[p].tuning.rumble;
        const Motors& motors = m_motors[p];
        const MotorCommand command =
            motors.ns != 0 && now - motors.ns < kMotorHoldNs ? motors.command : MotorCommand{};
        Rumble& r = m_rumble[p];
        SDL_Gamepad* g = r.gamepad;
        const bool vibrator = r.vibrator;
        const RumbleOutput out = r.shaper.step(command, seconds, tuning);
        const bool renew = !out.silent() && now - r.sent_ns >= kRumbleRenewNs;
        if (!rumble_changed(out, r.sent) && !renew) {
            continue;
        }
        if (g != nullptr) {
            SDL_RumbleGamepad(g, rumble_word(out.low), rumble_word(out.high), kRumbleMs);
            // Only pads with motors in their triggers take it; the others answer false.
            const bool triggers_were = r.sent.left_trigger > 0.0f || r.sent.right_trigger > 0.0f;
            if (tuning.triggers || triggers_were) {
                SDL_RumbleGamepadTriggers(
                    g, rumble_word(out.left_trigger), rumble_word(out.right_trigger), kRumbleMs
                );
            }
        } else if (vibrator) {
            // One motor. Sent in steps of 1/8, since each command restarts a phone's vibrator.
            const float strength = std::ceil(std::max(out.low, out.high * 0.8f) * 8.0f) / 8.0f;
            if (strength == r.vibrator_sent && !renew) {
                r.sent = out;
                continue;
            }
            if (SDL_Haptic* h = device_vibrator(); h != nullptr) {
                if (strength <= 0.0f) {
                    SDL_StopHapticRumble(h);
                } else {
                    SDL_PlayHapticRumble(h, strength, kRumbleMs);
                }
            }
            r.vibrator_sent = strength;
        }
        r.sent = out;
        r.sent_ns = now;
    }
}

void Input::stop_rumble() {
    for (Rumble& r : m_rumble) {
        if (r.gamepad != nullptr) {
            SDL_RumbleGamepad(r.gamepad, 0, 0, 0);
            SDL_RumbleGamepadTriggers(r.gamepad, 0, 0, 0);
        }
        r = Rumble{};
    }
    if (m_vibrator != nullptr) {
        SDL_StopHapticRumble(m_vibrator);
    }
    m_motors = {};
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
        // Fresh state each frame.
        pad = Pad{};

        SDL_Gamepad* g = gamepad(port);
        const bool keyboard_here = m_assignment.keyboard == port;
        pad.connected = port == 0 || g != nullptr || keyboard_here;
        // The controllers stand still while another window has the keyboard, if the player asked.
        // While the controls window captures a press, every pad reads as nothing held.
        const bool read = g != nullptr && !m_capturing
                          && (m_bindings[static_cast<std::size_t>(port)].tuning.background
                              || SDL_GetKeyboardFocus() != nullptr);
        if (read) {
            PadInputs inputs = read_gamepad(g);
            // A press already let go, which the game has not read yet, reads as held.
            if (const auto it = std::find_if(
                    m_gamepads.begin(),
                    m_gamepads.end(),
                    [g](const Connected& c) { return c.gamepad == g; }
                );
                it != m_gamepads.end()) {
                for (std::size_t b = 0; b < it->tapped.size(); ++b) {
                    if (it->tapped[b] != 0) {
                        inputs.buttons[b] = true;
                        --it->tapped[b];
                    }
                }
            }
            // A pad labelled Nintendo's way (A east), read by its labels: A is PadSouth.
            if (m_bindings[static_cast<std::size_t>(port)].tuning.face_labels
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
                inputs,
                m_bindings[static_cast<std::size_t>(port)].tuning.trigger_threshold,
                m_latch[static_cast<std::size_t>(port)]
            );
            apply_pad(m_bindings[static_cast<std::size_t>(port)], inputs, gates, pad, m_dead_zone);
        } else {
            // Gone, or not read: nothing of it is held, so nothing is held back either.
            m_last[static_cast<std::size_t>(port)] = {};
            m_latch[static_cast<std::size_t>(port)] = {};
        }
        if (keyboard_here && m_keyboard_table && !m_capturing) {
            apply_keys(m_bindings[static_cast<std::size_t>(port)], keys, pad);
        }
        if (port == 0) {
            m_touch.idle(SDL_GetTicks());
            m_touch.apply(pad, m_dead_zone);
        }
        if (m_menu_chord_on) {
            pad.buttons =
                m_menu_chord[static_cast<std::size_t>(port)].filter(pad.buttons, SDL_GetTicksNS());
        }
    }
}

}  // namespace openrac::platform
