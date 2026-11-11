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
#include <format>
#include <imgui.h>
#include <imgui_internal.h>  // ImHashStr, ShadeVertsLinearColorGradientKeepAlpha
#include <string>

#include "frontend.h"
#include "platform/input.h"

// The menu's typefaces as bytes (cmake/Games.cmake, openrac_menu_fonts, from game/common/fonts/).
extern const unsigned char openrac_font_inter[];
extern const unsigned int openrac_font_inter_size;
extern const unsigned char openrac_font_inter_semibold[];
extern const unsigned int openrac_font_inter_semibold_size;
extern const unsigned char openrac_font_inter_medium[];
extern const unsigned int openrac_font_inter_medium_size;
extern const unsigned char openrac_font_orbitron[];
extern const unsigned int openrac_font_orbitron_size;

namespace openrac::frontend::debug_overlay {

MenuFonts menu_fonts;

const ui_colors::Colors& ui() {
    return ui_colors::launcher();
}

ImU32 pressed_fill() {
    // A neutral lift, as a list row under the pointer on the site: no gold tint (that reads brown
    // on dark) and no navy.
    return ui().control_card_hi;
}

ImU32 control_fill() {
    return ui().control_card;
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

// A choice's row makes its own sound (menu_choice), not the press's.
bool g_quiet_row = false;

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
    menu_fonts.bar = add_font(openrac_font_inter_medium, openrac_font_inter_medium_size, body);
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
          &menu_fonts.bar,
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

void panel_fill(
    ImDrawList* list,
    ImVec2 a,
    ImVec2 b,
    ImU32 top,
    ImU32 bottom,
    ImU32 glow,
    float glow_depth,
    float round,
    ImDrawFlags corners
) {
    if (b.y <= a.y) {
        return;
    }
    if ((corners & ImDrawFlags_RoundCornersMask_) == 0) {
        corners |= ImDrawFlags_RoundCornersAll;
    }
    // Colours are set per vertex, so the panel is cut into bands down through the light; one
    // rect would fade the light over the whole height and not match a panel drawn over it.
    float cuts[16];
    int count = 0;
    cuts[count++] = a.y;
    const float top_round = (corners & ImDrawFlags_RoundCornersTop) ? round + 1.0f : 0.0f;
    const float bottom_round = (corners & ImDrawFlags_RoundCornersBottom) ? round + 1.0f : 0.0f;
    const float lowest = b.y - bottom_round;
    if (a.y + top_round < lowest) {
        cuts[count++] = a.y + top_round;
    }
    constexpr int kBands = 10;
    for (int i = 1; i <= kBands; ++i) {
        const float y = a.y + glow_depth * i / kBands;
        if (y > cuts[count - 1] + 0.5f && y < lowest) {
            cuts[count++] = y;
        }
    }
    if (lowest > cuts[count - 1]) {
        cuts[count++] = lowest;
    }
    cuts[count++] = b.y;
    for (int i = 0; i + 1 < count; ++i) {
        ImDrawFlags flags = ImDrawFlags_RoundCornersNone;
        if (i == 0) {
            flags |= corners & ImDrawFlags_RoundCornersTop;
        }
        if (i + 2 == count) {
            flags |= corners & ImDrawFlags_RoundCornersBottom;
        }
        const int first = list->VtxBuffer.Size;
        list->AddRectFilled(
            ImVec2(a.x, cuts[i]), ImVec2(b.x, cuts[i + 1]), IM_COL32_WHITE, round, flags
        );
        for (int v = first; v < list->VtxBuffer.Size; ++v) {
            ImDrawVert& vert = list->VtxBuffer[v];
            const float t = std::clamp((vert.pos.y - a.y) / (b.y - a.y), 0.0f, 1.0f);
            const float lit = std::clamp(1.0f - (vert.pos.y - a.y) / glow_depth, 0.0f, 1.0f);
            vert.col = ui_colors::mix(ui_colors::mix(top, bottom, t), glow, lit * lit);
        }
    }
}

ImU32 panel_glow() {
    return ui_colors::mix(ui().panel_hi, ui().glow, 0.08f);
}

void primary_fill(ImDrawList* list, ImVec2 a, ImVec2 b, float round, float held) {
    // The launcher's main button: a flat fill of the accent, no edge, no stripes; darker while held.
    list->AddRectFilled(a, b, ui_colors::mix(ui().amber, ui().ink_deep, 0.18f * held), round);
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
    // Whole pixels: Dear ImGui truncates a text's position, which would set it up to a pixel left
    // of and above the middle.
    return ImVec2(
        std::round(centre.x - (ink_left + ink_right) * 0.5f), std::round(centre.y - cap_middle)
    );
}

float beside_caps(const ImFont* font, float middle) {
    // Centre a capital letter (0.727 of the size in Inter) rather than the line box, which has
    // room for descenders and would sit the letters low.
    return middle + menu_fonts.small->FontSize * 0.727f * 0.5f - font->Ascent;
}

float key_cap(ImDrawList* list, float x, float middle, const std::string& key, float alpha) {
    const float h = key_cap_height();
    const float w = key_cap_width(key);
    // On whole pixels, as its text is (centred_text), so the two share one middle.
    const ImVec2 a(std::round(x), std::round(middle - h * 0.5f));
    const ImVec2 b(a.x + std::round(w), a.y + std::round(h));
    const ImVec2 centre((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    // A badge: on the darkest black, as the site's badges and key hints, 4 px corners, no border.
    list->AddRectFilled(a, b, ui_colors::fade(ui().ink_deep, alpha), dp(4.0f));
    const CapText t = cap_text(key);
    ImVec2 at = centred_text(t.font, t.size, key, centre);
    if (cap_sign(key)) {
        // A sign is centred by its own shape, not by a capital's.
        unsigned int c = 0;
        ImTextCharFromUtf8(&c, key.c_str(), nullptr);
        if (const ImFontGlyph* glyph = t.font->FindGlyph(static_cast<ImWchar>(c))) {
            at.y =
                std::round(centre.y - (glyph->Y0 + glyph->Y1) * 0.5f * (t.size / t.font->FontSize));
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
    // A card is a flat fill with no frame, 12 px corners (no stripes on big surfaces).
    list->AddRectFilled(s.card.top, end, ui().panel_hi, dp(12.0f));
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

bool menu_row(const char* label, ImU32 ink, const RowRight& right, float right_width) {
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
    if (pressed && !g_quiet_row) {
        menu_sound(MenuSound::Confirm);
    }
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
            a, b, ui_colors::fade(pressed_fill(), std::max(held, hover * 0.5f)), dp(6.0f)
        );
    }
    focus_ring(list, a, b, dp(6.0f));
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
    const ImVec2 text_at(
        at.x + dp(16.0f) + dp(4.0f) * mark, at.y + (height - ImGui::GetFontSize()) * 0.5f
    );
    if (right_width > 0.0f) {
        // Up to what the right side draws: a longer label is cut short with "...".
        const float until = at.x + width - dp(16.0f) - right_width - dp(10.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ink);
        ImGui::RenderTextEllipsis(
            list,
            text_at,
            ImVec2(until, text_at.y + ImGui::GetFontSize()),
            until,
            until,
            label,
            nullptr,
            nullptr
        );
        ImGui::PopStyleColor();
    } else {
        list->AddText(text_at, ink, label);
    }
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
        // The launcher's switch: a track in the darkest black, no edge; when on, the
        // gold tint of a control that is on and a gold knob (no wall of solid gold).
        list->AddRectFilled(a, b, ui_colors::mix(ui().ink_deep, ui().amber, 0.22f * k), h * 0.5f);
        const float knob = h * 0.5f - dp(4.0f);
        const float from = a.x + h * 0.5f;
        const float to = b.x - h * 0.5f;
        list->AddCircleFilled(
            ImVec2(from + (to - from) * k, right.y), knob, ui_colors::mix(ui().dim, ui().amber, k)
        );
    });
}

int menu_choice(const char* label, const std::string& value, bool can_lower, bool can_raise) {
    float back_until = 0.0f;  // a press left of this moves the value back
    // The values of every row in one column, centred between its arrows; a label too long for
    // what is left of the row ends in "..." (the drawer's "Anisotropic filtering").
    const float column = std::min(s.card.width * 0.48f, dp(190.0f));
    const auto draw = [&](ImDrawList* list, ImVec2 right) {
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
    };
    g_quiet_row = true;  // the row's sound is the value's (below)
    const bool pressed = menu_row(label, ui().text, draw, column + dp(8.0f));
    g_quiet_row = false;
    int by = 0;
    if (pressed) {
        by = ImGui::IsMouseReleased(0) && ImGui::GetIO().MousePos.x < back_until ? -1 : 1;
    }
    if (ImGui::IsItemFocused()) {
        // The arrow keys, which the d-pad also presses (feed_gamepads); not the sticks.
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            by = -1;
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            by = 1;
        }
    }
    const bool blocked = (by < 0 && !can_lower) || (by > 0 && !can_raise);
    if (by != 0) {
        // As the game's lists: the cursor's sound for a value moved, Confirm for one pressed, and
        // Denied past either end.
        menu_sound(blocked ? MenuSound::Denied : pressed ? MenuSound::Confirm : MenuSound::Cursor);
    }
    return blocked ? 0 : by;
}

// Draws a slider using the same focus and divider as the choice rows: its value between two arrows
// (a tap on one moves it a step), and a track to drag between them.
int menu_percent_slider(const char* label, int value, int minimum, int maximum, int step) {
    // The choice rows' column, so that the arrows line up with theirs.
    const float column = std::min(s.card.width * 0.48f, dp(190.0f));
    const float arrow_room = dp(16.0f);  // each arrow's part of the column, which a tap on it hits
    const float text_room = dp(42.0f);   // the value's, "120%"
    const bool touch = drawer_possible();
    float left = 0.0f;
    float track_left = 0.0f;
    float track_right = 0.0f;
    float right_x = 0.0f;
    const auto draw = [&](ImDrawList* list, ImVec2 right) {
        left = right.x - column;
        right_x = right.x;
        track_left = left + arrow_room;
        track_right = right.x - arrow_room - text_room;
        const ImU32 off = ui_colors::fade(ui().dim, 0.35f);
        arrow(list, ImVec2(left, right.y), -1.0f, 0.0f, value > minimum ? ui().amber : off);
        arrow(list, ImVec2(right.x, right.y), 1.0f, 0.0f, value < maximum ? ui().amber : off);
        const float fraction =
            static_cast<float>(value - minimum) / static_cast<float>(maximum - minimum);
        const ImVec2 start(track_left, right.y);
        const ImVec2 knob(std::lerp(track_left, track_right, fraction), right.y);
        list->AddLine(start, ImVec2(track_right, right.y), ui().line_strong, dp(3.0f));
        list->AddLine(start, knob, ui().amber, dp(3.0f));
        // The knob grows while a finger holds it, so that it shows beside the finger what moves.
        const bool held = s.slider_held && ImGui::IsItemActive();
        const float big = tween(item_part("knob"), held ? 1.0f : 0.0f, kFast);
        const float radius = touch ? dp(7.0f) : dp(5.0f);
        list->AddCircleFilled(knob, radius + dp(4.0f) * big, ui().amber);
        const std::string text = std::format("{}%", value);
        list->AddText(
            centred_text(
                ImGui::GetFont(),
                ImGui::GetFontSize(),
                text,
                ImVec2(track_right + text_room * 0.5f + dp(4.0f), right.y)
            ),
            ui().gold,
            text.c_str()
        );
    };
    g_quiet_row = true;
    const bool pressed = menu_row(label, ui().text, draw, column + dp(8.0f));
    g_quiet_row = false;

    const ImGuiIO& io = ImGui::GetIO();
    const auto at_pointer = [&] {
        const float fraction =
            std::clamp((io.MousePos.x - track_left) / (track_right - track_left), 0.0f, 1.0f);
        return minimum
               + step
                     * static_cast<int>(
                         std::lround(fraction * static_cast<float>((maximum - minimum) / step))
                     );
    };
    // Where the finger or the mouse came down: on an arrow, or on the track or near it.
    const float start_x = io.MouseClickedPos[0].x;
    const bool on_lower = start_x >= left - dp(12.0f) && start_x < track_left - dp(4.0f);
    const bool on_raise = start_x > track_right + text_room && start_x <= right_x + dp(12.0f);
    const bool on_track = start_x >= track_left - dp(4.0f) && start_x <= track_right + dp(8.0f);
    const bool active = ImGui::IsItemActive() && ImGui::IsMouseDown(0);
    if (!ImGui::IsMouseDown(0)) {
        s.slider_held = false;
    }
    int next = value;
    int by = 0;
    if (pressed && (on_lower || on_raise)) {
        by = on_lower ? -1 : 1;
    } else if (touch) {
        // A finger moved sideways first holds the slider, which then follows it until it lets go;
        // moved up or down first, it scrolls the drawer (drag_scroll) and the row lets go. A tap
        // on the track puts the value there.
        const ImVec2 moved = ImGui::GetMouseDragDelta(0, 0.0f);
        if (active && on_track && !s.slider_held && std::abs(moved.x) > dp(6.0f)
            && std::abs(moved.x) > std::abs(moved.y)) {
            s.slider_held = true;
        }
        if ((active && s.slider_held) || (on_track && pressed)) {
            next = at_pointer();
        }
    } else if (on_track && (active || pressed)) {
        next = at_pointer();
    }

    // The arrow keys, which the d-pad also presses (feed_gamepads): a step each, held to repeat.
    if (ImGui::IsItemFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            by = -1;
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            by = 1;
        }
    }
    if (by != 0) {
        next = std::clamp(value + by * step, minimum, maximum);
    }
    // As the choice rows: the cursor's sound for each step it moves, Denied past either end.
    if (next != value) {
        menu_sound(MenuSound::Cursor);
    } else if (by != 0) {
        menu_sound(MenuSound::Denied);
    }
    return next;
}

bool menu_button(const char* label) {
    const float height = dp(46.0f);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + ImGui::GetContentRegionAvail().x, a.y + height);
    const bool pressed =
        ImGui::InvisibleButton(label, ImVec2(b.x - a.x, height), ImGuiButtonFlags_EnableNav);
    if (pressed) {
        menu_sound(MenuSound::Confirm);
    }
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float round = dp(6.0f);
    primary_fill(
        list, a, b, round, tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast)
    );
    focus_ring(
        list,
        ImVec2(a.x - dp(3.0f), a.y - dp(3.0f)),
        ImVec2(b.x + dp(3.0f), b.y + dp(3.0f)),
        round + dp(3.0f)
    );
    // In SemiBold, as the launcher's main button.
    ImFont* font = menu_fonts.strong;
    list->AddText(
        font,
        font->FontSize,
        centred_text(font, font->FontSize, label, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f)),
        ui().bar_ink,
        label
    );
    return pressed;
}

void menu_note(const char* line, ImU32 ink) {
    // First in its card, as far from the card's top as a row's label is.
    ImGui::Dummy(ImVec2(0.0f, dp(s.card.rows == 0 ? 14.0f : 10.0f)));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dp(16.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ink));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - dp(16.0f));
    ImGui::TextUnformatted(line);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, dp(10.0f)));
    ++s.card.rows;
}

void menu_key_rows(const std::vector<KeyRow>& rows) {
    const float gap = dp(4.0f);
    float caps = 0.0f;
    for (const KeyRow& row : rows) {
        float w = 0.0f;
        for (const std::string& key : row.keys) {
            w += key_cap_width(key) + gap;
        }
        caps = std::max(caps, w);
    }
    ImDrawList* list = ImGui::GetWindowDrawList();
    ImFont* body = menu_fonts.body;
    const float line = dp(32.0f);
    ImGui::Dummy(ImVec2(0.0f, dp(s.card.rows == 0 ? 8.0f : 4.0f)));
    for (const KeyRow& row : rows) {
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const float middle = at.y + line * 0.5f;
        float x = at.x + dp(16.0f);
        for (const std::string& key : row.keys) {
            x += key_cap(list, x, middle, key, 1.0f) + gap;
        }
        list->AddText(
            body,
            body->FontSize,
            ImVec2(std::round(at.x + dp(16.0f) + caps + dp(8.0f)), beside_caps(body, middle)),
            ui().text,
            row.does.c_str()
        );
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, line));
    }
    ImGui::Dummy(ImVec2(0.0f, dp(6.0f)));
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
    s.quit_asked = false;
    // The buttons and keys that closed it (the menu button, B, Esc) are not the game's too.
    if (platform::Input* input = frontend::input()) {
        input->hold_back_held();
    }
    s.drawer_controller = false;  // the drawer opens again with its own rows alone
    s.drawer_controller_at = 0.0f;
    if (s.shot_when_shut) {
        s.shot_when_shut = false;
        s.shot_tick = s.tick;
    }
}

}  // namespace openrac::frontend::debug_overlay
