// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Notification cards (debug_overlay.h): they slide in at the top of the screen, stack, and slide
// away. Icons are drawn with lines, so no icon font is needed.

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <imgui.h>
#include <string>
#include <vector>

#include "debug/menu_ui.h"
#include "debug/overlay.h"
#include "platform/show_folder.h"

namespace openrac::frontend::debug_overlay {

namespace {

// One card on screen.
struct Slot {
    Notification n;
    double left = 0.0;    // seconds left on screen
    float in = 0.0f;      // 0 to 1 while sliding in
    float out = 0.0f;     // 0 to 1 while sliding out
    bool going = false;   // timed out, dismissed or pushed out by newer slots
    float y = 0.0f;       // drawn top; eases to its place when a card above leaves
    bool placed = false;  // y set at least once
    ImVec2 a{}, b{};      // where it was drawn last, for a press on it
    float alpha = 0.0f;   // how much it showed then; 0 hidden
};

std::vector<Slot> slots;
float stack_shown = 1.0f;  // 0 while the menu hides the slots

constexpr std::size_t kMost = 4;  // slots at once; a new one pushes out the oldest timed one
constexpr float kIn = 0.45f;      // seconds to slide in
constexpr float kOut = 0.32f;     // seconds to slide out

ImU32 tone_colour(Tone tone) {
    switch (tone) {
        case Tone::Info:
            return ui().amber;
        case Tone::Done:
            return ui_colors::rgba(0x8f, 0xd6, 0x94);
        case Tone::Warning:
            return ui_colors::rgba(0xff, 0xc8, 0x57);
        case Tone::Error:
            return ui().err;
        case Tone::Live:
            return ui_colors::rgba(0xff, 0x5f, 0x5f);
    }
    return ui().amber;
}

// The icon inside the circle at the card's left: centre `c`, circle radius `r`.
void mark(ImDrawList* list, ImVec2 c, float r, Tone tone, ImU32 ink) {
    const float w = dp(2.0f);
    const float k = r * 0.42f;
    switch (tone) {
        case Tone::Info:  // the menu's three bars
            for (int i = -1; i <= 1; ++i) {
                const float y = c.y + static_cast<float>(i) * k * 0.7f;
                list->AddLine(ImVec2(c.x - k, y), ImVec2(c.x + k, y), ink, w);
            }
            break;
        case Tone::Done: {
            const ImVec2 tick[] = {
                ImVec2(c.x - k, c.y + k * 0.05f),
                ImVec2(c.x - k * 0.3f, c.y + k * 0.7f),
                ImVec2(c.x + k, c.y - k * 0.6f),
            };
            list->AddPolyline(tick, 3, ink, ImDrawFlags_None, w);
            break;
        }
        case Tone::Warning:
            list->AddLine(ImVec2(c.x, c.y - k), ImVec2(c.x, c.y + k * 0.25f), ink, w);
            list->AddCircleFilled(ImVec2(c.x, c.y + k * 0.8f), w * 0.75f, ink);
            break;
        case Tone::Error:
            list->AddLine(
                ImVec2(c.x - k * 0.75f, c.y - k * 0.75f),
                ImVec2(c.x + k * 0.75f, c.y + k * 0.75f),
                ink,
                w
            );
            list->AddLine(
                ImVec2(c.x + k * 0.75f, c.y - k * 0.75f),
                ImVec2(c.x - k * 0.75f, c.y + k * 0.75f),
                ink,
                w
            );
            break;
        case Tone::Live: {
            // Pulses like a camera's recording light.
            const float beat = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.0f);
            list->AddCircleFilled(c, k * 0.85f, ui_colors::fade(ink, beat));
            break;
        }
    }
}

// A card's measured content, used for its size and when drawing it.
struct Layout {
    float width = 0.0f;   // the content's, right of the mark
    float height = 0.0f;  // the content's
    float title_h = 0.0f;
    float wrap = 0.0f;    // title wrap width
    float keys_w = 0.0f;  // the caps' line, 0 without keys
    float text_wrap = 0.0f;
};

constexpr float kOrWidth = 22.0f;  // the "or" between two ways, in dp

Layout measure(const Notification& n, float most) {
    Layout l;
    ImFont* title = menu_fonts.strong;
    ImFont* body = menu_fonts.body;
    l.wrap = most;
    const ImVec2 t = title->CalcTextSizeA(title->FontSize, FLT_MAX, most, n.title.c_str());
    l.title_h = t.y;
    l.width = t.x;
    for (std::size_t i = 0; i < n.keys.size(); ++i) {
        l.keys_w += key_caps(nullptr, 0.0f, 0.0f, n.keys[i], 1.0f) + (i > 0 ? dp(kOrWidth) : 0.0f);
    }
    float second_w = l.keys_w;
    float second_h = n.keys.empty() ? 0.0f : key_cap_height();
    if (!n.text.empty()) {
        const float gap = n.keys.empty() ? 0.0f : dp(8.0f);
        l.text_wrap = std::max(most - l.keys_w - gap, dp(120.0f));
        const ImVec2 b = body->CalcTextSizeA(body->FontSize, FLT_MAX, l.text_wrap, n.text.c_str());
        second_w += gap + b.x;
        second_h = std::max(second_h, b.y);
    }
    l.width = std::max(l.width, second_w);
    l.height = l.title_h + (second_h > 0.0f ? dp(4.0f) + second_h : 0.0f);
    return l;
}

void draw_card(
    ImDrawList* list, const Slot& card, ImVec2 a, float width, const Layout& l, float alpha
) {
    const Notification& n = card.n;
    const float pad = dp(14.0f);
    const float circle = dp(34.0f);
    const float h = std::max(circle, l.height) + pad * 2.0f;
    const ImVec2 b(a.x + width, a.y + h);
    const float round = std::min(h * 0.5f, dp(12.0f));
    const ImU32 tone = tone_colour(n.tone);
    soft_shadow(list, a, b, round, dp(18.0f), dp(6.0f), 12, 0.035f * alpha);
    // A card: its fill, no border.
    list->AddRectFilled(a, b, ui_colors::fade(ui().panel, 0.97f * alpha), round);
    // Icon on a tint of its tone.
    const ImVec2 c(a.x + pad + circle * 0.5f, a.y + h * 0.5f);
    list->AddCircleFilled(c, circle * 0.5f, ui_colors::fade(tone, 0.16f * alpha));
    mark(list, c, circle * 0.5f, n.tone, ui_colors::fade(tone, alpha));
    // Text.
    const float x = a.x + pad + circle + dp(12.0f);
    float y = a.y + (h - l.height) * 0.5f;
    ImFont* title = menu_fonts.strong;
    ImFont* body = menu_fonts.body;
    list->AddText(
        title,
        title->FontSize,
        ImVec2(x, y),
        ui_colors::fade(ui().text, alpha),
        n.title.c_str(),
        nullptr,
        l.wrap
    );
    y += l.title_h + dp(4.0f);
    float cx = x;
    const float middle = y + key_cap_height() * 0.5f;
    for (std::size_t i = 0; i < n.keys.size(); ++i) {
        if (i > 0) {
            ImFont* small = menu_fonts.small;
            const float ow = small->CalcTextSizeA(small->FontSize, FLT_MAX, 0.0f, "or").x;
            list->AddText(
                small,
                small->FontSize,
                ImVec2(cx + (dp(kOrWidth) - ow) * 0.5f, beside_caps(small, middle)),
                ui_colors::fade(ui().dim, alpha),
                "or"
            );
            cx += dp(kOrWidth);
        }
        cx += key_caps(list, cx, middle, n.keys[i], alpha);
    }
    if (!n.text.empty()) {
        const float tx = n.keys.empty() ? x : cx + dp(8.0f);
        const float ty = n.keys.empty() ? y : beside_caps(body, middle);
        list->AddText(
            body,
            body->FontSize,
            ImVec2(tx, ty),
            ui_colors::fade(ui().soft, alpha),
            n.text.c_str(),
            nullptr,
            l.text_wrap
        );
    }
    // Time left, as a line along the bottom that shrinks.
    if (n.seconds > 0.0) {
        const float t = static_cast<float>(std::clamp(card.left / n.seconds, 0.0, 1.0));
        const float x0 = a.x + round + dp(4.0f);
        const float x1 = b.x - round - dp(4.0f);
        const float line_y = b.y - dp(4.0f);
        list->AddLine(
            ImVec2(x0, line_y), ImVec2(x1, line_y), ui_colors::fade(ui().line, alpha), dp(1.5f)
        );
        list->AddLine(
            ImVec2(x0, line_y),
            ImVec2(x0 + (x1 - x0) * t, line_y),
            ui_colors::fade(tone, 0.7f * alpha),
            dp(1.5f)
        );
    }
}

Slot* find(std::string_view id) {
    if (id.empty()) {
        return nullptr;
    }
    for (Slot& card : slots) {
        if (card.n.id == id) {
            return &card;
        }
    }
    return nullptr;
}

}  // namespace

void notify(Notification n) {
    if (Slot* card = find(n.id)) {
        // Same id: update in place, and restart its timer if the title or time changed.
        const bool restart =
            card->going || n.title != card->n.title || n.seconds != card->n.seconds;
        card->n = std::move(n);
        if (card->going) {
            card->going = false;
            card->in = 1.0f - card->out;  // comes back from where it was going
            card->out = 0.0f;
        }
        if (restart) {
            card->left = card->n.seconds;
        }
        return;
    }
    Slot card;
    card.left = n.seconds;
    card.n = std::move(n);
    slots.push_back(std::move(card));
    // Too many cards: the oldest timed one leaves first.
    std::size_t staying = 0;
    for (const Slot& c : slots) {
        staying += c.going ? 0 : 1;
    }
    for (Slot& c : slots) {
        if (staying <= kMost) {
            break;
        }
        if (!c.going && c.n.seconds > 0.0) {
            c.going = true;
            --staying;
        }
    }
}

void dismiss(std::string_view id) {
    if (Slot* card = find(id)) {
        card->going = true;
    }
}

bool press_notification(ImVec2 at) {
    for (Slot& card : slots) {
        if (card.going || card.alpha < 0.5f || at.x < card.a.x || at.x > card.b.x || at.y < card.a.y
            || at.y > card.b.y) {
            continue;
        }
        // A card with a file opens its folder; a timed one goes once pressed. The press is the
        // card's either way, not the game's.
        if (!card.n.folder.empty() && !platform::show_folder(card.n.folder)) {
            notify({
                .title = "The folder could not be opened",
                .text = card.n.folder,
                .tone = Tone::Error,
                .seconds = 6.0,
            });
            return true;  // `card` may have moved
        }
        if (card.n.seconds > 0.0) {
            card.going = true;
        }
        return true;
    }
    return false;
}

void notify_menu_hint() {
    // The desktop's menu key or Android's Back, and the controller's menu button.
    std::vector<std::string> keys;
    keys.push_back(drawer_possible() ? std::string("Back") : menu_key_label());
    if (menu_bar_chord() || menu_bar_button() != SDL_GAMEPAD_BUTTON_INVALID) {
        keys.push_back(menu_bar_button_label());
    }
    notify({
        .id = "menu.hint",
        .title = "Open the menu any time",
        .keys = std::move(keys),
        .tone = Tone::Info,
        .seconds = 7.0,
    });
}

bool notifications_pending() {
    return !slots.empty();
}

void draw_notifications() {
    const float dt = ImGui::GetIO().DeltaTime;
    // The menu and the touch pad editor hide the cards and pause their timers.
    const bool hidden = s.menu_open || s.menu_at > 0.0f || s.touch_editing;
    stack_shown = slide(stack_shown, !hidden);
    for (Slot& card : slots) {
        if (!hidden && !card.going && card.n.seconds > 0.0) {
            card.left -= dt;
            card.going = card.left <= 0.0;
        }
        if (card.going) {
            card.out = std::min(card.out + dt / kOut, 1.0f);
        } else if (!hidden) {
            card.in = std::min(card.in + dt / kIn, 1.0f);
        }
    }
    std::erase_if(slots, [](const Slot& card) { return card.going && card.out >= 1.0f; });
    for (Slot& card : slots) {
        card.alpha = 0.0f;
    }
    if (slots.empty() || stack_shown <= 0.0f) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Below a phone's notch.
    float top = viewport->Pos.y + dp(14.0f);
    SDL_Rect safe{};
    if (s.window != nullptr && SDL_GetWindowSafeArea(s.window, &safe)) {
        top += static_cast<float>(safe.y);
    }
    const float most = std::min(viewport->Size.x - dp(32.0f), dp(560.0f));
    ImDrawList* list = ImGui::GetForegroundDrawList();
    const float glide = 1.0f - std::exp(-dt * 14.0f);
    float y = top;
    for (Slot& card : slots) {
        const Layout l = measure(card.n, most - dp(14.0f) * 2.0f - dp(46.0f));
        const float h = std::max(dp(34.0f), l.height) + dp(14.0f) * 2.0f;
        const float width =
            std::clamp(l.width + dp(14.0f) * 2.0f + dp(46.0f) + dp(8.0f), dp(260.0f), most);
        const float come = ease_out(card.in);
        const float go = ease_out(card.out);
        card.y = card.placed ? card.y + (y - card.y) * glide : y;
        card.placed = true;
        // Slides down and grows a little on the way in; slides up on the way out.
        const float drop = -dp(28.0f) * (1.0f - come) - dp(18.0f) * go;
        const float scale = 0.94f + 0.06f * come;
        const float alpha = come * (1.0f - go) * ease_out(stack_shown);
        const float w = width * scale;
        const ImVec2 a(viewport->Pos.x + (viewport->Size.x - w) * 0.5f, card.y + drop);
        if (alpha > 0.0f) {
            draw_card(list, card, a, w, l, alpha);
        }
        card.a = a;
        card.b = ImVec2(a.x + w, a.y + h);
        card.alpha = alpha;
        // The next card goes below; a leaving card gives up its space as it fades.
        y += (h + dp(10.0f)) * (1.0f - go);
    }
}

}  // namespace openrac::frontend::debug_overlay
