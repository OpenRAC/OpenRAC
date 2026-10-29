// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the player bound each pad button to (platform/bindings.h): the names, and what a button
// presses from once a settings file has been read.
#include <map>
#include <string>

#include "platform/bindings.h"
#include "tests/check.h"

namespace {

using openrac::platform::Binding;
using openrac::platform::Button;

bool has_key(Button button, SDL_Scancode key) {
    const auto& b = openrac::platform::bound(button);
    for (int i = 0; i < b.key_count; ++i) {
        if (b.keys[i] == key) {
            return true;
        }
    }
    return false;
}

bool has_pad(Button button, SDL_GamepadButton pad) {
    const auto& b = openrac::platform::bound(button);
    for (int i = 0; i < b.pad_button_count; ++i) {
        if (b.pad_buttons[i] == pad) {
            return true;
        }
    }
    return false;
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
    openrac::platform::set_bindings({});
    CHECK(has_key(Button::Cross, SDL_SCANCODE_SPACE));
    CHECK(has_key(Button::Square, SDL_SCANCODE_F));
    CHECK(has_key(Button::Start, SDL_SCANCODE_RETURN));
    CHECK(has_pad(Button::Cross, SDL_GAMEPAD_BUTTON_SOUTH));
    CHECK(has_pad(Button::Square, SDL_GAMEPAD_BUTTON_WEST));
    CHECK(openrac::platform::bound(Button::L2).trigger == SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    CHECK(openrac::platform::bound(Button::Cross).trigger == SDL_GAMEPAD_AXIS_INVALID);
}

// A binding replaces the default of its own kind only, and an empty one keeps both: a cleared
// binding can never leave a player unable to pause.
void test_a_binding_replaces_its_own_kind() {
    openrac::platform::set_bindings({
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
    openrac::platform::set_bindings({
        {"bind_r1", "KeyQ,KeyE"},
        {"bind_cross", "PadRightTrigger"},
    });
    CHECK(has_key(Button::R1, SDL_SCANCODE_Q));
    CHECK(has_key(Button::R1, SDL_SCANCODE_E));
    CHECK(openrac::platform::bound(Button::Cross).trigger == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    // The next read starts from the defaults again, not from what was set before.
    openrac::platform::set_bindings({});
    CHECK(!has_key(Button::R1, SDL_SCANCODE_E));
    CHECK(openrac::platform::bound(Button::Cross).trigger == SDL_GAMEPAD_AXIS_INVALID);
}

}  // namespace

int main() {
    test_names();
    test_keys_name_their_buttons();
    test_defaults();
    test_a_binding_replaces_its_own_kind();
    test_two_of_a_kind_and_a_trigger();
    return openrac::test::result();
}
