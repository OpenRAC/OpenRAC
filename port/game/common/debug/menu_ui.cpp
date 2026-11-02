// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The in-game menu's pieces in the launcher's look (debug/menu_ui.h): the rows, switches, arrows,
// headings, buttons and notes both menus draw, their typefaces, and the menu's opening and closing.

#include "debug/menu_ui.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
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

void add_menu_fonts(bool touch) {
    const float body = std::round(15.0f * s.ui_scale);
    menu_fonts.body = add_font(openrac_font_inter, openrac_font_inter_size, body);
    menu_fonts.strong =
        add_font(openrac_font_inter_semibold, openrac_font_inter_semibold_size, body);
    menu_fonts.head =
        add_font(openrac_font_orbitron, openrac_font_orbitron_size, std::round(17.0f * s.ui_scale));
    menu_fonts.title =
        add_font(openrac_font_orbitron, openrac_font_orbitron_size, std::round(30.0f * s.ui_scale));
    menu_fonts.small = add_font(
        openrac_font_inter_semibold,
        openrac_font_inter_semibold_size,
        std::round(12.0f * s.ui_scale)
    );
    // Larger on a touch screen, for reading at arm's length and pressing with a finger.
    menu_fonts.panel = add_font(
        openrac_font_inter,
        openrac_font_inter_size,
        std::round((touch ? 16.0f : 14.0f) * s.ui_scale)
    );
    ImFont* fallback =
        ImGui::GetIO().Fonts->Fonts.empty() ? nullptr : ImGui::GetIO().Fonts->Fonts[0];
    for (ImFont** font :
         {&menu_fonts.body,
          &menu_fonts.strong,
          &menu_fonts.head,
          &menu_fonts.title,
          &menu_fonts.small,
          &menu_fonts.panel}) {
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

namespace {

// A key that is a single symbol (an arrow, `, /) is drawn larger and centred by its own shape:
// at the caps' small size it would be a speck, and ` sits at the top of the line.
bool cap_sign(const std::string& key) {
    unsigned int c = 0;
    const int length = ImTextCharFromUtf8(&c, key.c_str(), nullptr);
    return length == static_cast<int>(key.size())
           && (c < 0x80 ? std::ispunct(static_cast<int>(c)) != 0 : c >= 0x2190 && c <= 0x2193);
}

struct CapText {
    ImFont* font;
    float size;
};

CapText cap_text(const std::string& key) {
    if (!cap_sign(key)) {
        return {menu_fonts.small, menu_fonts.small->FontSize};
    }
    // An arrow a little over the text's size; a mark, smaller of itself, in the titles' larger
    // letters at the size they were made at (scaled up, a glyph blurs).
    if (static_cast<unsigned char>(key[0]) >= 0x80) {
        return {menu_fonts.body, menu_fonts.body->FontSize * 1.25f};
    }
    return {menu_fonts.title, menu_fonts.title->FontSize};
}

}  // namespace

void soft_shadow(
    ImDrawList* list,
    ImVec2 a,
    ImVec2 b,
    float round,
    float spread,
    float drop,
    int layers,
    float alpha
) {
    const ImU32 ink = ui_colors::fade(ui_colors::rgba(0, 0, 0, 255), alpha);
    for (int i = layers; i >= 1; --i) {
        const float grow = spread * static_cast<float>(i) / static_cast<float>(layers);
        list->AddRectFilled(
            ImVec2(a.x - grow, a.y - grow + drop),
            ImVec2(b.x + grow, b.y + grow + drop),
            ink,
            round + grow
        );
    }
}

float ease_out(float t) {
    return 1.0f - std::pow(1.0f - std::clamp(t, 0.0f, 1.0f), 4.0f);
}

float slide(float at, bool open) {
    const float dt = ImGui::GetIO().DeltaTime;
    return std::clamp(at + (open ? dt / kMedium : -dt / 0.2f), 0.0f, 1.0f);
}

float key_caps(ImDrawList* list, float x, float middle, std::string_view keys, float alpha) {
    float w = 0.0f;
    while (true) {
        // "+" joins two keys, unless it ends the name ("Keypad +").
        const std::size_t plus = keys.find('+', 1);
        const bool joined = plus != std::string_view::npos && plus + 1 < keys.size();
        const std::string key(keys.substr(0, joined ? plus : keys.size()));
        w += list != nullptr ? key_cap(list, x + w, middle, key, alpha) : key_cap_width(key);
        if (!joined) {
            return w;
        }
        ImFont* font = menu_fonts.small;
        if (list != nullptr) {
            list->AddText(
                font,
                font->FontSize,
                ImVec2(x + w + dp(4.0f), beside_caps(font, middle)),
                ui_colors::fade(ui().dim, alpha),
                "+"
            );
        }
        w += font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, "+").x + dp(8.0f);
        keys.remove_prefix(plus + 1);
    }
}

float key_cap_height() {
    return dp(22.0f);
}

float key_cap_width(const std::string& key) {
    const CapText t = cap_text(key);
    const float w = t.font->CalcTextSizeA(t.size, FLT_MAX, 0.0f, key.c_str()).x;
    return std::max(key_cap_height(), w + dp(14.0f));
}

ImVec2 centred_text(ImFont* font, float size, std::string_view text, ImVec2 centre) {
    const float scale = size / font->FontSize;
    float pen = 0.0f;
    float ink_left = 0.0f;
    float ink_right = 0.0f;
    bool first = true;
    for (const char* at = text.data(); at < text.data() + text.size();) {
        unsigned int c = 0;
        at += ImTextCharFromUtf8(&c, at, text.data() + text.size());
        const ImFontGlyph* glyph = font->FindGlyph(static_cast<ImWchar>(c));
        if (glyph == nullptr) {
            continue;
        }
        if (glyph->Visible) {
            ink_left = first ? pen + glyph->X0 * scale : ink_left;
            ink_right = pen + glyph->X1 * scale;
            first = false;
        }
        pen += glyph->AdvanceX * scale;
    }
    // A capital's top and bottom; a font without H falls back to its line box.
    const ImFontGlyph* cap = font->FindGlyph('H');
    const float cap_middle = cap != nullptr ? (cap->Y0 + cap->Y1) * 0.5f * scale : size * 0.5f;
    return ImVec2(centre.x - (ink_left + ink_right) * 0.5f, centre.y - cap_middle);
}

float beside_caps(const ImFont* font, float middle) {
    // Centre a capital letter (0.727 of the size in Inter) rather than the line box, which has
    // room for descenders and would sit the letters low.
    return middle + menu_fonts.small->FontSize * 0.727f * 0.5f - font->Ascent;
}

float key_cap(ImDrawList* list, float x, float middle, const std::string& key, float alpha) {
    const float h = key_cap_height();
    const float w = key_cap_width(key);
    const ImVec2 a(x, middle - h * 0.5f);
    const ImVec2 b(x + w, middle + h * 0.5f);
    list->AddRectFilled(a, b, ui_colors::fade(ui().panel_hi, alpha), h * 0.5f);
    list->AddRect(a, b, ui_colors::fade(ui().line_strong, alpha), h * 0.5f);
    const CapText t = cap_text(key);
    ImVec2 at = centred_text(t.font, t.size, key, ImVec2(x + w * 0.5f, middle));
    if (cap_sign(key)) {
        // A sign is centred by its own shape, not by a capital's.
        unsigned int c = 0;
        ImTextCharFromUtf8(&c, key.c_str(), nullptr);
        if (const ImFontGlyph* glyph = t.font->FindGlyph(static_cast<ImWchar>(c))) {
            at.y = middle - (glyph->Y0 + glyph->Y1) * 0.5f * (t.size / t.font->FontSize);
        }
    }
    list->AddText(t.font, t.size, at, ui_colors::fade(ui().gold, alpha), key.c_str());
    return w;
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
    ImDrawList* list = ImGui::GetWindowDrawList();
    list->AddText(font, size, ImVec2(at.x + dp(4.0f), at.y), ui().gold, title);
    // A hairline after it to the card's edge, fading out, as the launcher's section rules.
    const float end = at.x + ImGui::GetContentRegionAvail().x;
    const float from =
        at.x + dp(4.0f) + font->CalcTextSizeA(size, FLT_MAX, 0.0f, title).x + dp(14.0f);
    if (end - from > dp(24.0f)) {
        const float y = std::round(at.y + size * 0.55f) + 0.5f;
        const ImU32 near = ui().line_strong;
        const ImU32 far = ui_colors::fade(ui().line, 0.0f);
        list->AddRectFilledMultiColor(
            ImVec2(from, y), ImVec2(end - dp(4.0f), y + 1.0f), near, far, far, near
        );
    }
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
    const bool pressed =
        ImGui::InvisibleButton("row", ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    ImGui::PopID();
    // The menu just opened or changed tab: its first row takes the focus, so that the arrows and a
    // controller start there and the desktop's side says what it does.
    if (s.menu_focus_first) {
        s.menu_focus_first = false;
        ImGui::FocusItem();
    }
    const ImVec2 a(at.x + dp(3.0f), at.y + dp(3.0f));
    const ImVec2 b(at.x + width - dp(3.0f), at.y + height - dp(3.0f));
    const float held = tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast);
    // Mouse hover: a fainter accent than a press. Not on a touch screen, where a finger only
    // presses.
    const bool over = !drawer_possible() && ImGui::IsItemHovered();
    const float hover = tween(item_part("hover"), over ? 1.0f : 0.0f, kFast);
    if (held > 0.0f || hover > 0.0f) {
        list->AddRectFilled(
            a, b, ui_colors::fade(pressed_fill(), std::max(held, hover * 0.5f)), dp(10.0f)
        );
    }
    focus_ring(list, a, b, dp(10.0f));
    // The row under the pointer or the focus: a short bar of the accent at its left, the label
    // moved a little right of it.
    const bool lit = over || (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible);
    const float mark = tween(item_part("mark"), lit ? 1.0f : 0.0f, kFast);
    if (mark > 0.0f) {
        const float half = height * 0.22f * mark;
        list->AddRectFilled(
            ImVec2(at.x + dp(7.0f), at.y + height * 0.5f - half),
            ImVec2(at.x + dp(10.0f), at.y + height * 0.5f + half),
            ui_colors::fade(ui().amber, mark),
            dp(2.0f)
        );
    }
    list->AddText(
        ImVec2(at.x + dp(16.0f) + dp(4.0f) * mark, at.y + (height - ImGui::GetFontSize()) * 0.5f),
        ink,
        label
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
        // The values of every row in one column, centred between its arrows; narrower in a
        // narrow card where the label would run under it (the drawer's "Anisotropic filtering").
        const float label_end =
            ImGui::GetItemRectMin().x + dp(20.0f) + ImGui::CalcTextSize(label).x + dp(12.0f);
        const float least = ImGui::CalcTextSize(value.c_str()).x + dp(44.0f);
        const float column =
            std::max(std::min({s.card.width * 0.48f, dp(190.0f), right.x - label_end}), least);
        const float left = right.x - column;
        const ImU32 off = ui_colors::fade(ui().dim, 0.35f);
        arrow(list, ImVec2(left, right.y), -1.0f, 0.0f, can_lower ? ui().amber : off);
        arrow(list, ImVec2(right.x, right.y), 1.0f, 0.0f, can_raise ? ui().amber : off);
        ImFont* font = ImGui::GetFont();
        list->AddText(
            centred_text(font, ImGui::GetFontSize(), value, ImVec2(left + column * 0.5f, right.y)),
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
        // The d-pad or the arrow keys (the sticks do not move through the menus).
        const auto held = [](ImGuiKey pad, ImGuiKey key) {
            return ImGui::IsKeyPressed(pad) || ImGui::IsKeyPressed(key);
        };
        if (held(ImGuiKey_GamepadDpadLeft, ImGuiKey_LeftArrow)) {
            by = -1;
        } else if (held(ImGuiKey_GamepadDpadRight, ImGuiKey_RightArrow)) {
            by = 1;
        }
    }
    return (by < 0 && !can_lower) || (by > 0 && !can_raise) ? 0 : by;
}

bool menu_button(const char* label) {
    const float height = dp(46.0f);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + ImGui::GetContentRegionAvail().x, a.y + height);
    const bool pressed =
        ImGui::InvisibleButton(label, ImVec2(b.x - a.x, height), ImGuiButtonFlags_EnableNav);
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
    list->AddText(
        centred_text(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            label,
            ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f)
        ),
        ui().bar_ink,
        label
    );
    return pressed;
}

void menu_note(const char* line, ImU32 ink) {
    ImGui::Dummy(ImVec2(0.0f, dp(10.0f)));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dp(16.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ink));
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
        dismiss("menu.hint");           // the hint at the start has done its work
    } else {
        s.paused = s.keep_paused;
    }
}

// The menu is out of the picture: what waited for that.
void menu_closed() {
    s.menu_confirm_reset = false;
    s.drawer_controller = false;  // the drawer opens again with its own rows alone
    s.drawer_controller_at = 0.0f;
    if (s.shot_when_shut) {
        s.shot_when_shut = false;
        s.shot_path = shot_name(s.tick);
    }
}

}  // namespace openrac::frontend::debug_overlay
