// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu on a desktop (debug/overlay.h): the menu key (` by default, the controller's R4) slides
// a bar of tabs down from the window's top, in the launcher's header's look (its striped gold, the
// logo, round tabs), and under it a window with the open tab's entries (menu_catalog.h) on cards,
// drawn by debug/menu_ui.h as the touch screen's drawer draws them, and beside them what the entry
// under the pointer or the focus does. The game waits while it is open.

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <format>
#include <imgui.h>
#include <imgui_internal.h>  // ShadeVertsLinearColorGradientKeepAlpha
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "common/build_info.h"
#include "debug/menu_ui.h"
#include "openrac/game_host.h"

namespace openrac::frontend::debug_overlay {

namespace {

// How long the controller's quit button is held to quit, in seconds.
constexpr float kQuitHold = 1.0f;

// A tab of the bar: one of the catalog's, or a mod's menu (mods::developer_menus).
struct BarTab {
    std::string name;
    int catalog = -1;  // menu::Tab, or -1
    int mod = -1;      // the mod menu's index, or -1
    int chip = 0;      // Chip: one of the bar's buttons, or 0
};

// After the tabs, L1 and R1 also reach the bar's two buttons at its right: Quit game (its page
// asks first) and Resume.
enum Chip : int {
    kQuitPage = 1,
    kResumePage = 2
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

// Where the bar's stripes start this frame (a band of the first gold): its buttons' stripes, in
// coal, line up with them, as the launcher's are laid on the window.
ImVec2 bar_origin(0.0f, 0.0f);

// The bar's buttons: the bar's stripes continued in coal (the launcher's --coal-stripes), lighter
// under the pointer and for the open tab (--coal-stripes-hi).
constexpr ImU32 kCoal = IM_COL32(0x18, 0x18, 0x18, 255);
constexpr ImU32 kCoalAlt = IM_COL32(0x12, 0x12, 0x12, 255);
constexpr ImU32 kCoalHi = IM_COL32(0x25, 0x25, 0x25, 255);
constexpr ImU32 kCoalHiAlt = IM_COL32(0x1f, 0x1f, 0x1f, 255);

// The bar's stripes, as the launcher's header has them: wide bands of two close golds at 35
// degrees, the picked game's own when it has an accent (the launcher's palettes.json).
void stripes(ImDrawList* list, ImVec2 a, ImVec2 b) {
    const ImU32 one = ui().bar_stripe;
    const ImU32 two = ui().bar_stripe_alt;
    list->AddRectFilled(a, b, one);
    const float band = dp(20.0f);
    const float lean = (b.y - a.y) * std::tan(35.0f * 3.14159265f / 180.0f);
    list->PushClipRect(a, b, true);
    // They drift slowly right without end: one band pair every few seconds, wrapping round.
    const float period = band * 2.0f;
    const float drift = std::fmod(static_cast<float>(ImGui::GetTime()) * dp(8.0f), period);
    bar_origin = ImVec2(a.x + drift, a.y);
    for (float x = a.x - lean - period + drift; x < b.x + lean; x += period) {
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
    const ImU32 dark = IM_COL32(0, 0, 0, 110);
    const ImU32 none = IM_COL32(0, 0, 0, 0);
    list->AddRectFilledMultiColor(
        ImVec2(a.x, b.y), ImVec2(b.x, b.y + dp(18.0f)), dark, dark, none, none
    );
}

// Bands of `ink` at the bar's lean over [a, b], `strength` at its top left corner and fading out
// to none down and to the right: the bar's stripes, barely there, in the corner of the window's
// side.
void faint_stripes(ImDrawList* list, ImVec2 a, ImVec2 b, ImU32 ink, float strength) {
    const float band = dp(20.0f);
    const float lean = (b.y - a.y) * std::tan(35.0f * 3.14159265f / 180.0f);
    const ImU32 top = ui_colors::fade(ink, strength);
    const ImU32 bottom = ui_colors::fade(ink, 0.0f);
    list->PushClipRect(a, b, true);
    for (float x = a.x - lean; x < b.x + lean; x += band * 2.0f) {
        const int first = list->VtxBuffer.Size;
        list->AddQuadFilled(
            ImVec2(x + band, a.y),
            ImVec2(x + band * 2.0f, a.y),
            ImVec2(x + band * 2.0f - lean, b.y),
            ImVec2(x + band - lean, b.y),
            IM_COL32_WHITE
        );
        // Each corner's colour by its height: the top's, fading to none at the bottom.
        for (int v = first; v < list->VtxBuffer.Size; ++v) {
            ImDrawVert& vert = list->VtxBuffer[v];
            const float down = std::clamp((vert.pos.y - a.y) / (b.y - a.y), 0.0f, 1.0f);
            const float across = std::clamp((vert.pos.x - a.x) / (b.x - a.x), 0.0f, 1.0f);
            vert.col = ui_colors::mix(top, bottom, std::min(1.0f, down + across));
        }
    }
    list->PopClipRect();
}

// A tab on the bar, a button of its own: coal stripes continuing the bar's, gold text, lighter
// under the pointer, no edge; the open one is the lighter chip under it (drawn by the bar, sliding
// from tab to tab), its name in light gold. True when clicked.
bool bar_tab(const char* name, bool open, float height) {
    ImFont* font = menu_fonts.bar;
    ImGui::PushFont(font);
    const ImVec2 text = ImGui::CalcTextSize(name);
    const ImVec2 size(text.x + dp(28.0f), height);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(name, size);
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float hover =
        tween(item_part("hover"), ImGui::IsItemHovered() && !open ? 1.0f : 0.0f, kFast);
    const float on = tween(item_part("on"), open ? 1.0f : 0.0f, kMedium);
    if (on < 1.0f) {
        const ImVec2 b(a.x + size.x, a.y + size.y);
        striped_rect(list, a, b, dp(6.0f), kCoal, kCoalAlt, bar_origin, 1.0f - on);
        striped_rect(list, a, b, dp(6.0f), kCoalHi, kCoalHiAlt, bar_origin, hover * (1.0f - on));
    }
    list->AddText(
        centred_text(font, font->FontSize, name, ImVec2(a.x + size.x * 0.5f, a.y + size.y * 0.5f)),
        ui_colors::mix(ui().amber, ui().gold, on),
        name
    );
    ImGui::PopFont();
    return pressed;
}

// A chip on the bar's right: a bar button (coal stripes), no border, 6 px corners, its text; true
// when clicked.
bool bar_chip(
    const char* label, ImU32 ink, bool light, float height, float held = 0.0f, bool picked = false
) {
    ImGui::PushFont(menu_fonts.bar);
    const ImVec2 text = ImGui::CalcTextSize(label);
    const ImVec2 size(text.x + dp(28.0f), height);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, size);
    const ImVec2 b(a.x + size.x, a.y + size.y);
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float hover = tween(item_part("hover"), ImGui::IsItemHovered() ? 1.0f : 0.0f, kFast);
    const float round = dp(6.0f);
    if (light) {
        list->AddRectFilled(a, b, ui_colors::mix(IM_COL32_WHITE, ui().gold, 0.25f * hover), round);
    } else {
        // A bar button: coal stripes continuing the bar's, no edge.
        striped_rect(list, a, b, round, kCoal, kCoalAlt, bar_origin);
        striped_rect(list, a, b, round, kCoalHi, kCoalHiAlt, bar_origin, hover);
    }
    if (held > 0.0f) {
        // Held on the controller: filling up to the moment it acts.
        list->PushClipRect(a, ImVec2(a.x + size.x * held, b.y), true);
        list->AddRectFilled(a, b, ui_colors::fade(ink, 0.35f), round);
        list->PopClipRect();
    }
    // Reached with L1/R1: a ring around it, as the open tab has its pill.
    const float ring = tween(item_part("picked"), picked ? 1.0f : 0.0f, kFast);
    if (ring > 0.0f) {
        const float out = dp(3.0f);
        list->AddRect(
            ImVec2(a.x - out, a.y - out),
            ImVec2(b.x + out, b.y + out),
            ui_colors::fade(ui().ink_deep, ring),
            round + out,
            ImDrawFlags_None,
            dp(2.0f)
        );
    }
    list->AddText(
        centred_text(
            menu_fonts.bar,
            menu_fonts.bar->FontSize,
            label,
            ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f)
        ),
        ink,
        label
    );
    ImGui::PopFont();
    return pressed;
}

// Text in `font` at `at`, wrapped at `wrap`: the height it took.
float text_block(
    ImDrawList* list, ImFont* font, ImVec2 at, ImU32 ink, const char* text, float wrap
) {
    list->AddText(font, font->FontSize, at, ink, text, nullptr, wrap);
    return font->CalcTextSizeA(font->FontSize, FLT_MAX, wrap, text).y;
}

// Small capitals with room between the letters, as the launcher's section numbers and kickers;
// in the accent's light amber, which reads on the dark panels for every game.
void kicker(ImDrawList* list, ImVec2 at, ImU32 ink, const std::string& text) {
    ImFont* font = menu_fonts.small;
    const float gap = dp(1.6f);
    float x = at.x;
    for (const char c : text) {
        const char up[2] = {static_cast<char>(std::toupper(static_cast<unsigned char>(c))), 0};
        list->AddText(font, font->FontSize, ImVec2(x, at.y), ink, up);
        x += font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, up).x + gap;
    }
}

// The right side of the window: what the entry under the pointer or the focus is and does, on a
// darker ground with the bar's stripes, barely there, in its top left corner.
void help_pane(ImVec2 a, ImVec2 b, bool mod_tab, int chip) {
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float round = dp(12.0f);
    list->AddRectFilled(
        a, b, ui_colors::fade(ui().ink_deep, 0.55f), round, ImDrawFlags_RoundCornersRight
    );
    faint_stripes(
        list,
        ImVec2(a.x, a.y + 1.0f),
        ImVec2(std::min(b.x - 1.0f, a.x + dp(300.0f)), a.y + dp(170.0f)),
        ui().amber,
        0.05f
    );
    list->AddLine(ImVec2(a.x, a.y), ImVec2(a.x, b.y), ui().line);
    const float pad = dp(30.0f);
    // A new entry's help rises into place and fades in.
    MenuMotion& m = s.menu_motion;
    if (m.help_of != s.menu_hover) {
        m.help_of = s.menu_hover;
        m.help = 0.0f;
    }
    m.help = std::min(1.0f, m.help + ImGui::GetIO().DeltaTime / kMedium);
    const float in = 1.0f - std::pow(1.0f - m.help, 3.0f);
    const auto ink = [in](ImU32 c) {
        return ui_colors::fade(c, 0.2f + 0.8f * in);
    };
    float y = a.y + dp(34.0f) + dp(8.0f) * (1.0f - in);
    const float wrap = b.x - a.x - pad * 2.0f;
    if (chip == kQuitPage) {
        // The Quit page's question, where an option's details are.
        kicker(list, ImVec2(a.x + pad, y), ink(ui().amber), menu::entry(menu::Id::Quit).label);
        y += menu_fonts.small->FontSize + dp(12.0f);
        y += text_block(
            list, menu_fonts.head, ImVec2(a.x + pad, y), ink(ui().gold), "Quit the game?", wrap
        );
        y += dp(12.0f);
        list->AddRectFilled(
            ImVec2(a.x + pad, y),
            ImVec2(a.x + pad + dp(36.0f), y + dp(3.0f)),
            ink(ui().amber),
            dp(1.5f)
        );
        y += dp(18.0f);
        text_block(
            list,
            menu_fonts.body,
            ImVec2(a.x + pad, y),
            ink(ui().amber),
            "Progress since the last save is lost.",
            wrap
        );
    } else if (s.menu_hover >= 0 && s.menu_hover < menu::kIds) {
        const auto id = static_cast<menu::Id>(s.menu_hover);
        const menu::Entry& e = menu::entry(id);
        kicker(
            list,
            ImVec2(a.x + pad, y),
            ink(ui().amber),
            std::string(menu::tab_name(e.tab)) + "  /  " + e.section
        );
        y += menu_fonts.small->FontSize + dp(12.0f);
        const std::string label = entry_label(id, menu::kDesktop);
        y += text_block(
            list, menu_fonts.head, ImVec2(a.x + pad, y), ink(ui().gold), label.c_str(), wrap
        );
        y += dp(12.0f);
        list->AddRectFilled(
            ImVec2(a.x + pad, y),
            ImVec2(a.x + pad + dp(36.0f), y + dp(3.0f)),
            ink(ui().amber),
            dp(1.5f)
        );
        y += dp(18.0f);
        text_block(list, menu_fonts.body, ImVec2(a.x + pad, y), ink(ui().soft), e.help, wrap);
    } else {
        const char* line = mod_tab ? "A mod's own menu: its items do what the mod says they do."
                                   : "Point at an option, or move to it with the arrow keys or a "
                                     "controller, to see what it does here.";
        text_block(list, menu_fonts.body, ImVec2(a.x + pad, y), ink(ui().dim), line, wrap);
    }
    // At its foot, over a hairline: what draws the picture.
    ImFont* body = menu_fonts.body;
    const float line = body->FontSize * 1.4f;
    const float foot = b.y - pad - line;
    list->AddLine(
        ImVec2(a.x + pad, foot - dp(14.0f)), ImVec2(b.x - pad, foot - dp(14.0f)), ui().line
    );
    list->AddText(
        body,
        body->FontSize,
        ImVec2(a.x + pad, foot),
        ui().dim,
        menu_graphics_line().c_str(),
        nullptr,
        wrap
    );
}

// The open tab's name over its rows, large, with its place among the tabs before it.
void tab_title(const std::string& name, int index, int count) {
    ImDrawList* list = ImGui::GetWindowDrawList();
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(0.0f, dp(22.0f)));
    const std::string place = std::format("{:02} / {:02}", index + 1, count);
    kicker(list, ImVec2(at.x + dp(4.0f), at.y + dp(22.0f)), ui().amber, place);
    ImFont* title = menu_fonts.title;
    list->AddText(
        title,
        title->FontSize,
        ImVec2(at.x + dp(2.0f), at.y + dp(22.0f) + menu_fonts.small->FontSize + dp(6.0f)),
        ui().text,
        name.c_str()
    );
    ImGui::Dummy(ImVec2(0.0f, menu_fonts.small->FontSize + dp(6.0f) + title->FontSize));
}

// Where the game comes from, at the screen's foot on the right while the menu is open: OpenRAC's
// version and commit, the game and its disc.
void version_corner(const ImGuiViewport* viewport, float shown) {
    ImDrawList* list = ImGui::GetForegroundDrawList();
    ImFont* font = menu_fonts.small;
    const std::string first =
        std::format("OpenRAC {}  ·  {}", build_info::version(), build_info::commit());
    const std::string second = std::format("{}  ·  {}", openrac_game.title, openrac_game.serial);
    const float pad = dp(18.0f);
    const float line = font->FontSize * 1.45f;
    const float right = viewport->Pos.x + viewport->Size.x - pad;
    const float foot = viewport->Pos.y + viewport->Size.y - pad;
    const ImU32 strong = ui_colors::fade(ui().soft, shown);
    const ImU32 quiet = ui_colors::fade(ui().dim, shown);
    const ImVec2 one = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, first.c_str());
    const ImVec2 two = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, second.c_str());
    list->AddText(font, font->FontSize, ImVec2(right - two.x, foot - line), strong, second.c_str());
    list->AddText(
        font, font->FontSize, ImVec2(right - one.x, foot - line * 2.0f), quiet, first.c_str()
    );
}

// The make of the first controller connected (SDL's), for the names its buttons wear.
SDL_GamepadType pad_type() {
    SDL_GamepadType type = SDL_GAMEPAD_TYPE_STANDARD;
    int count = 0;
    if (SDL_JoystickID* pads = SDL_GetGamepads(&count)) {
        if (count > 0) {
            type = SDL_GetGamepadTypeForID(pads[0]);
        }
        SDL_free(pads);
    }
    return type;
}

// A controller's button as it is printed on a pad of `type`: A, Cross, B ...; its shoulders as
// that make names them.
std::string pad_name(SDL_GamepadType type, SDL_GamepadButton button) {
    const bool sony = type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4
                      || type == SDL_GAMEPAD_TYPE_PS5;
    const bool nintendo = type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO
                          || type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
    switch (button) {
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return sony ? "L1" : nintendo ? "L" : "LB";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return sony ? "R1" : nintendo ? "R" : "RB";
        default:
            break;
    }
    switch (SDL_GetGamepadButtonLabelForType(type, button)) {
        case SDL_GAMEPAD_BUTTON_LABEL_A:
            return "A";
        case SDL_GAMEPAD_BUTTON_LABEL_B:
            return "B";
        case SDL_GAMEPAD_BUTTON_LABEL_X:
            return "X";
        case SDL_GAMEPAD_BUTTON_LABEL_Y:
            return "Y";
        case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
            return "Cross";
        case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
            return "Circle";
        case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
            return "Square";
        case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
            return "Triangle";
        default:
            return "?";
    }
}

// What the buttons do, along the screen's foot on the left while the menu is open: the
// controller's when it was the last used, else the keyboard's. Each as a key cap and its use.
void hints(const ImGuiViewport* viewport, float shown) {
    struct Hint {
        std::vector<std::string> keys;  // each its own cap
        const char* does;
    };

    const bool pad = s.menu_motion.pad;
    std::vector<Hint> list;
    if (pad) {
        const SDL_GamepadType type = pad_type();
        list = {
            {{pad_name(type, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER),
              pad_name(type, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)},
             "Tabs"},
            {{pad_name(type, SDL_GAMEPAD_BUTTON_SOUTH)}, "Select"},
            {{"\u2190", "\u2192"}, "Change"},
            {{pad_name(type, SDL_GAMEPAD_BUTTON_EAST)}, "Back to the game"},
            {{pad_name(type, SDL_GAMEPAD_BUTTON_NORTH)}, "Hold to quit"},
        };
    } else {
        list = {
            {{"PgUp", "PgDn"}, "Tabs"},
            {{"Enter"}, "Select"},
            {{"\u2190", "\u2192"}, "Change"},
            {{"Esc"}, "Back to the game"},
        };
    }
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImFont* body = menu_fonts.body;
    const float edge = dp(18.0f);
    float x = viewport->Pos.x + edge;
    const float middle = viewport->Pos.y + viewport->Size.y - edge - key_cap_height() * 0.5f;
    for (const Hint& hint : list) {
        for (const std::string& key : hint.keys) {
            x += key_cap(draw, x, middle, key, shown) + dp(4.0f);
        }
        const ImVec2 size = body->CalcTextSizeA(body->FontSize, FLT_MAX, 0.0f, hint.does);
        draw->AddText(
            body,
            body->FontSize,
            ImVec2(x + dp(4.0f), beside_caps(body, middle)),
            ui_colors::fade(ui().soft, shown),
            hint.does
        );
        x += dp(4.0f) + size.x + dp(22.0f);
    }
}

}  // namespace

void menu_window(const Shown& shown) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool was_out = s.menu_at > 0.0f;
    s.menu_at = slide(s.menu_at, s.menu_open);
    if (s.menu_at <= 0.0f) {
        // Once, as it shuts. The overlay also draws while the menu is shut (a notification is up),
        // and holding back every frame then ate the game's presses.
        if (was_out) {
            menu_closed();
        }
        return;
    }
    (void)shown;
    s.menu_row_height = 42.0f;
    const float ease = ease_out(s.menu_at);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    {
        // The game under it dimmed, more at the foot and the sides, so the window stands out of it.
        ImDrawList* back = ImGui::GetBackgroundDrawList();
        const ImVec2 a = viewport->Pos;
        const ImVec2 b(a.x + viewport->Size.x, a.y + viewport->Size.y);
        const ImU32 light = ui_colors::fade(ui().ink_deep, 0.45f * ease);
        const ImU32 heavy = ui_colors::fade(ui().ink_deep, 0.82f * ease);
        back->AddRectFilledMultiColor(a, b, light, light, heavy, heavy);
        const float side = viewport->Size.x * 0.22f;
        const ImU32 edge = ui_colors::fade(ui().ink_deep, 0.5f * ease);
        const ImU32 none = ui_colors::fade(ui().ink_deep, 0.0f);
        back->AddRectFilledMultiColor(a, ImVec2(a.x + side, b.y), edge, none, none, edge);
        back->AddRectFilledMultiColor(ImVec2(b.x - side, a.y), b, none, edge, edge, none);
    }
    version_corner(viewport, ease);
    hints(viewport, ease);

    std::vector<BarTab> tabs = bar_tabs();
    if (tabs.empty()) {
        return;
    }
    const int tab_count = static_cast<int>(tabs.size());  // the bar's tabs, before its buttons
    tabs.push_back({menu::entry(menu::Id::Quit).label, -1, -1, kQuitPage});
    tabs.push_back({"Resume", -1, -1, kResumePage});
    if (s.menu_want_tab >= 0) {
        for (std::size_t i = 0; i < tabs.size(); ++i) {
            if (tabs[i].catalog == s.menu_want_tab) {
                s.menu_tab = static_cast<int>(i);
                s.menu_focus = true;
            }
        }
        s.menu_want_tab = -1;
    }
    s.menu_tab = std::clamp(s.menu_tab, 0, static_cast<int>(tabs.size()) - 1);
    // The menu opens again at the tab it was on, not at a button's page.
    if (s.menu_tab < tab_count) {
        s.menu_motion.last_tab = s.menu_tab;
    } else if (!s.menu_open) {
        s.menu_tab = std::min(s.menu_motion.last_tab, tab_count - 1);
    }
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
        // B goes back to the game, as the menu's button does (debug_overlay.cpp); Start does not,
        // being the game's pause.
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
            set_menu(false);
        }
        MenuMotion& m = s.menu_motion;
        // Held for a second, Y is its own confirmation.
        m.quit_hold = ImGui::IsKeyDown(ImGuiKey_GamepadFaceUp) ? m.quit_hold + io.DeltaTime : 0.0f;
        if (m.quit_hold >= kQuitHold) {
            m.quit_hold = 0.0f;
            quit_game();
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
        // The open tab's lighter pill (coal-stripes-hi), sliding from tab to tab.
        const ImVec2 pill_at = ImGui::GetCursorScreenPos();
        list->ChannelsSplit(2);
        list->ChannelsSetCurrent(1);
        float pill_x = pill_at.x;
        float pill_w = 0.0f;
        for (int i = 0; i < tab_count; ++i) {
            if (i > 0) {
                ImGui::SameLine(0.0f, dp(6.0f));  // each tab on its own, with room between
            }
            const float x = ImGui::GetCursorScreenPos().x;
            if (bar_tab(tabs[static_cast<std::size_t>(i)].name.c_str(), i == s.menu_tab, tab_h)) {
                s.menu_tab = i;
                s.menu_hover = -1;
                s.menu_focus = true;
            }
            // On Quit's or Resume's page the pill stays on the last tab and fades there, not
            // sliding off to the bar's start.
            const int pill_tab = s.menu_tab < tab_count ? s.menu_tab : s.menu_motion.last_tab;
            if (i == pill_tab) {
                pill_x = x;
                pill_w = ImGui::GetItemRectSize().x;
            }
        }
        list->ChannelsSetCurrent(0);
        const float px = tween(ImGui::GetID("pill x"), pill_x, kMedium);
        const float pw = tween(ImGui::GetID("pill w"), pill_w, kMedium);
        const float pill_alpha =
            tween(ImGui::GetID("pill alpha"), s.menu_tab < tab_count ? 1.0f : 0.0f, kFast);
        striped_rect(
            list,
            ImVec2(px, pill_at.y),
            ImVec2(px + pw, pill_at.y + tab_h),
            dp(6.0f),
            kCoalHi,
            kCoalHiAlt,
            bar_origin,
            pill_alpha
        );
        list->ChannelsMerge();

        // At the right: Quit and Resume, both bar buttons.
        ImGui::PushFont(menu_fonts.bar);
        const float quit_w = ImGui::CalcTextSize(menu::entry(menu::Id::Quit).label).x + dp(28.0f);
        const float resume_w = ImGui::CalcTextSize("Resume").x + dp(28.0f);
        ImGui::PopFont();
        const float right = bar_a.x + viewport->Size.x - dp(18.0f);
        ImGui::SetCursorScreenPos(
            ImVec2(right - resume_w - dp(8.0f) - quit_w, bar_a.y + (bar - tab_h) * 0.5f)
        );
        // Quit opens its page, which asks first.
        const int quit_at = tab_count;
        if (bar_chip(
                menu::entry(menu::Id::Quit).label,
                ui().err,
                false,
                tab_h,
                s.menu_motion.quit_hold / kQuitHold,
                s.menu_tab == quit_at
            )) {
            s.menu_tab = quit_at;
            s.menu_hover = -1;
            s.menu_focus = true;
        }
        ImGui::SameLine(0.0f, dp(8.0f));
        // A bar button with gold text, as Quit game beside it.
        if (bar_chip("Resume", ui().amber, false, tab_h, 0.0f, s.menu_tab == quit_at + 1)) {
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, dp(5.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImGui::ColorConvertU32ToFloat4(ui().line_strong));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4(0, 0, 0, 0));  // the rows draw their own
    ImGui::PushFont(menu_fonts.body);
    if (ImGui::Begin(
            "OpenRAC menu",
            nullptr,
            fixed | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        )) {
        ImDrawList* list = ImGui::GetWindowDrawList();
        const ImVec2 b(at.x + width, at.y + height);
        // The launcher's panel: a shadow and its fill, no border.
        // (Outside the window's own rectangle, so not clipped to its square corners.)
        list->PushClipRectFullScreen();
        // Many thin faint layers add up to a smooth edge.
        soft_shadow(list, at, b, dp(12.0f), dp(24.0f), dp(8.0f), 24, 3.0f / 255.0f * ease);
        list->PopClipRect();
        // The launcher's panel, lit a little from above: the theme's coal with a glow of the
        // game's colour along its top (--glow), as the launcher's page under its bar.
        panel_fill(
            list,
            at,
            b,
            ui_colors::mix(ui().panel, ui().panel_hi, 0.6f),
            ui().ink_deep,
            panel_glow(),
            dp(160.0f),
            dp(12.0f)
        );
        const float rows_w = std::floor(width * 0.62f);
        MenuMotion& m = s.menu_motion;
        // A tab just picked: its page comes in from the side of the one before, as the launcher's
        // pages do, at the top of its rows.
        if (m.page_tab != s.menu_tab) {
            m.page_from = m.page_tab < 0 ? 0.0f : (s.menu_tab > m.page_tab ? 1.0f : -1.0f);
            m.page_tab = s.menu_tab;
            m.page = 0.0f;
            m.scroll_target = 0.0f;
            m.scroll_shown = 0.0f;
        }
        m.page = std::min(1.0f, m.page + io.DeltaTime / 0.38f);
        const float page = 1.0f - std::pow(1.0f - m.page, 3.0f);
        // The rows stop short of the window's top and foot, so none reach its rounded corners.
        const float inset = dp(10.0f);
        const ImVec2 rows_at(at.x + dp(48.0f) * m.page_from * (1.0f - page), at.y + inset);
        const float rows_h = height - inset * 2.0f;

        // The rows' scroll eases to where it is sent: by the wheel over them, or by the focus
        // moving to a row out of sight (ImGui's own scroll request, taken over here).
        if (ImGuiWindow* rows =
                m.rows_window != 0 ? ImGui::FindWindowByID(m.rows_window) : nullptr) {
            const float most = rows->ScrollMax.y;
            if (rows->ScrollTarget.y != FLT_MAX) {
                // ImGui brings the focus just inside the edge; a little further, so it stands
                // clear of the edge's fade.
                const float to =
                    rows->ScrollTarget.y - rows->ScrollTargetCenterRatio.y * rows->SizeFull.y;
                const float clear = to > m.scroll_shown ? dp(48.0f) : -dp(48.0f);
                m.scroll_target = to + (std::abs(to - m.scroll_shown) > 0.5f ? clear : 0.0f);
            }
            const ImVec2 mouse = io.MousePos;
            if (io.MouseWheel != 0.0f && mouse.x >= at.x && mouse.x < at.x + rows_w
                && mouse.y >= at.y && mouse.y < b.y) {
                m.scroll_target -= io.MouseWheel * dp(120.0f);
            }
            m.scroll_target = std::clamp(m.scroll_target, 0.0f, most);
            m.scroll_shown +=
                (m.scroll_target - m.scroll_shown) * (1.0f - std::exp(-io.DeltaTime * 14.0f));
            if (std::abs(m.scroll_target - m.scroll_shown) < 0.5f) {
                m.scroll_shown = m.scroll_target;
            }
            ImGui::SetNextWindowScroll(ImVec2(-1.0f, std::round(m.scroll_shown)));
        }
        ImGui::SetCursorScreenPos(rows_at);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease * page);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(24.0f), dp(6.0f)));
        if (ImGui::BeginChild(
                "rows",
                ImVec2(rows_w, rows_h),
                ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened,
                ImGuiWindowFlags_NoScrollWithMouse
            )) {
            m.rows_window = ImGui::GetCurrentWindow()->ID;  // its scroll, eased next frame
            tab_title(tab.name, s.menu_tab, static_cast<int>(tabs.size()));
            if (tab.chip != 0 && s.menu_focus) {
                s.quit_focus = true;  // "Back to the game" first
            }
            if (s.menu_focus) {
                s.menu_focus_first = true;
            }
            if (tab.chip != 0) {
                // The space a tab's first section heading leaves under the title.
                ImGui::Dummy(ImVec2(0.0f, dp(14.0f)));
            }
            if (tab.chip == kQuitPage) {
                card_begin();
                quit_confirm_rows(false);  // the question is on the right
                card_end();
            } else if (tab.chip == kResumePage) {
                card_begin();
                if (s.quit_focus) {
                    s.quit_focus = false;
                    s.menu_focus_first = true;
                }
                if (menu_row("Back to the game")) {
                    set_menu(false);
                }
                card_end();
            } else if (tab.catalog >= 0) {
                draw_tab(static_cast<menu::Tab>(tab.catalog), menu::kDesktop);
            } else {
                draw_mod_menu(tab.mod);
            }
            ImGui::Dummy(ImVec2(0.0f, dp(20.0f)));
            s.menu_focus_first = false;
            s.menu_focus = false;
            // Rows under the window's edges fade into its ground rather than being cut, while
            // there is more of them that way.
            ImDrawList* rows_list = ImGui::GetWindowDrawList();
            const float fade_h = dp(44.0f);
            const float x0 = rows_at.x + dp(16.0f);
            const float x1 = rows_at.x + rows_w - dp(8.0f);
            const float top_y = rows_at.y;
            const float foot_y = rows_at.y + rows_h;
            const auto ground = [&](float y) {
                const float t = std::clamp((y - at.y) / height, 0.0f, 1.0f);
                const float lit = std::clamp(1.0f - (y - at.y) / dp(160.0f), 0.0f, 1.0f);
                return ui_colors::mix(
                    ui_colors::mix(
                        ui_colors::mix(ui().panel, ui().panel_hi, 0.6f), ui().ink_deep, t
                    ),
                    ui_colors::mix(ui().panel_hi, ui().amber, 0.08f),
                    lit * lit
                );
            };
            if (ImGui::GetScrollY() > 0.5f) {
                const ImU32 c = ground(top_y);
                const ImU32 none = ui_colors::fade(c, 0.0f);
                rows_list->AddRectFilledMultiColor(
                    ImVec2(x0, top_y), ImVec2(x1, top_y + fade_h), c, c, none, none
                );
            }
            if (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 0.5f) {
                const ImU32 c = ground(foot_y);
                const ImU32 none = ui_colors::fade(c, 0.0f);
                rows_list->AddRectFilledMultiColor(
                    ImVec2(x0, foot_y - fade_h), ImVec2(x1, foot_y), none, none, c, c
                );
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        help_pane(ImVec2(at.x + rows_w, at.y), b, tab.mod >= 0, tab.chip);
    }
    ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(6);
}

}  // namespace openrac::frontend::debug_overlay
