// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's touch handling (debug/overlay.h): whether the drawer is offered, the drawer's
// opening and shutting, and the fingers it takes as ImGui's pointer, away from the on-screen pad;
// with the drag that scrolls the drawer and the panel, and the editing of the pad's layout.

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <imgui.h>
// ImGui::ClearActiveID: a finger that scrolls lets go of what it pressed; FindHoveredWindowEx.
#include <imgui_internal.h>
#include <optional>

#include <SDL3/SDL.h>

#include "common/env.h"
#include "debug/menu_ui.h"
#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {

namespace {

// Whether a finger lands on the bar of the layout's editing.
bool on_edit_bar(const SDL_TouchFingerEvent& f) {
    SDL_Window* window = SDL_GetWindowFromID(f.windowID);
    int w = 0;
    int h = 0;
    if (!s.touch_editing || window == nullptr || !SDL_GetWindowSize(window, &w, &h)) {
        return false;
    }
    const float x = f.x * static_cast<float>(w);
    const float y = f.y * static_cast<float>(h);
    return x >= s.bar_min.x && x <= s.bar_max.x && y >= s.bar_min.y && y <= s.bar_max.y;
}

// A finger, at its place in the window, as ImGui's pointer.
// A finger's place in ImGui's coordinates (the window's), or none without its window.
std::optional<ImVec2> finger_point(const SDL_TouchFingerEvent& f) {
    SDL_Window* window = SDL_GetWindowFromID(f.windowID);
    int w = 0;
    int h = 0;
    if (window == nullptr || !SDL_GetWindowSize(window, &w, &h)) {
        return std::nullopt;
    }
    return ImVec2(f.x * static_cast<float>(w), f.y * static_cast<float>(h));
}

void pointer_at(const SDL_TouchFingerEvent& f) {
    if (const std::optional<ImVec2> at = finger_point(f)) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(at->x, at->y);
    }
}

// Whether one of the overlay's windows (the debug panel, an inspector) is under `at`.
bool on_overlay_window(ImVec2 at) {
    ImGuiWindow* window = nullptr;
    ImGui::FindHoveredWindowEx(at, false, &window, nullptr);
    return window != nullptr;
}

}  // namespace

// On mobile the drawer has the menus (debug_overlay.h); OPENRAC_DRAWER=1 elsewhere too.
bool drawer_possible() {
#if defined(__ANDROID__) || defined(OPENRAC_IOS)
    return true;
#else
    return env::flag(env::Var::Drawer);
#endif
}

// A finger dragged up or down the window being drawn scrolls it, and lets go of what it came down
// on, which is then not pressed; let go, the rows glide on (Scroll).
void drag_scroll(Scroll& scroll) {
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsMouseClicked(0)) {
        scroll.dragged = false;
        scroll.velocity = 0.0f;  // a finger that lands stops it
        s.slider_held = false;
    }
    if (s.slider_held) {
        return;  // the finger moves a slider (menu_percent_slider), not the rows
    }
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
        && ImGui::IsMouseDown(0)
        && (scroll.dragged || std::abs(ImGui::GetMouseDragDelta(0).y) > dp(10.0f))) {
        if (!scroll.dragged) {
            ImGui::ClearActiveID();
        }
        scroll.dragged = true;
        ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
        // The finger's speed over the last few frames, which it leaves the rows with.
        if (io.DeltaTime > 0.0f) {
            scroll.velocity += (-io.MouseDelta.y / io.DeltaTime - scroll.velocity) * 0.4f;
        }
        return;
    }
    // Let go: the rows glide on and slow down, as a phone's lists do, until an edge stops them.
    if (scroll.velocity == 0.0f || ImGui::IsMouseDown(0)) {
        return;
    }
    const float before = ImGui::GetScrollY();
    ImGui::SetScrollY(
        std::clamp(before + scroll.velocity * io.DeltaTime, 0.0f, ImGui::GetScrollMaxY())
    );
    scroll.velocity *= std::exp(-io.DeltaTime * 4.0f);
    if (std::abs(scroll.velocity) < dp(20.0f) || (before <= 0.0f && scroll.velocity < 0.0f)
        || (before >= ImGui::GetScrollMaxY() && scroll.velocity > 0.0f)) {
        scroll.velocity = 0.0f;
    }
}

// ---- The drawer ----

// Starts or ends the editing of the on-screen pad's layout: the menu shuts, and the game stays
// paused until it ends.
void edit_touch(bool on) {
    if (on == s.touch_editing) {
        return;
    }
    if (on) {
        set_menu(false);
        s.keep_paused = s.paused;
        s.paused = true;
        s.held_frames = 0;
        s.bar_min = s.bar_max = ImVec2(0.0f, 0.0f);
    } else {
        s.paused = s.keep_paused;
    }
    s.touch_editing = on;
}

// The drawer's events: Back and Guide open and shut it; fingers are ImGui's pointer (the system's
// mouse made from them follows only the first finger on the screen, perhaps a thumb on a stick),
// and those that land while it is open are its own, not the on-screen pad's.
bool drawer_event(const SDL_Event& e) {
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (e.key.key != SDLK_AC_BACK) {
                return false;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                // Back ends the layout's editing, as Done does.
                s.touch_editing ? edit_touch(false) : set_menu(!s.menu_open);
            }
            return true;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (e.gbutton.button != SDL_GAMEPAD_BUTTON_GUIDE) {
                return false;
            }
            s.touch_editing ? edit_touch(false) : set_menu(!s.menu_open);
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            return e.motion.which == SDL_TOUCH_MOUSEID;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return e.button.which == SDL_TOUCH_MOUSEID;
        case SDL_EVENT_MOUSE_WHEEL:
            return e.wheel.which == SDL_TOUCH_MOUSEID;
        case SDL_EVENT_FINGER_DOWN: {
            if (e.tfinger.touchID == SDL_MOUSE_TOUCHID || !s.initialised) {
                return false;
            }
            const ImVec2 at = finger_point(e.tfinger).value_or(ImVec2(-FLT_MAX, -FLT_MAX));
            // A notification takes a tap on it (debug/notifications.cpp).
            if (!s.menu_open && press_notification(at)) {
                s.taken.push_back(e.tfinger.fingerID);
                return true;
            }
            const bool on_bar = on_edit_bar(e.tfinger);
            if (!s.pointer_down && (s.menu_open || s.visible || on_bar)) {
                s.pointer = e.tfinger.fingerID;
                s.pointer_down = true;
                pointer_at(e.tfinger);
                ImGui::GetIO().AddMouseButtonEvent(0, true);
            }
            // The game's on-screen pad has the finger unless the menu, the editor's bar or one of
            // the overlay's windows is under it.
            if (!s.menu_open && !on_bar && !(s.visible && on_overlay_window(at))) {
                return false;
            }
            s.taken.push_back(e.tfinger.fingerID);
            return true;
        }
        case SDL_EVENT_FINGER_MOTION:
            if (s.pointer_down && e.tfinger.fingerID == s.pointer && s.initialised) {
                pointer_at(e.tfinger);
            }
            return std::find(s.taken.begin(), s.taken.end(), e.tfinger.fingerID) != s.taken.end();
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: {
            if (s.pointer_down && e.tfinger.fingerID == s.pointer && s.initialised) {
                pointer_at(e.tfinger);
                ImGuiIO& io = ImGui::GetIO();
                io.AddMouseButtonEvent(0, false);
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);  // a lifted finger hovers nothing
                s.pointer_down = false;
            }
            const auto it = std::find(s.taken.begin(), s.taken.end(), e.tfinger.fingerID);
            if (it == s.taken.end()) {
                return false;
            }
            s.taken.erase(it);
            return true;
        }
        default:
            return false;
    }
}

}  // namespace openrac::frontend::debug_overlay
