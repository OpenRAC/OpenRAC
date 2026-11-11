// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The in-game menu's pieces, in the launcher's look (ui_colors.h, launcher/docs/STYLE.md): the rows
// of options on cards, their switches and arrows, the headings, buttons and notes, and the menu's
// entries (menu_catalog.h) drawn with them. The desktop's menu (debug/menu_window.cpp) and the touch
// screen's drawer (debug/drawer.cpp) are two frames around the same rows. Only the overlay's files
// include it.

#pragma once

#include <cstdint>
#include <functional>
#include <imgui.h>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL_gamepad.h>

#include "audio/ui_sounds.h"
#include "debug/overlay.h"
#include "menu_catalog.h"
#include "ui_colors.h"

namespace openrac::frontend::debug_overlay {

// ---- The look ----

// The launcher's colours, as the launcher started the game with them.
const ui_colors::Colors& ui();

// A row or button held down or under the pointer: the launcher's --control-card-hi, a neutral lift.
ImU32 pressed_fill();
// A plain button's fill in place of a border: the launcher's --control-card, as the menu's
// controls sit on a panel.
ImU32 control_fill();
// The launcher's transitions: what changes eases to its new state over `seconds`; `id` is ImGui's.
float tween(ImGuiID id, float target, float seconds);

// One of the last item's parts that eases on its own (its press, its focus, a switch's knob).
ImGuiID item_part(const char* part);

constexpr float kFast = 0.15f;    // the launcher's --fast
constexpr float kMedium = 0.28f;  // --medium

// The launcher's --ease, near enough: fast at first, then settling. `t` from 0 to 1.
float ease_out(float t);

// Moves `at` (0 shut, 1 open) one frame towards open over kMedium, or towards shut over 0.2 s.
float slide(float at, bool open);

// The menu's typefaces (the launcher's: Inter for text, Orbitron for headings), added to ImGui's
// atlas beside the overlay's own font; each is ImGui's default font when its file could not be
// read. Set by add_menu_fonts.
struct MenuFonts {
    ImFont* body = nullptr;    // Inter, the rows
    ImFont* strong = nullptr;  // Inter SemiBold, the buttons
    ImFont* bar = nullptr;     // Inter Medium, the bar's tabs and chips (as thin as the logo)
    ImFont* head = nullptr;    // Orbitron, the headings
    ImFont* title = nullptr;   // Orbitron, larger: the open tab's name over its rows
    ImFont* small = nullptr;   // Inter SemiBold, smaller: a section's kicker, the version
    ImFont* panel = nullptr;   // Inter, the debug panel's and the other overlay windows' text
};

extern MenuFonts menu_fonts;

// Adds the menu's typefaces to the atlas at s.ui_scale (debug_overlay.cpp's style_and_font).
void add_menu_fonts(bool touch);

// The logo from the font atlas (debug_overlay.cpp, add_logo): its mark in `mark`, its name in
// `name`. `left_middle` is its left edge at the height of its letters.
void draw_logo(ImDrawList* list, ImVec2 left_middle, ImU32 mark, ImU32 name);

// The focus ring a gamepad or the keyboard moves, around the item just drawn.
void focus_ring(ImDrawList* list, ImVec2 a, ImVec2 b, float round);

// The main button's fill: the accent, flat; darker as `held`.
void primary_fill(ImDrawList* list, ImVec2 a, ImVec2 b, float round, float held);
// A panel's ground: a rounded rectangle from `top` at its top to `bottom` at its foot, with a glow
// of `glow` along its top `glow_depth` pixels, rounded at `corners`.
void panel_fill(
    ImDrawList* list,
    ImVec2 a,
    ImVec2 b,
    ImU32 top,
    ImU32 bottom,
    ImU32 glow,
    float glow_depth,
    float round,
    ImDrawFlags corners = ImDrawFlags_None
);
// The light along a panel's top: its card coal with 8 % of the game's colour (--glow), as the
// launcher's page under its bar; more looks odd on the coal.
ImU32 panel_glow();

// An arrow as the launcher draws one, pointing `dx` (-1, 1) or, with `dx` 0, `dy` (1 down, -1 up).
void arrow(ImDrawList* list, ImVec2 tip, float dx, float dy, ImU32 ink);

// A soft shadow under the box [a, b]: `layers` faint copies, each a little larger, reaching
// `spread` out and moved `drop` down; `alpha` is one layer's opacity (0 to 1).
void soft_shadow(
    ImDrawList* list,
    ImVec2 a,
    ImVec2 b,
    float round,
    float spread,
    float drop,
    int layers,
    float alpha
);

// Draws a key or pad button as a rounded cap (menu footer, notifications), left edge at `x`,
// vertical centre at `middle`. Returns its width.
float key_cap(ImDrawList* list, float x, float middle, const std::string& key, float alpha);
float key_cap_width(const std::string& key);
float key_cap_height();

// Keys pressed together as caps with a "+" between them ("Ctrl+F3"). Returns their width;
// draws them only when `list` is set.
float key_caps(ImDrawList* list, float x, float middle, std::string_view keys, float alpha);

// The make of the first controller connected, and a button's name as that make prints it
// (A, Cross, LB, R1 ...).
SDL_GamepadType pad_type();
std::string pad_name(SDL_GamepadType type, SDL_GamepadButton button);

// Where to draw `text` so its letters (not its line box) are centred on `centre`: across by the
// ink, from the first letter's left edge to the last's right; down by a capital's height.
ImVec2 centred_text(ImFont* font, float size, std::string_view text, ImVec2 centre);

// The y to draw `font` text at so it lines up with caps centred at `middle`.
float beside_caps(const ImFont* font, float middle);

// ---- Rows on cards ----

// A section's rows on one card: begun before its first row, ended after its last.
void card_begin();
void card_end();

// A section's heading above its card, in the accent's gold.
void menu_heading(const char* title);

// A row of the card open now: its label at the left, `right` drawing what it shows at its right
// (from its right edge, inset, at its middle). True when it was pressed (a finger that dragged the
// rows along presses none). As high as a thumb on a touch screen, a little lower on a desktop.
// `right_width` > 0: what `right` draws takes that much, and a longer label ends in "...".
using RowRight = std::function<void(ImDrawList* list, ImVec2 right)>;
bool menu_row(const char* label, ImU32 ink, const RowRight& right, float right_width = 0.0f);
bool menu_row(const char* label, ImU32 ink = ui().text);

// A row that turns something on and off: true when pressed.
bool menu_switch(const char* label, bool on);

// A row with its value between two arrows: -1 or 1 when it was moved back or on (a press on the
// value's left part moves it back, anywhere else on; left and right while it has the focus).
int menu_choice(
    const char* label, const std::string& value, bool can_lower = true, bool can_raise = true
);

// A percentage slider in a menu row, between two arrows: the new value after a drag, a tap on the
// track or an arrow, or left and right while it has the focus (the d-pad).
int menu_percent_slider(const char* label, int value, int minimum, int maximum, int step);

// The launcher's primary button, across the space left.
bool menu_button(const char* label);

// A line of the card, in `ink` (the launcher's soft text by default): what is not pressed.
void menu_note(const char* line, ImU32 ink = ui().soft);

// A list of keys or buttons and what each does, one per line in a card, the caps on the left
// and the words lined up after them.
struct KeyRow {
    std::vector<std::string> keys;
    std::string does;
};

void menu_key_rows(const std::vector<KeyRow>& rows);

// ---- The menu's entries ----

// Draws the entries of `tab` offered on `where` (menu::Where), a heading and a card for each
// section with anything to show. The entry under the pointer or the focus is kept in s.menu_hover
// (for the desktop's help). False when nothing of the tab is shown.
// With `only`, draws just the sections (by heading) it returns true for.
using SectionFilter = std::function<bool(std::string_view section)>;
bool draw_tab(menu::Tab tab, std::uint8_t where, const SectionFilter& only = {});

// True for the Controls tab's controller sections (bindings, sticks, players); a touch screen
// shows them in their own panel next to the drawer.
bool controller_section(std::string_view section);

// Says whether `tab` has anything to show on `where` now.
bool tab_shown(menu::Tab tab, std::uint8_t where);

// Draws entry `id` alone (its row, or nothing while it is hidden).
void draw_entry(menu::Id id, std::uint8_t where);

// Does what entry `id` does when pressed (a switch turns over, a choice moves on), outside the rows:
// a button of the menu's own (the desktop's Quit).
void press_entry(menu::Id id);

// Ends the game as the window's close button does (Quit, once confirmed).
void quit_game();

// Quit's confirmation as rows of the card open now: "Back to the game" first, so a press of A
// that came too soon does not quit, then "Quit game". Over them, unless `note` is false (the
// window's Quit page says it on the right), that the progress since the last save is lost.
void quit_confirm_rows(bool note = true);

// What the picture is drawn with ("Vulkan · Adreno ..."), for the menus' foot.
const std::string& menu_graphics_line();

// The key that opens and closes the menu, as the player reads it (menu_bar_key_label).
std::string menu_key_label();

// An entry's label now, and its help: what the catalog says, or what it says of itself now.
std::string entry_label(menu::Id id, std::uint8_t where);

// Remembers `entry` (a menu::Id) as the one under the pointer or the focus when the item just drawn
// is, for the desktop's help: a focus moved by a controller or keys wins over the pointer.
void note_hover(int entry);

// ---- The Controls tab (debug/menu_controls.cpp) ----

// What one of the Controls tab's plain entries shows now (its file, its player, a setting).
struct ControlsRow {
    bool shown = true;
    std::string label;  // empty: the catalog's
    bool on = false;
    std::string value;
    bool can_lower = true;
    bool can_raise = true;
};

ControlsRow controls_row(menu::Id id);

// Does what a plain Controls entry does: `by` -1 or 1 for a choice moved, 1 for a press.
void controls_act(menu::Id id, int by);

// Draws a Controls panel (menu::Kind::Panel): its rows on the card open now.
void draw_controls_panel(menu::Id id);

// Each frame: a press awaited for a binding arrives, gives up after a while, or ends with the
// menu.
void controls_frame();

// Whether a press is awaited for a binding: the controllers' presses are not the menu's meanwhile.
bool controls_capturing();
void controls_cancel_capture();

// The mods' developer menus (mods::developer_menus): how many, a menu's title, and its items as
// rows, a heading for each group.
int mod_menu_count();
std::string mod_menu_title(int index);
void draw_mod_menu(int index);

// The menu's state as both frames share it: opening pauses the game (as an emulator's menu does)
// and closing leaves it as Keep paused says; a screenshot asked for is taken once it is gone.
void set_menu(bool open);
void menu_closed();

// The menu's own soft tones (audio/ui_sounds.h).
using MenuSound = audio::ui_sounds::Sound;
// Plays one now, unless one already played this frame (debug/menu_sound.cpp).
void menu_sound(MenuSound sound);
// Once a frame, after the menu is built: the menu opened or shut, its tab or page changed, the
// focus moved to another row.
void menu_sounds_frame();

}  // namespace openrac::frontend::debug_overlay
