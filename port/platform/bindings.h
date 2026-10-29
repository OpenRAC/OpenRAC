// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the player bound each of the pad's controls to: the names the settings file and the
// launcher use, and the table (Bindings) input.cpp presses the pad from.
//
// The controls are the sixteen buttons and the two sticks' four directions each ("targets"). A
// binding is one key or one pad input. Keys are named as the launcher, a mod's keybind setting
// and the SDK already name them (sdk/include/openrac/keys.h: "KeyW" is the key left of E on any
// layout, "Space", "ShiftLeft"), and a name's value there is its USB HID usage, which is what an
// SDL scancode is. Pad inputs are the port's own names ("PadSouth", "PadLeftTrigger",
// "PadLeftStickUp"): they are not in the SDK's table, since a mod reading a keybind setting cannot
// use one. Two pad inputs joined by "+" are a chord: the second presses only while the first is
// held ("PadLeftShoulder+PadSouth"). "none" binds nothing of either kind.
//
// Keys press buttons only: the sticks' keys are the port's own (input.h), until keyboard
// remapping comes. A target with no binding of a kind keeps the port's default for it (the
// tables in bindings.cpp), so an empty or misspelt value can never leave a player unable to pause,
// and Escape is Start whatever Start is bound to.
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
        PadTrigger,
        PadAxis,  // one direction of a stick's axis
    };
    Kind kind = Kind::None;
    // Kind::Key: an SDL scancode. Kind::PadButton: an SDL_GamepadButton. Kind::PadTrigger and
    // Kind::PadAxis: an SDL_GamepadAxis (a trigger counts as held above kTriggerThreshold).
    int value = 0;
    // Kind::PadAxis: the direction, -1 (left or up) or +1 (right or down).
    int sign = 0;

    bool operator==(const Binding&) const = default;
};

// `name` as the settings file writes it ("KeyW", "PadSouth"); Kind::None for one no table has.
Binding parse_binding(std::string_view name);

// A binding's name ("KeyW", "PadSouth"), or "" for one the port has no name for.
const char* binding_name(const Binding& binding);

// The controls a binding presses: the sixteen buttons by their bit (Select is 0, Square 15, as
// pad.h's Button), then each stick's four directions.
enum class Target : std::uint8_t {
    LeftStickUp = 16,
    LeftStickDown,
    LeftStickLeft,
    LeftStickRight,
    RightStickUp,
    RightStickDown,
    RightStickLeft,
    RightStickRight,
};
inline constexpr int kTargetCount = 24;

// The target of a button.
int target_of(Button button);

// Says whether target `target` is one of the sixteen buttons (else a stick's direction).
inline bool is_button(int target) {
    return target < 16;
}

// The target a settings key names ("bind_cross", "bind_lstick_up"), or -1 for a key that is not one
// of them. The prefix every binding key has is kBindPrefix.
inline constexpr std::string_view kBindPrefix = "bind_";
int target_of_key(std::string_view key);

// The button a settings key names ("bind_cross" -> Button::Cross), or false for a key that is not
// one of the sixteen buttons'.
bool button_of_key(std::string_view key, Button& button);

// Every target's settings key, in the order the launcher shows them, ended by nullptr.
const char* const* binding_keys();

// The settings key of target `target` ("bind_cross").
const char* key_of_target(int target);

// A pad input that presses, alone or as a chord: `input` presses while `with` is held too (`with`
// Kind::None when it is no chord).
struct PadChord {
    Binding input;
    Binding with;

    bool operator==(const PadChord&) const = default;
};

// The most of a kind (keys, or pad inputs) one target holds.
inline constexpr int kMostOfAKind = 4;

// The value that binds nothing of either kind.
inline constexpr std::string_view kUnbound = "none";

// What the pad is pressed from: for each target, the keys and the pad inputs that press it.
struct Bindings {
    struct Slot {
        std::array<SDL_Scancode, kMostOfAKind> keys{};
        int key_count = 0;
        std::array<PadChord, kMostOfAKind> pad{};
        int pad_count = 0;
    };

    std::array<Slot, kTargetCount> slots{};

    const Slot& of(Button button) const {
        return slots[static_cast<std::size_t>(target_of(button))];
    }
};

// The port's own tables: what every target is pressed from when the player bound nothing.
Bindings default_bindings();

// One target's value from the settings file, read into `slot`: a kind the value names replaces
// that kind, a kind it does not name is left as it was, and "none" empties both. A name no table
// has, a key on a stick's direction and a chord of anything but two pad inputs are reported in the
// log under `key` and left out.
void read_binding(std::string_view key, std::string_view value, int target, Bindings::Slot& slot);

// A target's slot as the settings file writes it: its keys, then its pad inputs, by name, comma
// separated; "none" for an empty slot.
std::string binding_value(const Bindings::Slot& slot);

// The player's bindings, as the settings file holds them (settings.h's Settings::bindings), read
// over the defaults with read_binding.
Bindings compile_bindings(const std::map<std::string, std::string, std::less<>>& bindings);

// One gamepad as SDL last reported it: every button, and every axis in SDL's range (sticks
// -32768..32767, triggers 0..32767), by SDL's numbers.
struct PadInputs {
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
    std::array<std::int16_t, SDL_GAMEPAD_AXIS_COUNT> axes{};
};

// A trigger counts as a button held above about an eighth of its travel; its pressure byte follows
// the trigger all the way. A stick's direction bound to a button presses it from half way.
inline constexpr std::uint8_t kTriggerThreshold = 0x20;
inline constexpr float kStickPress = 0.5f;

// Presses `pad`'s buttons from the keys held (`keys`, by SDL scancode, as SDL_GetKeyboardState
// gives them), and moves its sticks from the port's stick keys (input.h). Escape is Start, whatever
// Start is bound to.
void apply_keys(const Bindings& bindings, std::span<const bool> keys, Pad& pad);

// Presses `pad`'s buttons and sets its sticks from one gamepad. The gamepad's sticks are shaped as
// the DualShock 2's are (pad.h) before anything reads them: `gates` is what its left and right
// sticks have shown. A chord that presses takes its input from the bindings of that input alone, so
// one press does not do both.
void apply_pad(
    const Bindings& bindings, const PadInputs& inputs, std::array<StickGate, 2>& gates, Pad& pad
);

}  // namespace openrac::platform
