// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the player bound each of the pad's sixteen buttons to: the names the settings file and the
// launcher use, and the table (Bindings) input.cpp presses the pad from.
//
// A binding is one key or one pad input. Keys are named as the launcher, a mod's keybind setting
// and the SDK already name them (sdk/include/openrac/keys.h: "KeyW" is the key left of E on any
// layout, "Space", "ShiftLeft"), and a name's value there is its USB HID usage, which is what an
// SDL scancode is. Pad inputs are the port's own names ("PadSouth", "PadLeftTrigger"): they are
// not in the SDK's table, since a mod reading a keybind setting cannot use one.
//
// A button with no binding of a kind keeps the port's default for it (the tables in bindings.cpp),
// so clearing a binding can never leave a player unable to pause.
//
// The table is read once when the bindings change (compile_bindings) and kept by its user (Input):
// pressing the pad from it each frame (apply_keys, apply_pad) parses nothing and calls nothing of
// SDL's, so the tests drive it with made-up keys and gamepads.

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "platform/pad.h"

namespace openrac::platform {

// One name read from the settings file.
struct Binding {
    enum class Kind {
        None,
        Key,
        PadButton,
        PadTrigger
    };
    Kind kind = Kind::None;
    // Kind::Key: an SDL scancode. Kind::PadButton: an SDL_GamepadButton. Kind::PadTrigger: an
    // SDL_GamepadAxis (the trigger counts as held above input.cpp's threshold).
    int value = 0;
};

// `name` as the settings file writes it ("KeyW", "PadSouth"); Kind::None for one no table has.
Binding parse_binding(std::string_view name);

// A pad input's name, or "" for one the port has no name for: for the log and the launcher.
const char* binding_name(const Binding& binding);

// The button a settings key names ("bind_cross" -> Button::Cross), or nothing for a key that is
// not one of the sixteen. The prefix every binding key has is kBindPrefix.
inline constexpr std::string_view kBindPrefix = "bind_";
bool button_of_key(std::string_view key, Button& button);

// Every button's settings key, in the order the launcher shows them, ended by nullptr.
const char* const* binding_keys();

// The most of a kind (keys, or pad inputs) one button holds: two is what the launcher offers (a key
// and a spare).
inline constexpr int kMostOfAKind = 2;

// What the pad is pressed from: for each button, the keys and the pad inputs that press it.
struct Bindings {
    struct Slot {
        std::array<SDL_Scancode, kMostOfAKind> keys{};
        int key_count = 0;
        std::array<Binding, kMostOfAKind> pad{};  // Kind::PadButton or Kind::PadTrigger
        int pad_count = 0;
    };

    // By the button's bit: Select is slot 0, Square slot 15 (pad.h's Button).
    std::array<Slot, 16> slots{};

    const Slot& of(Button button) const;
};

// The port's own tables: what every button is pressed from when the player bound nothing.
Bindings default_bindings();

// The player's bindings, as the settings file holds them (settings.h's Settings::bindings): the
// values are read with parse_binding, and a button with none of a kind keeps the port's default of
// that kind. A name no table has is reported in the log and left out.
Bindings compile_bindings(const std::map<std::string, std::string, std::less<>>& bindings);

// One gamepad as SDL last reported it: every button, and every axis in SDL's range (sticks
// -32768..32767, triggers 0..32767), by SDL's numbers.
struct PadInputs {
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
    std::array<std::int16_t, SDL_GAMEPAD_AXIS_COUNT> axes{};
};

// A trigger counts as a button held above about an eighth of its travel; its pressure byte follows
// the trigger all the way.
inline constexpr std::uint8_t kTriggerThreshold = 0x20;

// Presses `pad`'s buttons from the keys held (`keys`, by SDL scancode, as SDL_GetKeyboardState
// gives them), and moves its sticks from the port's stick keys (input.h). Escape is Start, whatever
// Start is bound to.
void apply_keys(const Bindings& bindings, std::span<const bool> keys, Pad& pad);

// Presses `pad`'s buttons from one gamepad, and sets its sticks from the gamepad's, shaped as the
// DualShock 2's are (pad.h): `gates` is what the gamepad's left and right sticks have shown.
void apply_pad(
    const Bindings& bindings, const PadInputs& inputs, std::array<StickGate, 2>& gates, Pad& pad
);

}  // namespace openrac::platform
