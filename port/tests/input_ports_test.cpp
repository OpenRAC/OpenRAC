// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which gamepad plays on which port (platform/input.h): a gamepad that is never pressed, found
// before the controller (a USB-C dock's, a hub's), does not keep the controller off port 0; a
// controller picked up while port 0's is left alone takes port 0; gamepads coming and going.
#include <cstdint>
#include <cstdio>

#include <SDL3/SDL.h>

#include "platform/input.h"
#include "tests/check.h"

namespace {

using openrac::platform::Bindings;
using openrac::platform::Button;
using openrac::platform::Input;
using openrac::platform::kPadPorts;

// The events' clock, in milliseconds, which the test moves itself.
std::uint64_t g_now_ms = 1000;

SDL_JoystickID attach(const char* name) {
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = name;
    return SDL_AttachVirtualJoystick(&desc);
}

// The events SDL has for the gamepads, handed to the input as the window's loop does, at the
// test's time.
void pump(Input& input) {
    SDL_UpdateJoysticks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        event.common.timestamp = g_now_ms * 1'000'000;
        input.handle_event(event);
    }
}

void later(Input& input, std::uint64_t ms) {
    g_now_ms += ms;
    pump(input);
}

void hold(Input& input, SDL_JoystickID id, SDL_GamepadButton button, bool down) {
    SDL_SetJoystickVirtualButton(SDL_GetJoystickFromID(id), button, down);
    pump(input);
}

// A press of Cross, let go at once.
void tap(Input& input, SDL_JoystickID id, SDL_GamepadButton button = SDL_GAMEPAD_BUTTON_SOUTH) {
    hold(input, id, button, true);
    hold(input, id, button, false);
}

void stick(Input& input, SDL_JoystickID id, std::int16_t x) {
    SDL_SetJoystickVirtualAxis(SDL_GetJoystickFromID(id), SDL_GAMEPAD_AXIS_LEFTX, x);
    pump(input);
}

// Which of `a` and `b` plays on `port`: its Cross held, read as the game reads the ports. 'a',
// 'b' or '-' (neither).
char on_port(Input& input, int port, SDL_JoystickID a, SDL_JoystickID b) {
    char found = '-';
    for (const SDL_JoystickID id : {a, b}) {
        SDL_Joystick* joystick = SDL_GetJoystickFromID(id);
        if (joystick == nullptr) {
            continue;
        }
        SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
        SDL_UpdateJoysticks();
        input.refresh_pads();
        if (input.pad(port).held(Button::Up)) {
            found = id == a ? 'a' : 'b';
        }
        SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
        SDL_UpdateJoysticks();
    }
    // Only the state was looked at: the presses are not the player's.
    SDL_FlushEvents(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_JOYSTICK_BUTTON_DOWN, SDL_EVENT_JOYSTICK_BUTTON_UP);
    return found;
}

// There is no window here, so nothing has the focus: the pads are read as in the background.
void play_in_background(Input& input) {
    std::array<Bindings, kPadPorts> bindings{input.bindings(0), input.bindings(1)};
    for (Bindings& b : bindings) {
        b.tuning.background = true;
    }
    input.set_bindings(bindings);
}

// By default a pad does not play while another window has the focus (pad_background = 0).
void test_unfocused() {
    Input input;
    const SDL_JoystickID a = attach("Pad");
    pump(input);
    SDL_Joystick* joystick = SDL_GetJoystickFromID(a);
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    SDL_UpdateJoysticks();
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Up));
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    SDL_DetachVirtualJoystick(a);
    pump(input);
}

// A dock's device found first and never pressed: the controller pressed plays on port 0, and the
// dock's device gets port 1 only once it is pressed.
void test_dock_first() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID dock = attach("dock");
    const SDL_JoystickID pad = attach("controller");
    pump(input);
    CHECK(on_port(input, 0, pad, dock) == 'b');  // before any press, the order they came in
    tap(input, pad);
    CHECK(on_port(input, 0, pad, dock) == 'a');
    CHECK(on_port(input, 1, pad, dock) == 'b');
    // The dock's device unplugged: port 0 keeps the controller, port 1 is empty.
    SDL_DetachVirtualJoystick(dock);
    pump(input);
    CHECK(on_port(input, 0, pad, dock) == 'a');
    input.refresh_pads();
    CHECK(!input.pad(1).connected);
    SDL_DetachVirtualJoystick(pad);
    pump(input);
}

// Two players playing at once keep their ports, whoever presses when.
void test_two_players() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("a");
    const SDL_JoystickID b = attach("b");
    pump(input);
    tap(input, b);  // the second one found pressed first: player 1
    later(input, 300);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'b');
    CHECK(on_port(input, 1, a, b) == 'a');
    for (int i = 0; i < 10; ++i) {
        later(input, 1500);
        tap(input, i % 2 == 0 ? a : b);
    }
    CHECK(on_port(input, 0, a, b) == 'b');
    CHECK(on_port(input, 1, a, b) == 'a');
    SDL_DetachVirtualJoystick(a);
    SDL_DetachVirtualJoystick(b);
    pump(input);
}

// One player putting a controller down and picking up another: the other takes port 0 once the
// first has been left alone two seconds, not while it is held.
void test_hand_over() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("a");
    const SDL_JoystickID b = attach("b");
    pump(input);
    tap(input, a);
    later(input, 1900);
    tap(input, b);
    CHECK(on_port(input, 0, a, b) == 'a');
    later(input, 2100);
    tap(input, b);
    CHECK(on_port(input, 0, a, b) == 'b');
    CHECK(on_port(input, 1, a, b) == 'a');

    // b's button held down (no more events) keeps the port.
    hold(input, b, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    later(input, 5000);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'b');
    hold(input, b, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);

    // So does its stick pushed all the way and held.
    later(input, 5000);
    stick(input, b, 32767);
    later(input, 5000);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'b');
    stick(input, b, 0);

    // A stick moved counts as playing too.
    later(input, 1500);
    stick(input, b, -32768);
    stick(input, b, 0);
    later(input, 1500);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'b');
    later(input, 2100);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'a');
    SDL_DetachVirtualJoystick(a);
    SDL_DetachVirtualJoystick(b);
    pump(input);
}

// Unplugged and plugged in again, mid-game.
void test_unplug() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("a");
    SDL_JoystickID b = attach("b");
    pump(input);
    tap(input, a);
    tap(input, b);
    input.refresh_pads();  // the game reads the two taps
    input.taps_read(0);
    input.taps_read(1);
    // Port 0's unplugged with Cross held: port 0 is b's at once, and nothing stays pressed.
    hold(input, a, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_DetachVirtualJoystick(a);
    pump(input);
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Cross));
    CHECK(on_port(input, 0, a, b) == 'b');
    CHECK(!input.pad(1).connected);
    // Plugged in again and pressed while b plays: player 2.
    const SDL_JoystickID again = attach("a");
    pump(input);
    tap(input, b);
    later(input, 500);
    tap(input, again);
    CHECK(on_port(input, 0, again, b) == 'b');
    CHECK(on_port(input, 1, again, b) == 'a');
    // Pressed once b has been left alone: player 1.
    later(input, 2500);
    tap(input, again);
    CHECK(on_port(input, 0, again, b) == 'a');
    // b unplugged and plugged in again, not pressed yet: a keeps port 0, the new one gets port 1.
    SDL_DetachVirtualJoystick(b);
    pump(input);
    b = attach("b");
    pump(input);
    CHECK(on_port(input, 0, again, b) == 'a');
    CHECK(on_port(input, 1, again, b) == 'b');
    SDL_DetachVirtualJoystick(again);
    SDL_DetachVirtualJoystick(b);
    pump(input);
}

// Three: the third one pressed, while the two players play, has no port; a dock's device never
// pressed takes none from a used one.
void test_three() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID dock = attach("dock");
    const SDL_JoystickID a = attach("a");
    const SDL_JoystickID b = attach("b");
    pump(input);
    tap(input, b);
    tap(input, a);
    CHECK(on_port(input, 0, a, b) == 'b');
    CHECK(on_port(input, 1, a, b) == 'a');
    CHECK(on_port(input, 0, dock, dock) == '-');
    CHECK(on_port(input, 1, dock, dock) == '-');
    // Port 0's unplugged: a moves up and the dock's device, never pressed, takes port 1.
    SDL_DetachVirtualJoystick(b);
    pump(input);
    CHECK(on_port(input, 0, a, dock) == 'a');
    CHECK(on_port(input, 1, a, dock) == 'b');
    // Another plugged in and pressed while a plays: port 1 is its, not the dock's device's.
    const SDL_JoystickID c = attach("c");
    pump(input);
    tap(input, a);
    tap(input, c);
    CHECK(on_port(input, 0, a, c) == 'a');
    CHECK(on_port(input, 1, a, c) == 'b');
    SDL_DetachVirtualJoystick(c);
    SDL_DetachVirtualJoystick(dock);
    SDL_DetachVirtualJoystick(a);
    pump(input);
}

// The menu waits for a press with the game paused: update() is not called, and poll_capture()
// alone must see the press. A button held as the capture began does not count.
void test_capture_while_paused() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("Pad");
    pump(input);
    SDL_Joystick* joystick = SDL_GetJoystickFromID(a);
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_UpdateJoysticks();
    input.begin_capture();
    input.poll_capture();
    CHECK(!input.take_captured().has_value());
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, true);
    input.poll_capture();
    const auto got = input.take_captured();
    CHECK(got.has_value());
    input.end_capture();
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, false);
    SDL_DetachVirtualJoystick(a);
    pump(input);
}

// A menu closed with a button: what is held then does not press in the game until let go.
void test_hold_back_held() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("Pad");
    pump(input);
    tap(input, a);         // port 0 takes it
    input.refresh_pads();  // read once, as every frame of the game does
    SDL_Joystick* joystick = SDL_GetJoystickFromID(a);
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    SDL_UpdateJoysticks();
    input.hold_back_held();
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Up));
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    SDL_UpdateJoysticks();
    input.refresh_pads();
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    SDL_UpdateJoysticks();
    input.refresh_pads();
    CHECK(input.pad(0).held(Button::Up));
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    SDL_DetachVirtualJoystick(a);
    pump(input);
}

// A press let go before the game reads the pad (a quick tap while the frames are slow) still
// reaches the game, once: read, it is over; not read, it is over a few reads later.
void test_quick_tap() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach("Pad");
    pump(input);
    tap(input, a, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    input.refresh_pads();
    input.refresh_pads();  // read again just before the tick (responsive controls)
    CHECK(input.pad(0).held(Button::Down));
    input.taps_read(0);
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Down));

    tap(input, a, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    for (int i = 0; i < 6; ++i) {
        input.refresh_pads();
    }
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Down));

    // A menu closed with a tap: not the game's.
    tap(input, a, SDL_GAMEPAD_BUTTON_EAST);
    input.hold_back_held();
    input.refresh_pads();
    CHECK(!input.pad(0).held(Button::Circle));
    SDL_DetachVirtualJoystick(a);
    pump(input);
}

// What a virtual gamepad's motors were last told.
struct Felt {
    Uint16 low = 0;
    Uint16 high = 0;
    int calls = 0;
};

Felt g_felt;

SDL_JoystickID attach_rumble(const char* name) {
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = name;
    desc.Rumble = [](void*, Uint16 low, Uint16 high) {
        g_felt.low = low;
        g_felt.high = high;
        ++g_felt.calls;
        return true;
    };
    return SDL_AttachVirtualJoystick(&desc);
}

// The game's motor commands reach the player's gamepad, stop when the game stops renewing them,
// follow the player's strength, and a pad unplugged while it shakes is let go.
void test_rumble() {
    Input input;
    play_in_background(input);
    const SDL_JoystickID a = attach_rumble("Rumble pad");
    pump(input);
    g_felt = {};
    input.set_motors(0, {true, 255});
    input.update();
    CHECK(g_felt.calls >= 1);
    CHECK(g_felt.low > 0xC000);
    CHECK(g_felt.high > 0x8000);
    CHECK(input.motors(0) == (openrac::platform::MotorCommand{true, 255}));
    // Held: the pad is not told again every frame, only when it changes or to renew it.
    input.set_motors(0, {true, 255});
    input.update();
    input.set_motors(0, {true, 255});
    input.update();
    const int steady = g_felt.calls;
    input.set_motors(0, {true, 255});
    input.update();
    CHECK(g_felt.calls <= steady + 1);
    // The game stops asking: within a few frames past the hold, the motors are at rest.
    SDL_Delay(250);
    for (int i = 0; i < 40; ++i) {
        input.update();
        SDL_Delay(16);
    }
    CHECK(g_felt.low == 0 && g_felt.high == 0);
    CHECK(input.rumble_output(0).silent());
    // Off: nothing reaches the pad.
    Bindings bindings = input.bindings(0);
    bindings.tuning.rumble.strength = 0;
    input.set_bindings({bindings, input.bindings(1)});
    input.set_motors(0, {true, 255});
    input.update();
    CHECK(g_felt.low == 0 && g_felt.high == 0);
    bindings.tuning.rumble.strength = 100;
    input.set_bindings({bindings, input.bindings(1)});
    // Unplugged while it shakes: nothing is sent to it, and the port starts afresh.
    input.set_motors(0, {false, 200});
    input.update();
    CHECK(g_felt.low > 0);
    SDL_DetachVirtualJoystick(a);
    pump(input);
    input.set_motors(0, {false, 200});
    input.update();
    input.stop_rumble();
    CHECK(input.rumble_output(0).silent());
}

}  // namespace

int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    test_dock_first();
    test_two_players();
    test_hand_over();
    test_unplug();
    test_three();
    test_capture_while_paused();
    test_hold_back_held();
    test_unfocused();
    test_quick_tap();
    test_rumble();
    SDL_Quit();
    return openrac::test::result();
}
