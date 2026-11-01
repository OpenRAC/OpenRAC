// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's menu bar (debug/overlay.h): the menus along the window's top on Windows and
// Linux, whose items press the keys the port already answers or move its settings on.

#include <algorithm>
#include <format>
#include <imgui.h>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "common/env.h"
#include "common/log.h"
#include "common/slow_cpu.h"
#include "debug/overlay.h"
#include "mods.h"
#include "options_menu.h"
#include "renderer/gl.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

namespace {

// The menus along the window's top, as OpenGOAL has them: on Windows and Linux. macOS has its own
// menu bar, with the same menus (platform/macos_app.mm).
#if defined(__APPLE__)
constexpr bool kMenuBar = false;
#else
constexpr bool kMenuBar = true;
#endif

}  // namespace

// OPENRAC_MENU_BAR=1 has them on macOS as well (for looking at them there).
bool menu_bar_possible() {
    return (kMenuBar || env::flag(env::Var::MenuBar)) && !drawer_possible();
}

bool menu_bar_shown() {
    return menu_bar_possible() && settings::get().menu_bar;
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

namespace {

// A line under a menu item on when to use it, as the drawer's notes (drawer_note).
void menu_note(const char* line) {
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 20.0f);
    ImGui::TextDisabled("%s", line);
    ImGui::PopTextWrapPos();
}

// Developer > Record diagnostics: the drawer's rows (debug/drawer.cpp) in the same order.
void record_menu() {
    if (capture_pending()) {
        if (ImGui::MenuItem("Cancel recording", "Ctrl+F3")) {
            toggle_capture(false);
        }
        return;
    }
    if (capture_busy()) {
        if (ImGui::MenuItem("Stop recording", "Ctrl+F3")) {
            toggle_capture(false);
        }
        return;
    }
    if (ImGui::MenuItem("Gameplay recording", "Ctrl+F3")) {
        toggle_capture(false);
    }
    menu_note(
        "Records while you play, for the length below. Use it to catch stutters or a slow place "
        "or moment."
    );
    // Moves the length on, its value beside it, and leaves the menu open (as Graphics does).
    ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
    if (ImGui::MenuItem("Recording length", capture_length_name())) {
        step_capture_length(1);
    }
    ImGui::PopItemFlag();
    ImGui::Separator();
    if (ImGui::MenuItem(
            std::format("Graphics settings sweep ({:.0f} s)", settings_sweep_seconds()).c_str()
        )) {
        toggle_settings_sweep();
    }
    menu_note(
        "Stand still while it runs: it changes the graphics settings every few seconds, with "
        "VSync off, and measures what each one costs. Use it when the game is slow everywhere."
    );
    ImGui::Separator();
    // Where they are saved (capture.cpp's capture_dir), which the toast also names.
    if (ImGui::MenuItem("Open recordings folder")) {
        open_capture_folder();
    }
}

}  // namespace

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
    if (name == "none") {
        return SDL_GAMEPAD_BUTTON_INVALID;
    }
    const SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name.c_str());
    // A name SDL does not know: the default, R4.
    return button == SDL_GAMEPAD_BUTTON_INVALID ? SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1 : button;
}

std::string menu_bar_button_label() {
    switch (const SDL_GamepadButton button = menu_bar_button()) {
        case SDL_GAMEPAD_BUTTON_INVALID:
            return "none";
        // The back buttons by the names on Valve's pads (the Steam Controller, the Steam Deck).
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

void toggle_menu_bar() {
    settings::get().menu_bar = !settings::get().menu_bar;
    settings::save();
    if (!settings::get().menu_bar) {
        s.toast = menu_bar_key_label() + " shows the menus again";
        s.toast_until = ImGui::GetTime() + 4.0;
    }
}

void menu_bar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    const std::string key = menu_bar_key_label();
    if (ImGui::BeginMenu("Game")) {
        if (ImGui::MenuItem("Full screen", "F11")) {
            press(SDLK_F11);
        }
        if (ImGui::MenuItem(
                "Frame counter", "F2", settings::get().show_fps != settings::FrameCounter::Off
            )) {
            press(SDLK_F2);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Hide these menus", key.c_str())) {
            toggle_menu_bar();
        }
        ImGui::Separator();
        // Closes the game as the window's close button does: the frame loop ends the program
        // normally, so the launcher sees an ordinary exit.
        if (ImGui::MenuItem("Exit game", "Alt+F4")) {
            log::info("exit: chosen in the menu bar, closing the game");
            SDL_Event event;
            SDL_zero(event);
            event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&event);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Controls")) {
        if (ImGui::MenuItem("Controller bindings...", "F13", s.controls_window)) {
            s.controls_window = !s.controls_window;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Graphics")) {
        // Each item moves its setting on, its value beside it, and leaves the menu open.
        ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
        using namespace options_menu;
        for (const int row : {
                 kAspect, kHud, kRenderScale, kMsaa, kAnisotropy, kSceneryDetail, kFrameRate, kVSync
             }) {
            const bool shown_for =
                row != kHud || settings::get().aspect != settings::Aspect::Standard;
            if (ImGui::MenuItem(label(row), value(row).c_str(), false, shown_for)) {
                change_setting(row, 1);
            }
        }
        ImGui::PopItemFlag();
        if (gl::enhanced_rendering_available()) {
            ImGui::Separator();
            ImGui::MenuItem("Enhanced Settings", nullptr, &s.enhanced_settings);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Sound")) {
        if (ImGui::MenuItem("Volume up", "F10")) {
            press(SDLK_F10);
        }
        if (ImGui::MenuItem("Volume down", "F9")) {
            press(SDLK_F9);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Developer")) {
        if (ImGui::MenuItem("Debug overlay", "F3", s.visible)) {
            press(SDLK_F3);
        }
        bool picker = picker_enabled();
        if (ImGui::MenuItem("Moby picker", nullptr, &picker)) {
            set_picker_enabled(picker);
        }
        if (s.developer) {
            if (ImGui::MenuItem("Object Viewer", nullptr, &s.object_viewer.open)) {
                if (s.object_viewer.open) {
                    set_picker_enabled(true);
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Pause", "F5", s.paused)) {
            press(SDLK_F5);
        }
        if (ImGui::MenuItem("Step one tick", "F6")) {
            press(SDLK_F6);
        }
        if (ImGui::MenuItem("Slower", "F7", false, s.stretch < 16.0)) {
            press(SDLK_F7);
        }
        if (ImGui::MenuItem("Faster", "F8", false, s.stretch > 1.0)) {
            press(SDLK_F8);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Screenshot", "F12")) {
            press(SDLK_F12);
        }
        ImGui::Separator();
        // Diagnostics recordings (debug/capture.cpp) as the drawer has them: one entry that holds
        // the two kinds, each with its line on when to use it.
        if (ImGui::BeginMenu("Record diagnostics")) {
            record_menu();
            ImGui::EndMenu();
        }
        // A weaker device's processor, for testing how the game copes (common/slow_cpu.h).
        if (slow_cpu::available() && ImGui::BeginMenu("Simulate slow CPU")) {
            for (const int factor : {1, 2, 4, 6}) {
                const std::string name = factor > 1 ? std::format("{}x", factor) : "Off";
                if (ImGui::MenuItem(name.c_str(), nullptr, slow_cpu::factor() == factor)) {
                    slow_cpu::set_factor(factor);
                }
            }
            ImGui::EndMenu();
        }
        if (s.developer) {
            ImGui::Separator();
            if (ImGui::MenuItem("Level select", "F1")) {
                press(SDLK_F1);
            }
        }
        ImGui::EndMenu();
    }
    for (const mods::DeveloperMenu& menu : mods::developer_menus()) {
        if (!ImGui::BeginMenu(menu.title.c_str())) {
            continue;
        }
        for (std::size_t i = 0; i < menu.items.size(); ++i) {
            const mods::DeveloperMenuItem& item = menu.items[i];
            if (item.group.empty()) {
                const char* shortcut = item.shortcut.empty() ? nullptr : item.shortcut.c_str();
                ImGui::PushID(&item);
                if (ImGui::MenuItem(item.label.c_str(), shortcut)) {
                    mods::queue_developer_menu_action(menu.mod, item.action);
                }
                ImGui::PopID();
                continue;
            }
            const bool already_shown = std::any_of(
                menu.items.begin(),
                menu.items.begin() + i,
                [&](const mods::DeveloperMenuItem& prior) { return prior.group == item.group; }
            );
            if (already_shown || !ImGui::BeginMenu(item.group.c_str())) {
                continue;
            }
            for (const mods::DeveloperMenuItem& grouped : menu.items) {
                if (grouped.group != item.group) {
                    continue;
                }
                const char* shortcut =
                    grouped.shortcut.empty() ? nullptr : grouped.shortcut.c_str();
                ImGui::PushID(&grouped);
                if (ImGui::MenuItem(grouped.label.c_str(), shortcut)) {
                    mods::queue_developer_menu_action(menu.mod, grouped.action);
                }
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    // At the right, greyed: the key that hides the bar (and brings it back).
    const std::string hint = key + ": hide or show the menus";
    const float width = ImGui::CalcTextSize(hint.c_str()).x + ImGui::GetStyle().ItemSpacing.x;
    if (ImGui::GetContentRegionAvail().x > width) {
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - width - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextDisabled("%s", hint.c_str());
    }
    ImGui::EndMainMenuBar();
}

}  // namespace openrac::frontend::debug_overlay
