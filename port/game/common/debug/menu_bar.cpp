// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What opens the menu (debug/overlay.h): the key and the controller button the player picked, and
// the menu's opening and closing by them. The menu itself is debug/menu_window.cpp on a desktop and
// debug/drawer.cpp on a touch screen.

#include <string>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"
#include "debug_overlay.h"
#include "frontend.h"
#include "platform/input.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

// Everywhere the drawer is not: Windows, Linux and macOS (whose own menu bar keeps its few items,
// platform/macos_app.mm).
bool menu_window_possible() {
    return !drawer_possible();
}

// A menu item presses the key the port already answers, as the macOS menus do.
void press(SDL_Keycode key) {
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    event.key.down = true;
    SDL_PushEvent(&event);
    event.type = SDL_EVENT_KEY_UP;
    event.key.down = false;
    SDL_PushEvent(&event);
}

SDL_Scancode menu_bar_scancode() {
    const std::string& name = settings::get().menu_bar_key;
    const SDL_Scancode key =
        name == "Alt" ? SDL_SCANCODE_LALT : SDL_GetScancodeFromName(name.c_str());
    if (key == SDL_SCANCODE_LALT || key == SDL_SCANCODE_RALT) {
        return SDL_SCANCODE_UNKNOWN;
    }
    // A name SDL does not know: the default, the key under Escape.
    return key == SDL_SCANCODE_UNKNOWN ? SDL_SCANCODE_GRAVE : key;
}

std::string menu_bar_key_label() {
    const SDL_Scancode key = menu_bar_scancode();
    if (key == SDL_SCANCODE_UNKNOWN) {
        return "Alt";
    }
    // What the key prints on the player's own layout, else its place's name.
    const char* name = SDL_GetKeyName(SDL_GetKeyFromScancode(key, SDL_KMOD_NONE, false));
    return name != nullptr && *name != '\0' ? std::string(name)
                                            : std::string(SDL_GetScancodeName(key));
}

SDL_GamepadButton menu_bar_button() {
    const std::string& name = settings::get().menu_bar_button;
    return name == "none" ? SDL_GAMEPAD_BUTTON_INVALID
                          : SDL_GetGamepadButtonFromString(name.c_str());
}

// "start+back", the default, and any name SDL does not know.
bool menu_bar_chord() {
    return settings::get().menu_bar_button != "none"
           && menu_bar_button() == SDL_GAMEPAD_BUTTON_INVALID;
}

std::string menu_bar_button_label() {
    if (menu_bar_chord()) {
        return "Start+Select";
    }
    switch (const SDL_GamepadButton button = menu_bar_button()) {
        case SDL_GAMEPAD_BUTTON_INVALID:
            return "none";
        // The back buttons by the names on Valve's pads (the Steam Controller, the Steam Deck).
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return "L3";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return "R3";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
            return "R4";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
            return "L4";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
            return "R5";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
            return "L5";
        default: {
            const char* name = SDL_GetGamepadStringForButton(button);
            return name != nullptr ? std::string(name) : std::string("?");
        }
    }
}

std::string menu_key_label() {
    return menu_bar_key_label();
}

void toggle_menu() {
    set_menu(!s.menu_open);
}

void menu_pad_chord() {
    platform::Input* input = frontend::input();
    if (input == nullptr) {
        return;
    }
    input->set_menu_chord(menu_bar_chord());
    if (input->take_menu_chord() && !s.menu_open && SDL_GetKeyboardFocus() != nullptr
        && !s.pad_locked && !controls_capturing()) {
        s.menu_motion.pad = true;
        toggle_menu();
    }
}

}  // namespace openrac::frontend::debug_overlay
