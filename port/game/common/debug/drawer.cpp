// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's drawer (debug/overlay.h): the menu on a touch screen, a panel that slides out
// from the left with the menu's entries (menu_catalog.h) on cards, drawn by debug/menu_ui.h as the
// desktop's menu draws them, with rows as high as a thumb; and the bar shown while the on-screen
// pad's layout is edited.

#include <algorithm>
#include <cmath>
#include <format>
#include <imgui.h>
#include <imgui_internal.h>  // ImGui::FindRenderedTextEnd: a label's text before its "##" id
#include <string>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"
#include "renderer/gl.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

namespace {

// What the picture is drawn with: the launcher's renderer choice takes effect only when the game
// starts, and here the player sees which one this run has.
const std::string& graphics_line() {
    static const std::string graphics = [] {
        const auto* name =
            reinterpret_cast<const char*>(openrac::gl::glGetString(openrac::gl::GL_RENDERER));
        const char* api = s.gl_subset ? "Vulkan" : openrac::gl::is_es() ? "OpenGL ES" : "OpenGL";
        return std::format("{} · {}", api, name != nullptr ? name : "unknown GPU");
    }();
    return graphics;
}

}  // namespace

const std::string& menu_graphics_line() {
    return graphics_line();
}

void drawer(const Shown& shown) {
    const ImGuiIO& io = ImGui::GetIO();
    // Out in the launcher's --medium, back in a little quicker, as its pages come and go.
    const float target = s.menu_open ? 1.0f : 0.0f;
    const float move = io.DeltaTime / (s.menu_open ? kMedium : 0.2f);
    s.menu_at = s.menu_at < target ? std::min(s.menu_at + move, target)
                                   : std::max(s.menu_at - move, target);
    if (s.menu_at <= 0.0f) {
        menu_closed();
        return;
    }
    s.menu_row_height = 48.0f;
    // The launcher's --ease, cubic-bezier(0.2, 0.8, 0.2, 1): quick out of the edge, settling slowly.
    const float ease = 1.0f - std::pow(1.0f - s.menu_at, 4.0f);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // The left of the screen a camera's notch keeps.
    float inset = 0.0f;
    SDL_Rect safe{};
    if (shown.window != nullptr && SDL_GetWindowSafeArea(shown.window, &safe)) {
        inset = static_cast<float>(safe.x);
    }
    const float width = std::min(viewport->Size.x * 0.6f, dp(360.0f)) + inset;
    const float x = viewport->Pos.x - width * (1.0f - ease);
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewport->Pos,
        ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y),
        ui_colors::fade(ui().ink_deep, 0.6f * ease)
    );
    // A touch beside it shuts it, as does the gamepad's B.
    if (s.menu_open && ImGui::IsMouseReleased(0) && !s.menu_scroll.dragged
        && io.MouseClickedPos[0].x > x + width) {
        set_menu(false);
    }
    if (s.menu_open && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
        set_menu(false);
    }
    ImGui::SetNextWindowPos(ImVec2(x, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(width, viewport->Size.y));
    if (s.menu_focus) {
        ImGui::SetNextWindowFocus();
        s.menu_focus = false;
    }
    // The launcher's panel: its ink, a thin border, 16-pixel corners on the open side; a header
    // above the rows, which scroll under it.
    const float header = dp(52.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(14.0f) + inset, header + dp(4.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, dp(4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImGui::ColorConvertU32ToFloat4(ui().line_strong));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("OpenRAC options", nullptr, flags)) {
        drag_scroll(s.menu_scroll);
        {
            ImDrawList* back = ImGui::GetWindowDrawList();
            const ImVec2 top(x, viewport->Pos.y);
            const ImVec2 bottom(x + width, viewport->Pos.y + viewport->Size.y);
            back->PushClipRect(top, bottom, false);
            back->AddRectFilled(top, bottom, ui().panel, dp(16.0f), ImDrawFlags_RoundCornersRight);
            back->AddRect(top, bottom, ui().line, dp(16.0f), ImDrawFlags_RoundCornersRight);
            back->PopClipRect();
        }
        {
            // The header: the logo, and what the game is doing while the drawer is out.
            ImDrawList* front = ImGui::GetForegroundDrawList();
            const ImVec2 top(x, viewport->Pos.y);
            const ImVec2 bottom(x + width, viewport->Pos.y + header);
            front->AddRectFilled(
                top, bottom, ui().panel, dp(16.0f), ImDrawFlags_RoundCornersTopRight
            );
            // A hairline under it once rows have scrolled beneath, fading in as they do.
            const float under = std::min(ImGui::GetScrollY() / dp(24.0f), 1.0f);
            if (under > 0.0f) {
                front->AddLine(ImVec2(top.x, bottom.y), bottom, ui_colors::fade(ui().line, under));
            }
            draw_logo(
                front,
                ImVec2(top.x + inset + dp(18.0f), top.y + header * 0.5f),
                ui().amber,
                ui().text
            );
            const char* state = s.keep_paused ? "Paused" : "Paused while open";
            const ImVec2 state_size = ImGui::CalcTextSize(state);
            const ImVec2 pill_b(
                bottom.x - dp(16.0f), top.y + header * 0.5f + state_size.y * 0.5f + dp(4.0f)
            );
            const ImVec2 pill_a(
                pill_b.x - state_size.x - dp(20.0f), pill_b.y - state_size.y - dp(8.0f)
            );
            // A chip, as the launcher's: its deepest ink, a hairline, soft text.
            front->AddRectFilled(pill_a, pill_b, ui().ink_deep, (pill_b.y - pill_a.y) * 0.5f);
            front->AddRect(pill_a, pill_b, ui().line, (pill_b.y - pill_a.y) * 0.5f);
            front->AddText(ImVec2(pill_a.x + dp(10.0f), pill_a.y + dp(4.0f)), ui().soft, state);
        }

        if (menu_button("Resume")) {
            set_menu(false);
        }
        ImGui::SetItemDefaultFocus();  // where a gamepad starts

        // The menu's tabs one after another, a heading and a card for each of their sections,
        // then the mods' own menus.
        using menu::Tab;
        for (const Tab tab : {Tab::Graphics, Tab::Enhanced, Tab::Game, Tab::Controls}) {
            draw_tab(tab, menu::kTouch);
        }
        for (int i = 0; i < mod_menu_count(); ++i) {
            ImGui::PushID(i);
            draw_mod_menu(i);
            ImGui::PopID();
        }

        // The developer's tools, folded away under one row.
        ImGui::Dummy(ImVec2(0.0f, dp(16.0f)));
        card_begin();
        const bool developer = s.drawer_developer;
        if (menu_row("Developer tools", ui().text, [developer](ImDrawList* list, ImVec2 right) {
                // The arrow turns as the section opens.
                const float k = tween(item_part("open"), developer ? 1.0f : 0.0f, kMedium);
                const float dy = 1.0f - 2.0f * k;
                arrow(
                    list, ImVec2(right.x - dp(5.0f), right.y + dp(2.5f) * dy), 0.0f, dy, ui().soft
                );
            })) {
            s.drawer_developer = !s.drawer_developer;
        }
        card_end();
        if (developer) {
            draw_tab(Tab::Developer, menu::kTouch);
        }

        ImGui::Dummy(ImVec2(0.0f, dp(16.0f)));
        card_begin();
        draw_entry(menu::Id::Quit, menu::kTouch);
        card_end();

        ImGui::Dummy(ImVec2(0.0f, dp(14.0f)));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ui().dim));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dp(6.0f));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(graphics_line().c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, dp(12.0f)));
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(5);
}

namespace {

// A button of the editing bar, the launcher's main one or a plain one: true when pressed.
bool bar_button(const char* label, float width, bool primary = false) {
    const float height = dp(44.0f);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + width, a.y + height);
    const bool pressed = ImGui::InvisibleButton(label, ImVec2(width, height));
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float round = dp(12.0f);
    const float held = tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast);
    if (primary) {
        primary_fill(list, a, b, round, held);
    } else {
        list->AddRectFilled(a, b, ui_colors::mix(ui().panel_hi, pressed_fill(), held), round);
        list->AddRect(a, b, ui().line_strong, round);
    }
    focus_ring(
        list,
        ImVec2(a.x - dp(3.0f), a.y - dp(3.0f)),
        ImVec2(b.x + dp(3.0f), b.y + dp(3.0f)),
        round + dp(3.0f)
    );
    const ImVec2 size = ImGui::CalcTextSize(label, nullptr, true);
    list->AddText(
        ImVec2((a.x + b.x - size.x) * 0.5f, a.y + (height - size.y) * 0.5f),
        primary ? ui().bar_ink : ui().text,
        label,
        ImGui::FindRenderedTextEnd(label)
    );
    return pressed;
}

}  // namespace

// While the on-screen pad's layout is edited: a bar in the middle of the screen, clear of the
// controls' places, to make them smaller or larger, put them back, and finish.
void touch_edit_bar() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float gap = dp(8.0f);
    const float button = dp(52.0f);
    const float wide = dp(96.0f);
    const float width = button * 2.0f + wide * 3.0f + gap * 4.0f + dp(28.0f);
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.42f
        ),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(14.0f), dp(12.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, dp(10.0f)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(ui().panel));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(ui().line));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoSavedSettings
                                   | ImGuiWindowFlags_AlwaysAutoResize;
    // The lines the dragged group snapped to, in the accent, under the bar.
    ImDrawList* guides = ImGui::GetBackgroundDrawList();
    const ImU32 guide = ui_colors::fade(ui().amber, 0.7f);
    if (s.touch_guides.x >= 0.0f) {
        const float x = viewport->Pos.x + viewport->Size.x * s.touch_guides.x;
        guides->AddLine(
            ImVec2(x, viewport->Pos.y),
            ImVec2(x, viewport->Pos.y + viewport->Size.y),
            guide,
            dp(1.5f)
        );
    }
    if (s.touch_guides.y >= 0.0f) {
        const float y = viewport->Pos.y + viewport->Size.y * s.touch_guides.y;
        guides->AddLine(
            ImVec2(viewport->Pos.x, y),
            ImVec2(viewport->Pos.x + viewport->Size.x, y),
            guide,
            dp(1.5f)
        );
    }
    if (ImGui::Begin("Move the controls", nullptr, flags)) {
        auto& set = settings::get();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ui().soft));
        ImGui::TextUnformatted("Drag a control to move it. The game is paused.");
        ImGui::PopStyleColor();
        bool resized = false;
        if (bar_button("-##smaller", button)) {
            set.touch_scale = settings::step_listed(settings::kTouchScales, set.touch_scale, -1);
            resized = true;
        }
        ImGui::SameLine();
        const std::string size = std::format("{}%##size", set.touch_scale);
        bar_button(size.c_str(), wide);
        ImGui::SameLine();
        if (bar_button("+##larger", button)) {
            set.touch_scale = settings::step_listed(settings::kTouchScales, set.touch_scale, 1);
            resized = true;
        }
        ImGui::SameLine();
        if (bar_button("Reset", wide)) {
            s.touch_reset = true;
        }
        ImGui::SameLine();
        if (bar_button("Done", wide, true)) {
            edit_touch(false);
        }
        if (resized) {
            settings::save();
        }
        s.bar_min = ImGui::GetWindowPos();
        s.bar_max =
            ImVec2(s.bar_min.x + ImGui::GetWindowSize().x, s.bar_min.y + ImGui::GetWindowSize().y);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

}  // namespace openrac::frontend::debug_overlay
