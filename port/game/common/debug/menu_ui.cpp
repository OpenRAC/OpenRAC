// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The in-game menu's pieces in the launcher's look (debug/menu_ui.h): the rows, switches, arrows,
// headings, buttons and notes both menus draw, their typefaces, and the menu's opening and closing.

#include "debug/menu_ui.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <imgui_internal.h>  // ImHashStr, ShadeVertsLinearColorGradientKeepAlpha
#include <string>

// The menu's typefaces as bytes (cmake/Games.cmake, openrac_menu_fonts, from game/common/fonts/).
extern const unsigned char openrac_font_inter[];
extern const unsigned int openrac_font_inter_size;
extern const unsigned char openrac_font_inter_semibold[];
extern const unsigned int openrac_font_inter_semibold_size;
extern const unsigned char openrac_font_orbitron[];
extern const unsigned int openrac_font_orbitron_size;

namespace openrac::frontend::debug_overlay {

MenuFonts menu_fonts;

const ui_colors::Colors& ui() {
    return ui_colors::launcher();
}

ImU32 pressed_fill() {
    return ui_colors::mix(ui().panel_hi, ui().amber, 0.16f);
}

float tween(ImGuiID id, float target, float seconds) {
    auto [at, made] = s.tweens.try_emplace(id, target);
    float& v = at->second;
    if (!made) {
        v += (target - v) * (1.0f - std::exp(-ImGui::GetIO().DeltaTime * 3.0f / seconds));
        if (std::abs(target - v) < 0.002f) {
            v = target;
        }
    }
    return v;
}

ImGuiID item_part(const char* part) {
    return ImHashStr(part, 0, ImGui::GetItemID());
}

namespace {

// One typeface from its bytes, at `pixels`: ImGui copies nothing (the bytes stay the program's),
// and Latin with its accents, as the subset holds.
ImFont* add_font(const unsigned char* bytes, unsigned int size, float pixels) {
    if (size == 0) {
        return nullptr;
    }
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = 2;
    config.OversampleV = 1;
    static const ImWchar kRanges[] = {0x0020, 0x017F, 0x2013, 0x2026, 0x2190, 0x2193, 0};
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(bytes), static_cast<int>(size), pixels, &config, kRanges
    );
}

}  // namespace

void add_menu_fonts() {
    const float body = std::round(15.0f * s.ui_scale);
    menu_fonts.body = add_font(openrac_font_inter, openrac_font_inter_size, body);
    menu_fonts.strong =
        add_font(openrac_font_inter_semibold, openrac_font_inter_semibold_size, body);
    menu_fonts.head =
        add_font(openrac_font_orbitron, openrac_font_orbitron_size, std::round(17.0f * s.ui_scale));
    ImFont* fallback =
        ImGui::GetIO().Fonts->Fonts.empty() ? nullptr : ImGui::GetIO().Fonts->Fonts[0];
    for (ImFont** font : {&menu_fonts.body, &menu_fonts.strong, &menu_fonts.head}) {
        if (*font == nullptr) {
            *font = fallback;
        }
    }
}

void draw_logo(ImDrawList* list, ImVec2 left_middle, ImU32 mark, ImU32 name) {
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    const ImFontAtlasCustomRect* at =
        s.logo_rect >= 0 ? atlas->GetCustomRectByIndex(s.logo_rect) : nullptr;
    if (at == nullptr) {
        return;
    }
    ImVec2 uv0;
    ImVec2 uv1;
    atlas->CalcCustomRectUV(at, &uv0, &uv1);
    const float w = at->Width;
    const float h = at->Height;
    // The capitals of "RAC" sit 63% of the way down the logo's box (y 320 to 455 of 176 to 510).
    const ImVec2 a(std::round(left_middle.x), std::round(left_middle.y - h * 0.633f));
    const float split = static_cast<float>(s.logo_name_x);
    const float uv_split = uv0.x + (uv1.x - uv0.x) * split / w;
    list->AddImage(
        atlas->TexID, a, ImVec2(a.x + split, a.y + h), uv0, ImVec2(uv_split, uv1.y), mark
    );
    list->AddImage(
        atlas->TexID,
        ImVec2(a.x + split, a.y),
        ImVec2(a.x + w, a.y + h),
        ImVec2(uv_split, uv0.y),
        uv1,
        name
    );
}

void focus_ring(ImDrawList* list, ImVec2 a, ImVec2 b, float round) {
    const bool focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
    const float shown = tween(item_part("focus"), focused ? 1.0f : 0.0f, kFast);
    if (shown > 0.0f) {
        list->AddRect(a, b, ui_colors::fade(ui().amber, shown), round, 0, dp(2.0f));
    }
}

void primary_fill(ImDrawList* list, ImVec2 a, ImVec2 b, float round, float held) {
    const int first = list->VtxBuffer.Size;
    list->AddRectFilled(a, b, IM_COL32_WHITE, round);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(
        list,
        first,
        list->VtxBuffer.Size,
        ImVec2(a.x, a.y),
        ImVec2(a.x, b.y),
        ui_colors::mix(ui().bar_top, ui().ink_deep, 0.18f * held),
        ui_colors::mix(ui().bar_bottom, ui().ink_deep, 0.18f * held)
    );
    list->AddLine(
        ImVec2(a.x + round, a.y + 1.0f),
        ImVec2(b.x - round, a.y + 1.0f),
        IM_COL32(255, 255, 255, 90)
    );
}

void arrow(ImDrawList* list, ImVec2 tip, float dx, float dy, ImU32 ink) {
    const float arm = dp(5.0f);
    const ImVec2 back(tip.x - dx * arm, tip.y - dy * arm);
    const ImVec2 points[3] =
        {ImVec2(back.x - dy * arm, back.y - dx * arm),
         tip,
         ImVec2(back.x + dy * arm, back.y + dx * arm)};
    list->AddPolyline(points, 3, ink, ImDrawFlags_None, dp(2.0f));
}

void card_begin() {
    ImDrawList* list = ImGui::GetWindowDrawList();
    list->ChannelsSplit(2);
    list->ChannelsSetCurrent(1);
    s.card = {ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail().x, 0};
}

void card_end() {
    ImDrawList* list = ImGui::GetWindowDrawList();
    const ImVec2 end(s.card.top.x + s.card.width, ImGui::GetCursorScreenPos().y);
    list->ChannelsSetCurrent(0);
    list->AddRectFilled(s.card.top, end, ui().panel_hi, dp(16.0f));
    list->AddRect(s.card.top, end, ui().line, dp(16.0f));
    list->ChannelsMerge();
}

void menu_heading(const char* title) {
    ImGui::Dummy(ImVec2(0.0f, dp(18.0f)));
    ImFont* font = menu_fonts.head != nullptr ? menu_fonts.head : ImGui::GetFont();
    const float size =
        font == menu_fonts.head && font != nullptr ? font->FontSize : ImGui::GetFontSize() * 1.05f;
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()
        ->AddText(font, size, ImVec2(at.x + dp(4.0f), at.y), ui().gold, title);
    ImGui::Dummy(ImVec2(0.0f, size + dp(8.0f)));
}

bool menu_row(const char* label, ImU32 ink, const RowRight& right) {
    const float height = dp(s.menu_row_height);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* list = ImGui::GetWindowDrawList();
    if (s.card.rows++ > 0) {
        list->AddLine(
            ImVec2(at.x + dp(16.0f), at.y), ImVec2(at.x + width - dp(16.0f), at.y), ui().line
        );
    }
    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("row", ImVec2(width, height));
    ImGui::PopID();
    const ImVec2 a(at.x + dp(3.0f), at.y + dp(3.0f));
    const ImVec2 b(at.x + width - dp(3.0f), at.y + height - dp(3.0f));
    const float held = tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast);
    // A pointer over it (a desktop's): the accent, fainter than a press.
    const float hover = tween(item_part("hover"), ImGui::IsItemHovered() ? 1.0f : 0.0f, kFast);
    if (held > 0.0f || hover > 0.0f) {
        list->AddRectFilled(
            a, b, ui_colors::fade(pressed_fill(), std::max(held, hover * 0.5f)), dp(10.0f)
        );
    }
    focus_ring(list, a, b, dp(10.0f));
    list->AddText(
        ImVec2(at.x + dp(16.0f), at.y + (height - ImGui::GetFontSize()) * 0.5f), ink, label
    );
    if (right) {
        right(list, ImVec2(at.x + width - dp(16.0f), at.y + height * 0.5f));
    }
    return pressed;
}

bool menu_row(const char* label, ImU32 ink) {
    return menu_row(label, ink, RowRight());
}

bool menu_switch(const char* label, bool on) {
    return menu_row(label, ui().text, [on](ImDrawList* list, ImVec2 right) {
        const float k = tween(item_part("switch"), on ? 1.0f : 0.0f, kMedium);
        const float h = dp(26.0f);
        const ImVec2 a(right.x - dp(46.0f), right.y - h * 0.5f);
        const ImVec2 b(right.x, right.y + h * 0.5f);
        list->AddRectFilled(a, b, ui_colors::mix(ui().ink_deep, ui().brand, k), h * 0.5f);
        list->AddRect(a, b, ui_colors::mix(ui().line_strong, ui().brand, k), h * 0.5f);
        const float knob = h * 0.5f - dp(4.0f);
        const float from = a.x + h * 0.5f;
        const float to = b.x - h * 0.5f;
        list->AddCircleFilled(
            ImVec2(from + (to - from) * k, right.y), knob, ui_colors::mix(ui().dim, ui().text, k)
        );
    });
}

int menu_choice(const char* label, const std::string& value, bool can_lower, bool can_raise) {
    float back_until = 0.0f;  // a press left of this moves the value back
    const bool pressed = menu_row(label, ui().text, [&](ImDrawList* list, ImVec2 right) {
        // The values of every row in one column, centred between its arrows.
        const float column = std::min(s.card.width * 0.48f, dp(190.0f));
        const float left = right.x - column;
        const ImU32 off = ui_colors::fade(ui().dim, 0.35f);
        arrow(list, ImVec2(left, right.y), -1.0f, 0.0f, can_lower ? ui().amber : off);
        arrow(list, ImVec2(right.x, right.y), 1.0f, 0.0f, can_raise ? ui().amber : off);
        const ImVec2 size = ImGui::CalcTextSize(value.c_str());
        list->AddText(
            ImVec2(left + (column - size.x) * 0.5f, right.y - size.y * 0.5f),
            ui().gold,
            value.c_str()
        );
        back_until = left + column * 0.35f;
    });
    int by = 0;
    if (pressed) {
        by = ImGui::IsMouseReleased(0) && ImGui::GetIO().MousePos.x < back_until ? -1 : 1;
    }
    if (ImGui::IsItemFocused()) {
        const auto held = [](ImGuiKey pad, ImGuiKey stick, ImGuiKey key) {
            return ImGui::IsKeyPressed(pad) || ImGui::IsKeyPressed(stick)
                   || ImGui::IsKeyPressed(key);
        };
        if (held(ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadLStickLeft, ImGuiKey_LeftArrow)) {
            by = -1;
        } else if (held(
                       ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadLStickRight, ImGuiKey_RightArrow
                   )) {
            by = 1;
        }
    }
    return (by < 0 && !can_lower) || (by > 0 && !can_raise) ? 0 : by;
}

bool menu_button(const char* label) {
    const float height = dp(46.0f);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + ImGui::GetContentRegionAvail().x, a.y + height);
    const bool pressed = ImGui::InvisibleButton(label, ImVec2(b.x - a.x, height));
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float round = dp(12.0f);
    primary_fill(
        list, a, b, round, tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast)
    );
    focus_ring(
        list,
        ImVec2(a.x - dp(3.0f), a.y - dp(3.0f)),
        ImVec2(b.x + dp(3.0f), b.y + dp(3.0f)),
        round + dp(3.0f)
    );
    const ImVec2 size = ImGui::CalcTextSize(label);
    list->AddText(
        ImVec2((a.x + b.x - size.x) * 0.5f, a.y + (height - size.y) * 0.5f), ui().bar_ink, label
    );
    return pressed;
}

void menu_note(const char* line) {
    ImGui::Dummy(ImVec2(0.0f, dp(10.0f)));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dp(16.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ui().soft));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - dp(16.0f));
    ImGui::TextUnformatted(line);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, dp(10.0f)));
    ++s.card.rows;
}

// Opening pauses the game, as an emulator's menu does; closing leaves it as Keep paused says
// (paused as before it opened, unless changed there).
void set_menu(bool open) {
    if (open == s.menu_open) {
        return;
    }
    s.menu_open = open;
    if (open) {
        s.keep_paused = s.paused;
        s.paused = true;
        s.held_frames = 0;
        s.menu_focus = true;
        s.menu_scroll.velocity = 0.0f;  // a flick from the last time does not glide on
    } else {
        s.paused = s.keep_paused;
    }
}

// The menu is out of the picture: what waited for that.
void menu_closed() {
    s.menu_confirm_reset = false;
    if (s.shot_when_shut) {
        s.shot_when_shut = false;
        s.shot_path = shot_name(s.tick);
    }
}

}  // namespace openrac::frontend::debug_overlay
