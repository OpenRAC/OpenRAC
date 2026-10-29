// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the player bound each pad button to (platform/bindings.h): the names, what a button
// presses from once a settings file has been read, and the pad pressed from made-up keys and
// gamepads through that table.
#include <array>
#include <map>
#include <memory>
#include <string>

#include "platform/bindings.h"
#include "tests/check.h"

namespace {

using openrac::platform::Binding;
using openrac::platform::Bindings;
using openrac::platform::Button;
using openrac::platform::Pad;
using openrac::platform::PadInputs;
using openrac::platform::StickGate;

// The bindings the tests below read: what the last compile_bindings gave.
Bindings g_bindings = openrac::platform::default_bindings();

void set_bindings(const std::map<std::string, std::string, std::less<>>& bindings) {
    g_bindings = openrac::platform::compile_bindings(bindings);
}

bool has_key(Button button, SDL_Scancode key) {
    const Bindings::Slot& slot = g_bindings.of(button);
    for (int i = 0; i < slot.key_count; ++i) {
        if (slot.keys[static_cast<std::size_t>(i)] == key) {
            return true;
        }
    }
    return false;
}

bool has_pad_input(Button button, Binding::Kind kind, int value) {
    const Bindings::Slot& slot = g_bindings.of(button);
    for (int i = 0; i < slot.pad_count; ++i) {
        const Binding& b = slot.pad[static_cast<std::size_t>(i)];
        if (b.kind == kind && b.value == value) {
            return true;
        }
    }
    return false;
}

bool has_pad(Button button, SDL_GamepadButton pad) {
    return has_pad_input(button, Binding::Kind::PadButton, static_cast<int>(pad));
}

bool has_trigger(Button button, SDL_GamepadAxis axis) {
    return has_pad_input(button, Binding::Kind::PadTrigger, static_cast<int>(axis));
}

// The pad as one gamepad presses it through `bindings`, from nothing held.
Pad pad_from(const Bindings& bindings, const PadInputs& inputs) {
    Pad pad;
    pad.connected = true;
    std::array<StickGate, 2> gates{};
    openrac::platform::apply_pad(bindings, inputs, gates, pad);
    return pad;
}

// A key's name comes from the SDK's table, whose value is the HID usage an SDL scancode is; the
// pad's names are the port's own. Anything else is no binding.
void test_names() {
    const Binding w = openrac::platform::parse_binding("KeyW");
    CHECK(w.kind == Binding::Kind::Key);
    CHECK(static_cast<SDL_Scancode>(w.value) == SDL_SCANCODE_W);

    const Binding space = openrac::platform::parse_binding("Space");
    CHECK(space.kind == Binding::Kind::Key);
    CHECK(static_cast<SDL_Scancode>(space.value) == SDL_SCANCODE_SPACE);

    const Binding south = openrac::platform::parse_binding("PadSouth");
    CHECK(south.kind == Binding::Kind::PadButton);
    CHECK(static_cast<SDL_GamepadButton>(south.value) == SDL_GAMEPAD_BUTTON_SOUTH);

    const Binding trigger = openrac::platform::parse_binding("PadLeftTrigger");
    CHECK(trigger.kind == Binding::Kind::PadTrigger);
    CHECK(static_cast<SDL_GamepadAxis>(trigger.value) == SDL_GAMEPAD_AXIS_LEFT_TRIGGER);

    CHECK(openrac::platform::parse_binding("").kind == Binding::Kind::None);
    CHECK(openrac::platform::parse_binding("Key").kind == Binding::Kind::None);
    CHECK(openrac::platform::parse_binding("PadNowhere").kind == Binding::Kind::None);
    // MouseLeft is in the SDK's table, but a pad button is not pressed with the mouse.
    CHECK(openrac::platform::parse_binding("MouseLeft").kind == Binding::Kind::None);

    // Every name reads back as itself.
    CHECK(std::string(openrac::platform::binding_name(w)) == "KeyW");
    CHECK(std::string(openrac::platform::binding_name(south)) == "PadSouth");
    CHECK(std::string(openrac::platform::binding_name(trigger)) == "PadLeftTrigger");
}

// Each of the sixteen keys names its button, and nothing else does.
void test_keys_name_their_buttons() {
    const char* const* keys = openrac::platform::binding_keys();
    int count = 0;
    for (; keys[count] != nullptr; ++count) {
        Button button{};
        CHECK(openrac::platform::button_of_key(keys[count], button));
    }
    CHECK(count == 16);
    Button ignored{};
    CHECK(!openrac::platform::button_of_key("bind_nothing", ignored));
    CHECK(!openrac::platform::button_of_key("game_language", ignored));
}

// With no bindings at all, every button is the port's own default.
void test_defaults() {
    set_bindings({});
    CHECK(has_key(Button::Cross, SDL_SCANCODE_SPACE));
    CHECK(has_key(Button::Square, SDL_SCANCODE_F));
    CHECK(has_key(Button::Start, SDL_SCANCODE_RETURN));
    CHECK(has_pad(Button::Cross, SDL_GAMEPAD_BUTTON_SOUTH));
    CHECK(has_pad(Button::Square, SDL_GAMEPAD_BUTTON_WEST));
    CHECK(has_trigger(Button::L2, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    CHECK(!has_trigger(Button::Cross, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
}

// A binding replaces the default of its own kind only, and an empty one keeps both: a cleared
// binding can never leave a player unable to pause.
void test_a_binding_replaces_its_own_kind() {
    set_bindings({
        {"bind_cross", "KeyJ"},            // the key only: the pad keeps PadSouth
        {"bind_square", "PadNorth"},       // the pad only: the key keeps F
        {"bind_circle", "KeyK, PadEast"},  // both, spaces around a name allowed
        {"bind_start", ""},                // nothing: both defaults stay
        {"bind_triangle", "KeyNowhere"},   // no name of either table: both defaults stay
    });

    CHECK(has_key(Button::Cross, SDL_SCANCODE_J));
    CHECK(!has_key(Button::Cross, SDL_SCANCODE_SPACE));
    CHECK(has_pad(Button::Cross, SDL_GAMEPAD_BUTTON_SOUTH));

    CHECK(has_pad(Button::Square, SDL_GAMEPAD_BUTTON_NORTH));
    CHECK(!has_pad(Button::Square, SDL_GAMEPAD_BUTTON_WEST));
    CHECK(has_key(Button::Square, SDL_SCANCODE_F));

    CHECK(has_key(Button::Circle, SDL_SCANCODE_K));
    CHECK(has_pad(Button::Circle, SDL_GAMEPAD_BUTTON_EAST));

    CHECK(has_key(Button::Start, SDL_SCANCODE_RETURN));
    CHECK(has_pad(Button::Start, SDL_GAMEPAD_BUTTON_START));

    CHECK(has_key(Button::Triangle, SDL_SCANCODE_R));
    CHECK(has_pad(Button::Triangle, SDL_GAMEPAD_BUTTON_NORTH));
}

// Two keys on one button, and a trigger bound to a button that had none.
void test_two_of_a_kind_and_a_trigger() {
    set_bindings({
        {"bind_r1", "KeyQ,KeyE"},
        {"bind_cross", "PadRightTrigger"},
    });
    CHECK(has_key(Button::R1, SDL_SCANCODE_Q));
    CHECK(has_key(Button::R1, SDL_SCANCODE_E));
    CHECK(has_trigger(Button::Cross, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    // The next read starts from the defaults again, not from what was set before.
    set_bindings({});
    CHECK(!has_key(Button::R1, SDL_SCANCODE_E));
    CHECK(!has_trigger(Button::Cross, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
}

// The defaults are the tables the port had before any binding could be set: every key and pad
// button of them presses the button it always did, the triggers press L2 and R2 from an eighth of
// their travel with their pressure, and the sticks are the gamepad's, squared.
void test_defaults_press_as_before() {
    const Bindings defaults = openrac::platform::default_bindings();

    struct KeyCase {
        SDL_Scancode key;
        Button button;
    };

    constexpr KeyCase kKeys[] = {
        {SDL_SCANCODE_UP, Button::Up},
        {SDL_SCANCODE_DOWN, Button::Down},
        {SDL_SCANCODE_LEFT, Button::Left},
        {SDL_SCANCODE_RIGHT, Button::Right},
        {SDL_SCANCODE_SPACE, Button::Cross},
        {SDL_SCANCODE_F, Button::Square},
        {SDL_SCANCODE_E, Button::Circle},
        {SDL_SCANCODE_R, Button::Triangle},
        {SDL_SCANCODE_LSHIFT, Button::L1},
        {SDL_SCANCODE_Q, Button::R1},
        {SDL_SCANCODE_1, Button::L2},
        {SDL_SCANCODE_3, Button::R2},
        {SDL_SCANCODE_C, Button::L3},
        {SDL_SCANCODE_V, Button::R3},
        {SDL_SCANCODE_RETURN, Button::Start},
        {SDL_SCANCODE_ESCAPE, Button::Start},
        {SDL_SCANCODE_BACKSPACE, Button::Select},
    };
    for (const KeyCase& c : kKeys) {
        auto keys = std::make_unique<bool[]>(SDL_SCANCODE_COUNT);
        keys[c.key] = true;
        Pad pad;
        openrac::platform::apply_keys(
            defaults, std::span<const bool>(keys.get(), SDL_SCANCODE_COUNT), pad
        );
        CHECK(pad.held(c.button));
        CHECK(pad.buttons == static_cast<std::uint16_t>(~static_cast<std::uint16_t>(c.button)));
    }

    struct PadCase {
        SDL_GamepadButton pad;
        Button button;
    };

    constexpr PadCase kPads[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, Button::Cross},
        {SDL_GAMEPAD_BUTTON_EAST, Button::Circle},
        {SDL_GAMEPAD_BUTTON_WEST, Button::Square},
        {SDL_GAMEPAD_BUTTON_NORTH, Button::Triangle},
        {SDL_GAMEPAD_BUTTON_BACK, Button::Select},
        {SDL_GAMEPAD_BUTTON_START, Button::Start},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, Button::L3},
        {SDL_GAMEPAD_BUTTON_RIGHT_STICK, Button::R3},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Button::L1},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Button::R1},
        {SDL_GAMEPAD_BUTTON_DPAD_UP, Button::Up},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Button::Down},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Button::Left},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Button::Right},
    };
    for (const PadCase& c : kPads) {
        PadInputs inputs;
        inputs.buttons[c.pad] = true;
        const Pad pad = pad_from(defaults, inputs);
        CHECK(pad.buttons == static_cast<std::uint16_t>(~static_cast<std::uint16_t>(c.button)));
    }
    // The buttons nothing presses by default press nothing.
    for (const SDL_GamepadButton spare :
         {SDL_GAMEPAD_BUTTON_GUIDE, SDL_GAMEPAD_BUTTON_MISC1, SDL_GAMEPAD_BUTTON_TOUCHPAD}) {
        PadInputs inputs;
        inputs.buttons[spare] = true;
        CHECK(pad_from(defaults, inputs).buttons == 0xFFFF);
    }

    PadInputs triggers;
    triggers.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = 3000;  // below an eighth: not held
    triggers.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 32767;
    Pad pad = pad_from(defaults, triggers);
    CHECK(!pad.held(Button::L2));
    CHECK(pad.held(Button::R2));
    CHECK(pad.pressure[static_cast<std::size_t>(openrac::platform::Pressure::R2)] == 0xFF);
    triggers.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = 16384;
    pad = pad_from(defaults, triggers);
    CHECK(pad.held(Button::L2));
    CHECK(pad.pressure[static_cast<std::size_t>(openrac::platform::Pressure::L2)] == 0x80);

    // The sticks as stick_bytes gives them: half way stays, a diagonal on the rim reaches out.
    PadInputs sticks;
    sticks.axes[SDL_GAMEPAD_AXIS_LEFTX] = 11585;
    sticks.axes[SDL_GAMEPAD_AXIS_LEFTY] = 11585;
    sticks.axes[SDL_GAMEPAD_AXIS_RIGHTX] = -32768;
    pad = pad_from(defaults, sticks);
    CHECK(pad.left_x == 0xAD && pad.left_y == 0xAD);
    CHECK(pad.right_x == 0x00 && pad.right_y == 0x80);
    CHECK(pad.buttons == 0xFFFF);
}

// A rebound pad button presses its new button and no longer its old one.
void test_a_rebound_pad_button_presses() {
    set_bindings({{"bind_cross", "PadEast"}, {"bind_circle", "PadSouth"}});
    PadInputs inputs;
    inputs.buttons[SDL_GAMEPAD_BUTTON_SOUTH] = true;
    const Pad pad = pad_from(g_bindings, inputs);
    CHECK(pad.held(Button::Circle));
    CHECK(!pad.held(Button::Cross));
}

}  // namespace

int main() {
    test_names();
    test_keys_name_their_buttons();
    test_defaults();
    test_a_binding_replaces_its_own_kind();
    test_two_of_a_kind_and_a_trigger();
    test_defaults_press_as_before();
    test_a_rebound_pad_button_presses();
    return openrac::test::result();
}
