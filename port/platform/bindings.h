// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the player bound each of the pad's sixteen buttons to: the names the settings file and the
// launcher use, and the table input.cpp presses the pad from.
//
// A binding is one key or one pad input. Keys are named as the launcher, a mod's keybind setting
// and the SDK already name them (sdk/include/openrac/keys.h: "KeyW" is the key left of E on any
// layout, "Space", "ShiftLeft"), and a name's value there is its USB HID usage, which is what an
// SDL scancode is. Pad inputs are the port's own names ("PadSouth", "PadLeftTrigger"): they are
// not in the SDK's table, since a mod reading a keybind setting cannot use one.
//
// A button with no binding of a kind keeps the port's default for it (input.cpp's tables), so
// clearing a binding can never leave a player unable to pause.

#pragma once

#include <map>
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

// The player's bindings, as the settings file holds them (settings.h's Settings::bindings): the
// values are read with parse_binding, and a button with none keeps the port's default. Replaces
// what was set before; an empty map is the defaults. Called when the settings are read.
void set_bindings(const std::map<std::string, std::string, std::less<>>& bindings);

// What input.cpp presses from. A binding the player set, else the port's default.
struct Bound {
    // Keys that press this button, ended by SDL_SCANCODE_UNKNOWN.
    const SDL_Scancode* keys;
    int key_count;
    // Pad buttons that press it, ended by SDL_GAMEPAD_BUTTON_INVALID.
    const SDL_GamepadButton* pad_buttons;
    int pad_button_count;
    // Pad triggers that press it (the axis over the threshold), SDL_GAMEPAD_AXIS_INVALID for none.
    SDL_GamepadAxis trigger;
};

// What `button` is bound to now.
const Bound& bound(Button button);

}  // namespace openrac::platform
