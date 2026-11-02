// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu on a desktop (debug/overlay.h): the menu key (` by default, the controller's R4) slides
// a bar of tabs down from the window's top, in the launcher's header's look (its striped gold, the
// logo, round tabs), and under it a window with the open tab's entries (menu_catalog.h) on cards,
// drawn by debug/menu_ui.h as the touch screen's drawer draws them, and beside them what the entry
// under the pointer or the focus does. The game waits while it is open.

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <imgui.h>
#include <imgui_internal.h>  // ShadeVertsLinearColorGradientKeepAlpha
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"

namespace openrac::frontend::debug_overlay {

namespace {

// A tab of the bar: one of the catalog's, or a mod's menu (mods::developer_menus).
struct BarTab {
    std::string name;
    int catalog = -1;  // menu::Tab, or -1
    int mod = -1;      // the mod menu's index, or -1
};

// The tabs with something to show now: the catalog's in order, then the mods'.
std::vector<BarTab> bar_tabs() {
    std::vector<BarTab> tabs;
    for (int t = 0; t < static_cast<int>(menu::Tab::Program); ++t) {
        if (tab_shown(static_cast<menu::Tab>(t), menu::kDesktop)) {
            tabs.push_back({menu::tab_name(static_cast<menu::Tab>(t)), t, -1});
        }
    }
    for (int i = 0; i < mod_menu_count(); ++i) {
        tabs.push_back({mod_menu_title(i), -1, i});
    }
    return tabs;
}

// The bar's stripes, as the launcher's header has them: wide bands of two close golds at 35
// degrees, the picked game's own when it has an accent (the launcher's palettes.json).
void stripes(ImDrawList* list, ImVec2 a, ImVec2 b) {
    const ImU32 one = ui().bar_stripe;
    const ImU32 two = ui().bar_stripe_alt;
    list->AddRectFilled(a, b, one);
    const float band = dp(20.0f);
    const float lean = (b.y - a.y) * std::tan(35.0f * 3.14159265f / 180.0f);
    list->PushClipRect(a, b, true);
    for (float x = a.x - lean; x < b.x + lean; x += band * 2.0f) {
        list->AddQuadFilled(
            ImVec2(x + band, a.y),
            ImVec2(x + band * 2.0f, a.y),
            ImVec2(x + band * 2.0f - lean, b.y),
            ImVec2(x + band - lean, b.y),
            two
        );
    }
    list->PopClipRect();
    // Its light top edge and dark bottom one, and a shadow over the game below.
    list->AddLine(ImVec2(a.x, a.y + 0.5f), ImVec2(b.x, a.y + 0.5f), IM_COL32(255, 255, 255, 90));
    list->AddLine(ImVec2(a.x, b.y - 0.5f), ImVec2(b.x, b.y - 0.5f), IM_COL32(0, 0, 0, 76));
    const int first = list->VtxBuffer.Size;
    list->AddRectFilled(ImVec2(a.x, b.y), ImVec2(b.x, b.y + dp(18.0f)), IM_COL32_WHITE);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(
        list,
        first,
        list->VtxBuffer.Size,
        ImVec2(a.x, b.y),
        ImVec2(a.x, b.y + dp(18.0f)),
        IM_COL32(0, 0, 0, 110),
        IM_COL32(0, 0, 0, 0)
    );
}

// A round tab on the bar: true when clicked. The open one wears the dark pill (drawn by the bar,
// sliding from tab to tab), its name in the accent.
bool bar_tab(const char* name, bool open, float height) {
    ImFont* font = menu_fonts.strong;
    ImGui::PushFont(font);
    const ImVec2 text = ImGui::CalcTextSize(name);
    const ImVec2 size(text.x + dp(28.0f), height);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(name, size);
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float hover =
        tween(item_part("hover"), ImGui::IsItemHovered() && !open ? 1.0f : 0.0f, kFast);
    if (hover > 0.0f) {
        list->AddRectFilled(
            a,
            ImVec2(a.x + size.x, a.y + size.y),
            IM_COL32(0, 0, 0, static_cast<int>(26 * hover)),
            size.y * 0.5f
        );
    }
    const float on = tween(item_part("on"), open ? 1.0f : 0.0f, kMedium);
    list->AddText(
        ImVec2(a.x + dp(14.0f), a.y + (size.y - text.y) * 0.5f),
        ui_colors::mix(ui().bar_label, ui().amber, on),
        name
    );
    ImGui::PopFont();
    return pressed;
}

// A chip on the bar's right: dark, a hairline, its text; true when clicked.
bool bar_chip(const char* label, ImU32 ink, bool light, float height) {
    ImGui::PushFont(menu_fonts.strong);
    const ImVec2 text = ImGui::CalcTextSize(label);
    const ImVec2 size(text.x + dp(28.0f), height);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, size);
    const ImVec2 b(a.x + size.x, a.y + size.y);
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float hover = tween(item_part("hover"), ImGui::IsItemHovered() ? 1.0f : 0.0f, kFast);
    const ImU32 fill = light ? ui_colors::mix(IM_COL32_WHITE, ui().gold, 0.25f * hover)
                             : ui_colors::mix(ui().ink_deep, ui().panel_hi, hover);
    list->AddRectFilled(a, b, fill, size.y * 0.5f);
    if (!light) {
        list->AddRect(a, b, ui_colors::fade(ui().line, 1.0f), size.y * 0.5f);
    }
    list->AddText(ImVec2(a.x + dp(14.0f), a.y + (size.y - text.y) * 0.5f), ink, label);
    ImGui::PopFont();
    return pressed;
}

// The right side of the window: what the entry under the pointer or the focus is and does.
void help_pane(ImVec2 a, ImVec2 b, bool mod_tab) {
    ImDrawList* list = ImGui::GetWindowDrawList();
    list->AddLine(ImVec2(a.x, a.y + dp(20.0f)), ImVec2(a.x, b.y - dp(20.0f)), ui().line);
    const float pad = dp(28.0f);
    float y = a.y + dp(30.0f);
    const float wrap = b.x - a.x - pad * 2.0f;
    if (s.menu_hover >= 0 && s.menu_hover < menu::kIds) {
        const auto id = static_cast<menu::Id>(s.menu_hover);
        const std::string label = entry_label(id, menu::kDesktop);
        ImFont* head = menu_fonts.head;
        list->AddText(
            head, head->FontSize, ImVec2(a.x + pad, y), ui().gold, label.c_str(), nullptr, wrap
        );
        y += head->CalcTextSizeA(head->FontSize, FLT_MAX, wrap, label.c_str()).y + dp(14.0f);
        ImFont* body = menu_fonts.body;
        const char* help = menu::entry(id).help;
        list->AddText(body, body->FontSize, ImVec2(a.x + pad, y), ui().soft, help, nullptr, wrap);
    } else {
        ImFont* body = menu_fonts.body;
        const char* line = mod_tab ? "A mod's own menu: its items do what the mod says they do."
                                   : "Point at an option, or move to it with the arrow keys or a "
                                     "controller, to see what it does here.";
        list->AddText(body, body->FontSize, ImVec2(a.x + pad, y), ui().dim, line, nullptr, wrap);
    }
    // At its foot: what draws the picture, and how to get back to the game.
    ImFont* body = menu_fonts.body;
    const std::string keys = menu_key_label() + ", Esc or B: back to the game";
    const float line = body->FontSize * 1.35f;
    list->AddText(
        body,
        body->FontSize,
        ImVec2(a.x + pad, b.y - pad - line * 2.0f),
        ui().dim,
        menu_graphics_line().c_str(),
        nullptr,
        wrap
    );
    list->AddText(
        body,
        body->FontSize,
        ImVec2(a.x + pad, b.y - pad - line),
        ui().dim,
        keys.c_str(),
        nullptr,
        wrap
    );
}

}  // namespace

void menu_window(const Shown& shown) {
    const ImGuiIO& io = ImGui::GetIO();
    const float target = s.menu_open ? 1.0f : 0.0f;
    const float move = io.DeltaTime / (s.menu_open ? kMedium : 0.2f);
    s.menu_at = s.menu_at < target ? std::min(s.menu_at + move, target)
                                   : std::max(s.menu_at - move, target);
    if (s.menu_at <= 0.0f) {
        menu_closed();
        return;
    }
    (void)shown;
    s.menu_row_height = 42.0f;
    const float ease = 1.0f - std::pow(1.0f - s.menu_at, 4.0f);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewport->Pos,
        ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y),
        ui_colors::fade(ui().ink_deep, 0.55f * ease)
    );

    const std::vector<BarTab> tabs = bar_tabs();
    if (tabs.empty()) {
        return;
    }
    s.menu_tab = std::clamp(s.menu_tab, 0, static_cast<int>(tabs.size()) - 1);
    // Tabs back and on: the controller's shoulder buttons, Page Up and Page Down.
    int switch_by = 0;
    if (s.menu_open) {
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)
            || ImGui::IsKeyPressed(ImGuiKey_PageUp, false)) {
            switch_by = -1;
        } else if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)
                   || ImGui::IsKeyPressed(ImGuiKey_PageDown, false)) {
            switch_by = 1;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
            set_menu(false);
        }
    }

    // ---- The bar ----
    const float bar = dp(56.0f);
    const ImVec2 bar_a(viewport->Pos.x, viewport->Pos.y - bar * (1.0f - ease));
    ImGui::SetNextWindowPos(bar_a);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, bar));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(18.0f), 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(dp(4.0f), 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    const ImGuiWindowFlags fixed = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin(
            "OpenRAC menu bar",
            nullptr,
            fixed | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing
        )) {
        ImDrawList* list = ImGui::GetWindowDrawList();
        list->PushClipRectFullScreen();
        stripes(list, bar_a, ImVec2(bar_a.x + viewport->Size.x, bar_a.y + bar));
        list->PopClipRect();
        // The logo in the bar's ink, as on the launcher's desktop header.
        draw_logo(
            list, ImVec2(bar_a.x + dp(18.0f), bar_a.y + bar * 0.5f), ui().ink_deep, ui().ink_deep
        );
        const float logo_w = s.logo_rect >= 0
                                 ? ImGui::GetIO().Fonts->GetCustomRectByIndex(s.logo_rect)->Width
                                 : dp(120.0f);
        const float tab_h = dp(32.0f);
        ImGui::SetCursorScreenPos(
            ImVec2(bar_a.x + dp(18.0f) + logo_w + dp(22.0f), bar_a.y + (bar - tab_h) * 0.5f)
        );
        // The open tab's dark pill, sliding from tab to tab.
        const ImVec2 pill_at = ImGui::GetCursorScreenPos();
        list->ChannelsSplit(2);
        list->ChannelsSetCurrent(1);
        float pill_x = pill_at.x;
        float pill_w = 0.0f;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
            if (i > 0) {
                ImGui::SameLine();
            }
            const float x = ImGui::GetCursorScreenPos().x;
            if (bar_tab(tabs[static_cast<std::size_t>(i)].name.c_str(), i == s.menu_tab, tab_h)) {
                s.menu_tab = i;
                s.menu_hover = -1;
                s.menu_focus = true;
            }
            if (i == s.menu_tab) {
                pill_x = x;
                pill_w = ImGui::GetItemRectSize().x;
            }
        }
        list->ChannelsSetCurrent(0);
        const float px = tween(ImGui::GetID("pill x"), pill_x, kMedium);
        const float pw = tween(ImGui::GetID("pill w"), pill_w, kMedium);
        list->AddRectFilled(
            ImVec2(px, pill_at.y), ImVec2(px + pw, pill_at.y + tab_h), ui().ink_deep, tab_h * 0.5f
        );
        list->ChannelsMerge();

        // At the right: Quit, and Resume as the launcher's white button.
        ImGui::PushFont(menu_fonts.strong);
        const float quit_w = ImGui::CalcTextSize(menu::entry(menu::Id::Quit).label).x + dp(28.0f);
        const float resume_w = ImGui::CalcTextSize("Resume").x + dp(28.0f);
        ImGui::PopFont();
        const float right = bar_a.x + viewport->Size.x - dp(18.0f);
        ImGui::SetCursorScreenPos(
            ImVec2(right - resume_w - dp(8.0f) - quit_w, bar_a.y + (bar - tab_h) * 0.5f)
        );
        if (bar_chip(menu::entry(menu::Id::Quit).label, ui().err, false, tab_h)) {
            press_entry(menu::Id::Quit);
        }
        ImGui::SameLine(0.0f, dp(8.0f));
        if (bar_chip("Resume", ui().ink_deep, true, tab_h)) {
            set_menu(false);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);

    if (switch_by != 0) {
        s.menu_tab = (s.menu_tab + switch_by + static_cast<int>(tabs.size()))
                     % static_cast<int>(tabs.size());
        s.menu_hover = -1;
        s.menu_focus = true;
    }
    const BarTab& tab = tabs[static_cast<std::size_t>(s.menu_tab)];

    // ---- The window ----
    const float margin = dp(32.0f);
    const float width = std::min(viewport->Size.x - margin * 2.0f, dp(1060.0f));
    const float top = viewport->Pos.y + bar + dp(24.0f);
    const float height = std::min(viewport->Pos.y + viewport->Size.y - top - margin, dp(700.0f));
    if (height < dp(120.0f)) {
        return;
    }
    // In from a little above, as the launcher's pages come.
    const ImVec2 at(
        viewport->Pos.x + (viewport->Size.x - width) * 0.5f, top - dp(12.0f) * (1.0f - ease)
    );
    ImGui::SetNextWindowPos(at);
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowBgAlpha(0.0f);
    if (s.menu_focus) {
        ImGui::SetNextWindowFocus();
    }
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, dp(5.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImGui::ColorConvertU32ToFloat4(ui().line_strong));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushFont(menu_fonts.body);
    if (ImGui::Begin(
            "OpenRAC menu",
            nullptr,
            fixed | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        )) {
        ImDrawList* list = ImGui::GetWindowDrawList();
        const ImVec2 b(at.x + width, at.y + height);
        // The launcher's panel: a shadow, its ink, a hairline border.
        for (int i = 1; i <= 4; ++i) {
            const float grow = dp(4.0f) * static_cast<float>(i);
            list->AddRectFilled(
                ImVec2(at.x - grow, at.y - grow + dp(6.0f)),
                ImVec2(b.x + grow, b.y + grow + dp(6.0f)),
                IM_COL32(0, 0, 0, 22),
                dp(16.0f) + grow
            );
        }
        list->AddRectFilled(at, b, ui().panel, dp(16.0f));
        list->AddRect(at, b, ui().line, dp(16.0f));

        const float rows_w = std::floor(width * 0.6f);
        ImGui::SetCursorScreenPos(at);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(24.0f), dp(6.0f)));
        if (ImGui::BeginChild(
                "rows",
                ImVec2(rows_w, height),
                ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened
            )) {
            if (tab.catalog >= 0) {
                draw_tab(static_cast<menu::Tab>(tab.catalog), menu::kDesktop);
            } else {
                draw_mod_menu(tab.mod);
            }
            ImGui::Dummy(ImVec2(0.0f, dp(20.0f)));
            if (s.menu_focus) {
                s.menu_focus = false;
                ImGui::SetScrollY(0.0f);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        help_pane(ImVec2(at.x + rows_w, at.y), b, tab.mod >= 0);
    }
    ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(6);
}

}  // namespace openrac::frontend::debug_overlay
