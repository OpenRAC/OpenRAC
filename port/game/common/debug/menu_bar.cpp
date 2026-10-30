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

void menu_bar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Full screen", "F11")) {
            press(SDLK_F11);
        }
        if (ImGui::MenuItem(
                "Frame counter", "F2", settings::get().show_fps != settings::FrameCounter::Off
            )) {
            press(SDLK_F2);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Hide these menus", "Alt")) {
            settings::get().menu_bar = false;
            settings::save();
            s.toast = "Alt shows the menus again";
            s.toast_until = ImGui::GetTime() + 4.0;
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
        if (ImGui::MenuItem(
                capture_busy() ? "Stop recording diagnostics" : "Record diagnostics", "Ctrl+F3"
            )) {
            toggle_capture(false);
        }
        if (ImGui::BeginMenu("Recording length")) {
            for (const double seconds : {15.0, 30.0, 60.0, 120.0, 0.0}) {
                const std::string name =
                    seconds > 0.0 ? std::format("{:.0f} s", seconds) : std::string("Until stopped");
                if (ImGui::MenuItem(name.c_str(), nullptr, s.capture_seconds == seconds)) {
                    s.capture_seconds = seconds;
                }
            }
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
        // Each pass's own GPU time on a tile-based GPU, at the price of a slower frame.
        if (pass_timing_available()
            && ImGui::MenuItem(
                "Time GPU passes", nullptr, std::string_view(pass_timing_name()) == "On"
            )) {
            toggle_pass_timing();
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
    ImGui::EndMainMenuBar();
}

}  // namespace openrac::frontend::debug_overlay
