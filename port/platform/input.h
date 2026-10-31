// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The keyboard and SDL's gamepads, turned into the two DualShock 2 ports the
// game reads (pad.h). By default port 0 is the first gamepad, or the keyboard
// while there is none, and port 1 the second gamepad; the players' controllers
// and the keyboard can be chosen (players.h). A gamepad counts from its first
// press, ahead of those not pressed yet (a dock's or a keyboard's device that SDL
// also lists as one), and one pressed while port 0's has been left alone for two
// seconds takes port 0 (the player picked up another controller). Gamepad buttons
// are read by position, so the bottom face button is Cross on any controller
// unless the player binds it otherwise.
//
// Default keyboard (by key position, whatever the layout):
//
//   left stick  W A S D        right stick  I J K L
//   d-pad       arrow keys     Cross Space, Square F, Circle E, Triangle R
//   L1 Left Shift, R1 Q, L2 1, R2 3, L3 C, R3 V, Start Return, Select Backspace
//
// What presses each control is the player's controls (bindings.h, docs/port/CONTROLS.md), the
// defaults above included. A touch screen gets an on-screen pad (touch_pad.h), always on port 0. A mod may take the keyboard (mod interface 1.2,
// openrac_api.take_input): the table then stops, and the mod presses pad 0's buttons itself from
// the keyboard and mouse state below (KeyboardMouse), which is read for it every frame.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "platform/bindings.h"
#include "platform/pad.h"
#include "platform/players.h"
#include "platform/touch_pad.h"

union SDL_Event;
struct SDL_Gamepad;

namespace openrac::platform {

inline constexpr int kPadPorts = 2;
inline constexpr std::size_t kKeyCount = 512;  // SDL scancodes, which are USB HID usages

// The keyboard and mouse for the tick about to run, as a mod reads them (openrac_mod.h's
// openrac_input has the same meaning, field by field).
struct KeyboardMouse {
    std::uint32_t tick = 0;
    std::array<std::uint8_t, kKeyCount> keys{};  // bit 0 held, bit 1 went down since the last tick
    std::uint32_t mouse_held = 0;                // SDL_BUTTON_*MASK: left 1, middle 2, right 4, x1 8, x2 16
    std::uint32_t mouse_pressed = 0;
    float mouse_dx = 0.0f, mouse_dy = 0.0f;
    float wheel_x = 0.0f, wheel_y = 0.0f;  // notches; y > 0 away from the player
    bool focused = false;
    bool captured = false;
};

// A controller connected now, as the controls window and the launcher name it.
struct Device {
    std::string id;    // stable from one run to the next (players.h)
    std::string name;  // SDL's name for it ("DualSense Wireless Controller")
    int player = -1;   // the player it is, 0 or 1; -1 none
};

// Apply the port's default keyboard table to a pad: `keys` is indexed by SDL scancode, as
// SDL_GetKeyboardState returns it. A stick key overrides the stick's axis.
void apply_keyboard(Pad& pad, std::span<const bool> keys);

static_assert(kPlayers == kPadPorts);

class Input {
public:
    Input() = default;
    ~Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    // Every SDL event goes through here (gamepads arriving and leaving).
    void handle_event(const SDL_Event& event);

    // Read the keyboard and gamepads into the pads; once per game frame,
    // after the events.
    void update();

    // Read the pads again from what SDL holds now, just before the game's tick reads them: the
    // gamepads' state brought up to date, the keyboard's and the on-screen pad's as the last
    // events left them. The mods' keyboard and mouse and the motors stay update()'s.
    void refresh_pads();

    const Pad& pad(int port) const { return m_pads[static_cast<std::size_t>(port)]; }

    // The game sets the motors here (scePadSetActDirect); update() plays them.
    Pad& pad(int port) { return m_pads[static_cast<std::size_t>(port)]; }

    // The keyboard and mouse as update() last read them, for the mods.
    const KeyboardMouse& keyboard_mouse() const { return m_km; }

    // What each player's keys and gamepad press from now on (bindings.h), by player: the next read
    // uses them. What is held on a player's controller as theirs change, and the keys held as the
    // keyboard's player's change, are held back until let go (bindings.h's Latch).
    void set_bindings(const std::array<Bindings, kPadPorts>& bindings);

    const Bindings& bindings(int port) const { return m_bindings[static_cast<std::size_t>(port)]; }

    // Which controller each player reads (players.h): applied at once, and again whenever a
    // controller connects or leaves.
    void set_players(const Players& players);

    const Players& players() const { return m_players; }

    // The controllers connected, in the order they were, with the player each is.
    const std::vector<Device>& devices() const { return m_devices; }

    // Says whether a controller connected or left, or the players changed, since the last call.
    bool take_devices_changed();

    // What controller `index` of devices() holds now, read from SDL (the controls window's live
    // view); nothing held for one that is gone.
    PadInputs read_device(std::size_t index) const;

    // The controls window's "press a button": from begin_capture() every player's pad reads as
    // nothing held, and the first pad input pressed on any controller (a button, a trigger past
    // its threshold, a stick past most of the way) that was not already held is kept for
    // take_captured(). end_capture() lets the pads go again.
    void begin_capture();
    void end_capture();

    bool capturing() const { return m_capturing; }

    std::optional<Binding> take_captured() { return std::exchange(m_captured, std::nullopt); }

    // The player the keyboard is, -1 none.
    int keyboard_player() const { return m_assignment.keyboard; }

    // False: the keyboard table no longer presses its player's buttons (a mod took the keyboard).
    void set_keyboard_table(bool on) { m_keyboard_table = on; }

    // Whether the cursor is captured now (the frontend sets it: SDL's relative mouse mode).
    void set_captured(bool captured) { m_km.captured = captured; }

    // The on-screen pad, which adds to port 0 (touch_pad.h). The frontend tells it the window's
    // shape and draws it while it is visible.
    TouchPad& touch_pad() { return m_touch; }

private:
    SDL_Gamepad* gamepad(int port) const;
    struct Connected {
        SDL_Gamepad* gamepad = nullptr;
        std::uint64_t active_ns = 0;  // SDL's time of its last press or stick pushed well out
    };
    // How long port 0's gamepad is left alone before another one pressed takes the port.
    static constexpr std::uint64_t kHandOverNs = 2'000'000'000;

    std::vector<Connected>::iterator find(std::uint32_t which);  // SDL_JoystickID
    // A gamepad's button pressed at SDL's time `ns`: the gamepad may move up the ports.
    void pressed(std::uint32_t which, std::uint64_t ns);
    // Moves the gamepad at from to to, its device with it (the others keep their order).
    void move_gamepad(std::size_t from, std::size_t to);

    // Works out again which controller each player reads, and the devices' players.
    void assign_players();

    // One read of the capture: the first pad input pressed that was not held.
    void capture_step();

    std::array<Pad, kPadPorts> m_pads{};
    // The gamepads in port order: first those used so far (the first m_used), then the others in
    // the order they came. A gamepad pressed takes port 0 when port 0's has been left alone for
    // kHandOverNs (or was never used).
    std::vector<Connected> m_gamepads;  // as m_devices, index for index
    std::size_t m_used = 0;
    std::vector<Device> m_devices;
    Players m_players = default_players();
    Assignment m_assignment = assign(default_players(), {});
    bool m_devices_changed = true;
    // The capture: under way, what each controller held when it began (or since, until let go),
    // and what was pressed.
    bool m_capturing = false;
    std::vector<std::vector<Binding>> m_capture_held;
    std::optional<Binding> m_captured;
    // Each port's sticks, left and right, as their gamepad has shown them (pad.h), and the gamepad
    // that showed it: another one on the port starts afresh.
    std::array<std::array<StickGate, 2>, kPadPorts> m_gates{};
    std::array<Bindings, kPadPorts> m_bindings{default_bindings(), default_bindings()};
    // What each port's gamepad and the keyboard held at the last read, and what was held back when
    // the bindings last changed.
    std::array<PadInputs, kPadPorts> m_last{};
    std::array<Latch, kPadPorts> m_latch{};
    std::array<bool, kKeyCount> m_last_keys{};
    std::array<bool, kKeyCount> m_key_latch{};
    std::array<SDL_Gamepad*, kPadPorts> m_gates_of{};
    TouchPad m_touch;
    KeyboardMouse m_km;
    bool m_keyboard_table = true;
    // Gathered from the events between two update()s.
    std::array<std::uint8_t, kKeyCount> m_went_down{};
    std::uint32_t m_mouse_went_down = 0;
    float m_dx = 0.0f, m_dy = 0.0f, m_wheel_x = 0.0f, m_wheel_y = 0.0f;
};

}  // namespace openrac::platform
