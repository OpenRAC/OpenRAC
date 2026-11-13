// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's drawer (debug/overlay.h): the menu on a touch screen, a panel that slides out
// from the left with the menu's entries (menu_catalog.h) on cards, drawn by debug/menu_ui.h as the
// desktop's menu draws them, with rows as high as a thumb; and the bar shown while the on-screen
// pad's layout is edited.

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <format>
#include <imgui.h>
#include <imgui_internal.h>  // ImGui::FindRenderedTextEnd: a label's text before its "##" id
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"
#include "renderer/gl/gl.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {

namespace {

// The drawer's ground, as the desktop help pane's (user): the panel under the deep ink at 55 %.
void drawer_ground(ImDrawList* list, ImVec2 a, ImVec2 b, ImDrawFlags corners) {
    list->AddRectFilled(a, b, ui().panel, dp(12.0f), corners);
    list->AddRectFilled(a, b, ui_colors::fade(ui().ink_deep, 0.55f), dp(12.0f), corners);
}

// Where the drawer's faint stripes end: 300 dp across at most, and down to the header's foot, so
// they fade out above the line under it (user).
ImVec2 stripes_end(ImVec2 top, float width, float header) {
    return ImVec2(top.x + std::min(width - 1.0f, dp(300.0f)), top.y + header);
}

// What the picture is drawn with: the launcher's renderer choice takes effect only when the game
// starts, and here the player sees which one this run has.
const std::string& graphics_line() {
    static const std::string graphics = [] {
        const auto* name =
            reinterpret_cast<const char*>(openrac::gl::glGetString(openrac::gl::GL_RENDERER));
        const char* api = s.gl_subset ? "Vulkan" : openrac::gl::is_es() ? "OpenGL ES" : "OpenGL";
        std::string gpu = name != nullptr ? name : "unknown GPU";
        // On Vulkan the renderer's GL names the GPU "<name> (Vulkan)": the API is said already.
        if (constexpr std::string_view vk = " (Vulkan)"; gpu.ends_with(vk)) {
            gpu.resize(gpu.size() - vk.size());
        }
        return std::format("{} · {}", api, gpu);
    }();
    return graphics;
}

// Style shared by the drawer's windows while this object lives: no ImGui background, border or
// focus frame (we draw our own), no item spacing, a thin scrollbar.
class PanelStyle {
public:
    explicit PanelStyle(ImVec2 padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, dp(4.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(
            ImGuiCol_ScrollbarGrab, ImGui::ColorConvertU32ToFloat4(ui().line_strong)
        );
        ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4(0, 0, 0, 0));
    }

    ~PanelStyle() {
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(5);
    }

    PanelStyle(const PanelStyle&) = delete;
    PanelStyle& operator=(const PanelStyle&) = delete;
};

// A row that folds a section away, its arrow turning as it opens: true when pressed.
bool fold_row(const char* label, bool open) {
    card_begin();
    const bool pressed = menu_row(label, ui().text, [open](ImDrawList* list, ImVec2 right) {
        const float k = tween(item_part("open"), open ? 1.0f : 0.0f, kMedium);
        const float dy = 1.0f - 2.0f * k;
        arrow(list, ImVec2(right.x - dp(5.0f), right.y + dp(2.5f) * dy), 0.0f, dy, ui().soft);
    });
    card_end();
    return pressed;
}

// Controller settings in a panel right of the drawer, filling the rest of the screen: the list is
// too long for the drawer itself. `left` is the drawer's right edge, `right_inset` the notch's.
void controller_panel(float left, float right_inset) {
    const bool out = s.drawer_controller && s.menu_open;
    s.drawer_controller_at = slide(s.drawer_controller_at, out);
    if (s.drawer_controller_at <= 0.0f) {
        return;
    }
    const float ease = ease_out(s.drawer_controller_at);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float gap = dp(12.0f);
    // Slides in from the left and fades in.
    const float x = left + gap + dp(40.0f) * (1.0f - ease);
    const float width = viewport->Pos.x + viewport->Size.x - right_inset - gap - (left + gap);
    const ImVec2 a(x, viewport->Pos.y + gap);
    const ImVec2 size(width, viewport->Size.y - gap * 2.0f);
    ImGui::SetNextWindowPos(a);
    ImGui::SetNextWindowSize(size);
    if (s.drawer_controller_focus && out) {
        ImGui::SetNextWindowFocus();
    }
    const float header = dp(56.0f);
    const PanelStyle style(ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ease);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                                   | ImGuiWindowFlags_NoScrollbar
                                   | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("OpenRAC controller settings", nullptr, flags)) {
        ImDrawList* list = ImGui::GetWindowDrawList();
        const ImVec2 b(a.x + size.x, a.y + size.y);
        // The drawer's ground: its coal, lit along the top in the game's colour.
        panel_fill(
            list,
            a,
            b,
            ui_colors::fade(ui().panel, ease),
            ui_colors::fade(ui().panel, ease),
            ui_colors::fade(panel_glow(), ease),
            dp(160.0f),
            dp(12.0f)
        );
        // Header: title and a close button (B also closes it).
        ImFont* head = menu_fonts.head;
        list->AddText(
            head,
            head->FontSize,
            ImVec2(a.x + dp(22.0f), a.y + (header - head->FontSize) * 0.5f),
            ui_colors::fade(ui().gold, ease),
            "Controller settings"
        );
        const float close = dp(40.0f);
        ImGui::SetCursorScreenPos(ImVec2(b.x - close - dp(10.0f), a.y + (header - close) * 0.5f));
        if (ImGui::InvisibleButton("close", ImVec2(close, close))) {
            s.drawer_controller = false;
            s.menu_focus = true;  // the gamepad's focus goes back to the drawer
        }
        const ImVec2 c(b.x - close * 0.5f - dp(10.0f), a.y + header * 0.5f);
        const float held = tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast);
        list->AddCircleFilled(c, close * 0.5f, ui_colors::mix(ui().panel_hi, pressed_fill(), held));
        const float k = dp(6.0f);
        list->AddLine(ImVec2(c.x - k, c.y - k), ImVec2(c.x + k, c.y + k), ui().soft, dp(1.6f));
        list->AddLine(ImVec2(c.x + k, c.y - k), ImVec2(c.x - k, c.y + k), ui().soft, dp(1.6f));
        list->AddLine(
            ImVec2(a.x + dp(1.0f), a.y + header), ImVec2(b.x - dp(1.0f), a.y + header), ui().line
        );
        // Rows in a column of readable width at the panel's right, near the thumb, scrolling
        // under the header. A little padding keeps the cards' outlines inside the child's clip,
        // and no scrollbar takes their right edge (a finger drags the rows).
        const float column = std::min(size.x - dp(28.0f), dp(720.0f));
        ImGui::SetCursorScreenPos(ImVec2(b.x - dp(14.0f) - column, a.y + header + dp(1.0f)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp(2.0f), dp(6.0f)));
        if (ImGui::BeginChild(
                "rows",
                ImVec2(column, size.y - header - dp(2.0f)),
                ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened,
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar
            )) {
            drag_scroll(s.drawer_controller_scroll);
            if (s.drawer_controller_focus && out) {
                s.menu_focus_first = true;  // a gamepad starts at its first row
            }
            draw_tab(menu::Tab::Controls, menu::kTouch, controller_section);
            ImGui::Dummy(ImVec2(0.0f, dp(16.0f)));
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    s.drawer_controller_focus = false;
    ImGui::End();
    ImGui::PopStyleVar();
}

}  // namespace

const std::string& menu_graphics_line() {
    return graphics_line();
}

// The drawer's sections for its jumps: where each starts, and where a jump is gliding to.
// The pad's buttons as the launcher draws them: a round one, or a rounded pill for a long name
// (LB, Cross); A filled in the accent. The name is centred by its letters, not its line box.
float pad_glyph(
    ImDrawList* list, float x, float middle, const std::string& name, bool primary, float alpha
) {
    ImFont* font = menu_fonts.strong;
    const float size = std::round(menu_fonts.body->FontSize * 0.8f);
    const float h = std::round(dp(22.0f));
    const float text = font->CalcTextSizeA(size, FLT_MAX, 0.0f, name.c_str()).x;
    const float w = name.size() > 1 ? std::round(std::max(h, text + dp(12.0f))) : h;
    if (list == nullptr) {
        return w;
    }
    const ImVec2 a(std::round(x), std::round(middle - h * 0.5f));
    const ImVec2 b(a.x + w, a.y + h);
    list->AddRectFilled(
        a,
        b,
        ui_colors::fade(primary ? ui().amber : ui().control_card, alpha),
        name.size() > 1 ? dp(6.0f) : h * 0.5f
    );
    const ImVec2 at =
        centred_text(font, size, name, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f));
    list->AddText(
        font, size, at, ui_colors::fade(primary ? ui().bar_ink : ui().text, alpha), name.c_str()
    );
    return w;
}

// What the pad's buttons do, at the screen's foot on the right as in the launcher: shown while
// the pad was the last thing used.
void pad_hints(const ImGuiViewport* viewport, const SDL_Rect& safe, float ease) {
    const float shown =
        ease * tween(ImGui::GetID("pad hints"), s.menu_motion.pad ? 1.0f : 0.0f, kFast);
    if (shown <= 0.0f) {
        return;
    }

    struct Hint {
        std::vector<std::string> keys;
        const char* does;
    };

    const SDL_GamepadType type = pad_type();
    const std::string south = pad_name(type, SDL_GAMEPAD_BUTTON_SOUTH);
    const std::vector<Hint> list = {
        {{pad_name(type, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER),
          pad_name(type, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)},
         "Sections"},
        {{south}, "Select"},
        {{pad_name(type, SDL_GAMEPAD_BUTTON_EAST)}, "Back to the game"},
    };
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImFont* body = menu_fonts.body;
    const float gap = dp(6.0f);
    const float between = dp(16.0f);
    // Measured first (drawn nowhere), then drawn from the left so the last ends at the edge.
    float total = 0.0f;
    for (const Hint& hint : list) {
        for (const std::string& key : hint.keys) {
            total += pad_glyph(nullptr, 0.0f, 0.0f, key, false, 0.0f) + dp(4.0f);
        }
        total += gap - dp(4.0f) + body->CalcTextSizeA(body->FontSize, FLT_MAX, 0.0f, hint.does).x
                 + between;
    }
    total -= between;
    float right = viewport->Pos.x + viewport->Size.x;
    if (safe.w > 0) {
        right = std::min(right, viewport->Pos.x + static_cast<float>(safe.x + safe.w));
    }
    const float edge = dp(18.0f);
    float x = right - edge - total;
    const float middle = viewport->Pos.y + viewport->Size.y - edge - dp(11.0f);
    for (const Hint& hint : list) {
        for (const std::string& key : hint.keys) {
            x += pad_glyph(draw, x, middle, key, key == south, shown) + dp(4.0f);
        }
        x += gap - dp(4.0f);
        draw->AddText(
            body,
            body->FontSize,
            ImVec2(std::round(x), beside_caps(body, middle)),
            ui_colors::fade(ui().text, shown),
            hint.does
        );
        x += body->CalcTextSizeA(body->FontSize, FLT_MAX, 0.0f, hint.does).x + between;
    }
}

float s_jump_marks[4] = {};
float s_jump_to = -1.0f;
// The section a jump goes to, and how long after it lands the view stays on its heading: the
// rows above can still change height then (a fold opening, rows laid out for the first time), and
// a target taken once at the tap then landed a little above the heading.
int s_jump_section = -1;
float s_jump_hold = 0.0f;
// The section LB or RB jumped to: the pad's focus goes to its first row as it is drawn.
int s_jump_focus = -1;

void drawer(const Shown& shown) {
    const ImGuiIO& io = ImGui::GetIO();
    // Out in the launcher's --medium, back in a little quicker, as its pages come and go.
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
    s.menu_row_height = 48.0f;
    const float ease = ease_out(s.menu_at);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // The left of the screen a camera's notch keeps.
    float inset = 0.0f;
    SDL_Rect safe{};
    if (shown.window != nullptr && SDL_GetWindowSafeArea(shown.window, &safe)) {
        inset = static_cast<float>(safe.x);
    }
    const float width = std::min(viewport->Size.x * 0.6f, dp(360.0f)) + inset;
    const float x = viewport->Pos.x - width * (1.0f - ease);
    // The game blurred and dimmed under it, as under the desktop's menu.
    game_backdrop(viewport, shown, ease);
    // A tap right of the drawer or B closes it. With the controller panel open, B closes the panel
    // first and taps go to the panel.
    if (s.menu_open && ImGui::IsMouseReleased(0) && !s.menu_scroll.dragged
        && io.MouseClickedPos[0].x > x + width && !s.drawer_controller) {
        set_menu(false);
    }
    if (s.menu_open && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
        if (s.drawer_controller) {
            s.drawer_controller = false;
            s.menu_focus = true;
        } else {
            set_menu(false);
        }
    }
    ImGui::SetNextWindowPos(ImVec2(x, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(width, viewport->Size.y));
    if (s.menu_focus) {
        ImGui::SetNextWindowFocus();
        s.menu_focus = false;
    }
    // The launcher's panel: its ink, a thin border, 16-pixel corners on the open side; a header
    // above the rows, which scroll under it.
    // The logo's row, then the sections' row: a tap jumps to one.
    const float head = dp(52.0f);
    const float jumps_h = dp(40.0f);
    const float header = head + jumps_h;
    std::optional<PanelStyle> style(std::in_place, ImVec2(dp(14.0f) + inset, header + dp(4.0f)));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("OpenRAC options", nullptr, flags)) {
        drag_scroll(s.menu_scroll);
        if (s_jump_section >= 0) {
            // A jump glides there in a few frames, to where the section's heading is now (the
            // last frame's layout), and stays on it a moment after; a finger on the rows stops it.
            const int i = s_jump_section;
            s_jump_to = i == 0
                            ? 0.0f
                            : std::min(s_jump_marks[i] - header - dp(4.0f), ImGui::GetScrollMaxY());
            const float now = ImGui::GetScrollY();
            const float next = now + (s_jump_to - now) * std::min(1.0f, io.DeltaTime * 14.0f);
            const bool there = std::abs(s_jump_to - next) < 1.0f;
            ImGui::SetScrollY(there ? s_jump_to : next);
            if (there) {
                s_jump_hold -= io.DeltaTime;
            }
            if ((there && s_jump_hold <= 0.0f) || ImGui::IsMouseDown(0)) {
                s_jump_section = -1;
                s_jump_to = -1.0f;
            }
        }
        {
            ImDrawList* back = ImGui::GetWindowDrawList();
            const ImVec2 top(x, viewport->Pos.y);
            const ImVec2 bottom(x + width, viewport->Pos.y + viewport->Size.y);
            back->PushClipRect(top, bottom, false);
            // The desktop help pane's ground: the panel darkened, no light from above.
            drawer_ground(back, top, bottom, ImDrawFlags_RoundCornersRight);
            back->PopClipRect();
        }
        {
            // The header: the logo, and what the game is doing while the drawer is out.
            ImDrawList* front = ImGui::GetForegroundDrawList();
            const ImVec2 top(x, viewport->Pos.y);
            const ImVec2 bottom(x + width, viewport->Pos.y + header);
            // The same ground as the panel under it, so the header does not cut it off.
            drawer_ground(front, top, bottom, ImDrawFlags_RoundCornersTopRight);
            // The desktop help pane's faint stripes, in the drawer's top left, gone by its foot.
            faint_stripes(front, top, stripes_end(top, width, header), ui().amber, 0.05f);
            // A hairline under it once rows have scrolled beneath, fading in as they do.
            const float under = std::min(ImGui::GetScrollY() / dp(24.0f), 1.0f);
            if (under > 0.0f) {
                front->AddLine(ImVec2(top.x, bottom.y), bottom, ui_colors::fade(ui().line, under));
            }
            draw_logo(
                front, ImVec2(top.x + inset + dp(18.0f), top.y + head * 0.5f), ui().amber, ui().text
            );
            // Once Resume has scrolled away, the badge becomes it: the way back to the game
            // stays one tap away however far down the rows are.
            const float resume =
                std::clamp((ImGui::GetScrollY() - dp(30.0f)) / dp(24.0f), 0.0f, 1.0f);
            if (resume < 1.0f) {
                const char* state = s.keep_paused ? "Paused" : "Paused while open";
                const ImVec2 state_size = ImGui::CalcTextSize(state);
                // As tall as the sections' bar under it, its right edge on the bar's.
                const ImVec2 pill_b(bottom.x - dp(14.0f), top.y + head * 0.5f + dp(15.0f));
                const ImVec2 pill_a(pill_b.x - state_size.x - dp(20.0f), pill_b.y - dp(30.0f));
                // A badge on the launcher's control fill (as the sections under it), soft text.
                const float shown_badge = 1.0f - resume;
                front->AddRectFilled(
                    pill_a, pill_b, ui_colors::fade(ui().control_card, shown_badge), dp(6.0f)
                );
                front->AddText(
                    ImVec2(
                        pill_a.x + dp(10.0f),
                        std::round((pill_a.y + pill_b.y - state_size.y) * 0.5f)
                    ),
                    ui_colors::fade(ui().soft, shown_badge),
                    state
                );
            }
            if (resume > 0.0f) {
                ImFont* font = menu_fonts.bar;
                const char* label = "Resume";
                const ImVec2 size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, label);
                // The badge's place and height, the sections' bar's under it.
                const ImVec2 pill_b(bottom.x - dp(14.0f), top.y + head * 0.5f + dp(15.0f));
                const ImVec2 pill_a(pill_b.x - size.x - dp(32.0f), pill_b.y - dp(30.0f));
                front->AddRectFilled(pill_a, pill_b, ui_colors::fade(ui().amber, resume), dp(6.0f));
                front->AddText(
                    font,
                    font->FontSize,
                    ImVec2(pill_a.x + dp(16.0f), std::round((pill_a.y + pill_b.y - size.y) * 0.5f)),
                    ui_colors::fade(ui().bar_ink, resume),
                    label
                );
                // A tap on it (not the end of a drag) resumes; the pill is 30 dp tall, its tap
                // area the header's full height.
                const ImVec2 at = io.MouseClickedPos[0];
                if (resume >= 1.0f && s.menu_open && ImGui::IsMouseReleased(0)
                    && !s.menu_scroll.dragged && at.x >= pill_a.x - dp(6.0f) && at.x <= bottom.x
                    && at.y >= top.y && at.y <= top.y + head) {
                    menu_sound(MenuSound::Confirm);
                    set_menu(false);
                }
            }
            // The sections, as a segmented control: the one in view in the accent; a tap scrolls
            // to its heading. Where each starts is from the last frame's layout (marks).
            static constexpr const char* kJumps[] = {"Graphics", "Enhanced", "Game", "Controls"};
            const float scroll = ImGui::GetScrollY();
            int in_view = 0;
            for (int i = 0; i < 4; ++i) {
                if (s_jump_marks[i] > 0.0f && scroll >= s_jump_marks[i] - header - dp(8.0f)) {
                    in_view = i;
                }
            }
            // A section tapped is lit at once, while the list still glides to it.
            if (s_jump_section >= 0) {
                in_view = s_jump_section;
            }
            const float left = top.x + inset + dp(14.0f);
            const float span = (bottom.x - dp(14.0f) - left) / 4.0f;
            const float y0 = top.y + head + dp(2.0f);
            const float y1 = y0 + dp(30.0f);
            front->AddRectFilled(
                ImVec2(left, y0), ImVec2(left + span * 4.0f, y1), ui().control_card, dp(6.0f)
            );
            for (int i = 0; i < 4; ++i) {
                const ImVec2 ca(left + span * i, y0);
                const ImVec2 cb(left + span * (i + 1), y1);
                const float on = tween(ImGui::GetID(kJumps[i]), i == in_view ? 1.0f : 0.0f, kFast);
                if (on > 0.0f) {
                    // The whole cell, as tall as Resume above it.
                    front->AddRectFilled(ca, cb, ui_colors::fade(ui().amber, on), dp(6.0f));
                }
                ImFont* font = menu_fonts.bar;
                const ImVec2 size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, kJumps[i]);
                front->AddText(
                    font,
                    font->FontSize,
                    ImVec2((ca.x + cb.x - size.x) * 0.5f, (ca.y + cb.y - size.y) * 0.5f),
                    ui_colors::mix(ui().soft, ui().bar_ink, on),
                    kJumps[i]
                );
                const ImVec2 at = io.MouseClickedPos[0];
                if (s.menu_open && ImGui::IsMouseReleased(0) && !s.menu_scroll.dragged
                    && at.x >= ca.x && at.x < cb.x && at.y >= top.y + head && at.y <= top.y + header
                    && s_jump_marks[i] > 0.0f) {
                    menu_sound(MenuSound::Page);
                    s_jump_section = i;
                    s_jump_hold = 0.3f;
                    s.menu_scroll.velocity = 0.0f;
                }
            }
            // LB and RB step through the sections as a tap would, and the pad's focus goes along.
            const int step = ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)   ? 1
                             : ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) ? -1
                                                                              : 0;
            if (s.menu_open && !s.drawer_controller && step != 0) {
                const int to = std::clamp(in_view + step, 0, 3);
                if (to != in_view && s_jump_marks[to] > 0.0f) {
                    menu_sound(MenuSound::Page);
                    s_jump_section = to;
                    s_jump_hold = 0.3f;
                    s_jump_focus = to;
                    s.menu_scroll.velocity = 0.0f;
                }
            }
            pad_hints(viewport, safe, ease);
        }

        if (menu_button("Resume")) {
            set_menu(false);
        }
        ImGui::SetItemDefaultFocus();  // where a gamepad starts

        // The menu's tabs one after another, a heading and a card for each of their sections,
        // then the mods' own menus.
        using menu::Tab;
        // Where each section starts, for the jumps above (content's own y, as the scroll counts).
        const auto mark = [](int i) {
            s_jump_marks[i] = ImGui::GetCursorPosY();
            if (s_jump_focus == i) {
                ImGui::SetKeyboardFocusHere();
                s_jump_focus = -1;
            }
        };
        mark(0);
        draw_tab(Tab::Graphics, menu::kTouch);
        mark(1);
        draw_tab(Tab::Enhanced, menu::kTouch);
        mark(2);
        // The game's fixes are many: folded away under one row, as the developer tools are.
        const auto fixes = [](std::string_view section) {
            return section == "Game fixes";
        };
        draw_tab(Tab::Game, menu::kTouch, [&](std::string_view section) {
            return !fixes(section);
        });
        ImGui::Dummy(ImVec2(0.0f, dp(16.0f)));
        if (fold_row("Game fixes", s.drawer_fixes)) {
            s.drawer_fixes = !s.drawer_fixes;
        }
        if (s.drawer_fixes) {
            draw_tab(Tab::Game, menu::kTouch, fixes);
        }
        // Controller settings open in their own panel; touch settings stay in the drawer.
        mark(3);
        menu_heading("Controls");
        card_begin();
        const bool controller = s.drawer_controller;
        if (menu_row(
                "Controller settings", ui().text, [controller](ImDrawList* list, ImVec2 right) {
                    const float k = tween(item_part("open"), controller ? 1.0f : 0.0f, kMedium);
                    arrow(
                        list,
                        ImVec2(right.x - dp(2.0f) + dp(3.0f) * k, right.y),
                        1.0f,
                        0.0f,
                        ui_colors::mix(ui().soft, ui().amber, k)
                    );
                }
            )) {
            s.drawer_controller = !s.drawer_controller;
            s.drawer_controller_focus = s.drawer_controller;
            s.menu_focus = !s.drawer_controller;
        }
        card_end();
        draw_tab(Tab::Controls, menu::kTouch, [](std::string_view section) {
            return !controller_section(section);
        });
        for (int i = 0; i < mod_menu_count(); ++i) {
            ImGui::PushID(i);
            draw_mod_menu(i);
            ImGui::PopID();
        }

        // The developer's tools, folded away under one row.
        ImGui::Dummy(ImVec2(0.0f, dp(16.0f)));
        const bool developer = s.drawer_developer;
        if (fold_row("Developer tools", developer)) {
            s.drawer_developer = !s.drawer_developer;
        }
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
    style.reset();
    float right_inset = 0.0f;
    if (shown.window != nullptr && SDL_GetWindowSafeArea(shown.window, &safe)) {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(shown.window, &w, &h);
        right_inset = static_cast<float>(w - safe.x - safe.w);
    }
    controller_panel(x + width, right_inset);
}

namespace {

// A button of the editing bar, the launcher's main one or a plain one: true when pressed.
bool bar_button(const char* label, float width, bool primary = false) {
    const float height = dp(44.0f);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + width, a.y + height);
    const bool pressed =
        ImGui::InvisibleButton(label, ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    ImDrawList* list = ImGui::GetWindowDrawList();
    const float round = dp(6.0f);
    const float held = tween(item_part("held"), ImGui::IsItemActive() ? 1.0f : 0.0f, kFast);
    if (primary) {
        primary_fill(list, a, b, round, held);
    } else {
        list->AddRectFilled(a, b, ui_colors::mix(control_fill(), pressed_fill(), held), round);
    }
    focus_ring(
        list,
        ImVec2(a.x - dp(3.0f), a.y - dp(3.0f)),
        ImVec2(b.x + dp(3.0f), b.y + dp(3.0f)),
        round + dp(3.0f)
    );
    const char* end = ImGui::FindRenderedTextEnd(label);
    list->AddText(
        centred_text(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            std::string_view(label, static_cast<std::size_t>(end - label)),
            ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f)
        ),
        primary ? ui().bar_ink : ui().text,
        label,
        end
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
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
        // The control a finger last took hold of: its own size, apart from the rest.
        if (s.touch_selected < 0.0f) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ui().dim));
            ImGui::TextUnformatted("Tap a button to change its own size.");
            ImGui::PopStyleColor();
        } else {
            if (bar_button("-##button_smaller", button)) {
                --s.touch_size_steps;
            }
            ImGui::SameLine();
            const std::string own = std::format(
                "This button {}%##button_size",
                static_cast<int>(std::lround(s.touch_selected * 100.0f))
            );
            bar_button(own.c_str(), wide * 2.0f + gap);
            ImGui::SameLine();
            if (bar_button("+##button_larger", button)) {
                ++s.touch_size_steps;
            }
        }
        s.bar_min = ImGui::GetWindowPos();
        s.bar_max =
            ImVec2(s.bar_min.x + ImGui::GetWindowSize().x, s.bar_min.y + ImGui::GetWindowSize().y);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

}  // namespace openrac::frontend::debug_overlay
