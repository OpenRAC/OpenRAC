// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which gamepad plays on which port (platform/input.h): a gamepad that is never pressed, found
// before the controller (a USB-C dock's, a hub's), does not keep the controller off port 0.
#include <cstdio>

#include <SDL3/SDL.h>

#include "platform/input.h"
#include "tests/check.h"

namespace {

using openrac::platform::Button;
using openrac::platform::Input;

SDL_JoystickID attach(const char* name) {
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = name;
    return SDL_AttachVirtualJoystick(&desc);
}

// The events SDL has for the gamepads, handed to the input as the window's loop does.
void pump(Input& input) {
    SDL_UpdateJoysticks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input.handle_event(event);
    }
}

void press(Input& input, SDL_JoystickID id, bool down) {
    SDL_Joystick* joystick = SDL_GetJoystickFromID(id);
    SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, down);
    pump(input);
}

bool cross(Input& input, int port) {
    input.refresh_pads();
    return input.pad(port).held(Button::Cross);
}

}  // namespace

int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    {
        Input input;
        const SDL_JoystickID dock = attach("dock");
        const SDL_JoystickID controller = attach("controller");
        pump(input);

        // The controller pressed: it plays on port 0, though the dock's device came first.
        press(input, controller, true);
        CHECK(cross(input, 0));
        CHECK(!cross(input, 1));
        press(input, controller, false);

        // The dock's device pressed later goes behind it, to port 1.
        press(input, dock, true);
        CHECK(!cross(input, 0));
        CHECK(cross(input, 1));
        press(input, dock, false);

        // The controller gone: the other one moves up to port 0.
        SDL_DetachVirtualJoystick(controller);
        pump(input);
        press(input, dock, true);
        CHECK(cross(input, 0));
        press(input, dock, false);

        // A controller connected again comes after it, until it is pressed and the dock's is
        // still the one used first.
        const SDL_JoystickID again = attach("controller");
        pump(input);
        press(input, again, true);
        CHECK(!cross(input, 0));
        CHECK(cross(input, 1));
        press(input, again, false);
    }
    SDL_Quit();
    return openrac::test::result();
}
