// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Keyboard and mouse for Going Commando, built into the port (keyboard_mouse.h). What every game
// does alike (the settings, binds, sticks, cursor and pad report) is game/common/keyboard_mouse's
// (core.hpp, kbm.hpp, the shared manifest); this file adds Going Commando's own: where the game is,
// from the on-screen pad's context (touch.c), its menus under the mouse (menus.hpp, places.hpp)
// and its follow and first-person cameras, which the mouse turns itself (follow_mouse.h). Out of
// them the mouse is the right stick. It writes the pad report, the fonts' button icons
// (key_prompts.h) and, in play, those cameras' views, never other game state.

#include "keyboard_mouse.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "game/common/keyboard_mouse/core.hpp"
#include "game/rac2/host/follow_mouse.h"
#include "game/rac2/host/gui_texts.h"
#include "game/rac2/host/ui_quads.h"
#include "openrac/game_host.h"
#include "openrac/mod.hpp"
#include "places.hpp"

namespace {

using kbm::Action;
using kbm::Context;

// The game's fonts on each disc (key_prompts.h): the effect bank's word and the effect texture
// table.
struct Disc {
    const char* version;
    std::uint32_t font_bank;
    std::uint32_t font_table;
};

constexpr std::array<Disc, 3> kDiscs = {{
    {"ntsc", 0x001B1604u, 0x001B9740u},
    {"pal", 0x001B1604u, 0x001B9740u},
    {"ntsc2", 0x001B1584u, 0x001B96C0u},
}};

constexpr std::uint32_t kRamBytes = 32u << 20;
constexpr std::uint32_t kNoTick = 0xFFFFFFFFu;

openrac::Mod* s_mod = nullptr;
const Disc* s_disc = nullptr;

// The settings, read at load and again when they change.
kbm::Settings s_set;
float s_last_px = -1.0f;  // the pointer's last place in a menu, game pixels
float s_last_py = -1.0f;

// What the keys and the mouse hold now.
kbm::Virtual s_pad;
kbm::KeyStick s_keys;
kbm::MouseLook s_look;
kbm::RingPointer s_ring;
kbm::Latch s_latch;
kbm::Pulser s_pulses;
kbm::MenuDirections s_dirs;
bool s_ring_open = false;
std::uint32_t s_tick = kNoTick;
kbm::Capture s_capture;
openrac::key_prompts::Painter s_painter;

// The menus' texts the game drew last (gui_texts.h), kept while no frame drew any for a few ticks;
// a clicked text the selection is on its way to, and the ticks it may still take; whether the
// button of a click the mouse took is still down.
std::array<kbm::MenuText, OPENRAC_RAC2_GUI_TEXTS_MAX> s_texts{};
int s_text_count = 0;
int s_texts_age = 0;
constexpr int kTextsKeptTicks = 3;
bool s_click_pending = false;  // a clicked list item the selection is on its way to
std::uint16_t s_click_step = 0;
int s_click_left = 0;  // steps still to press, then Cross
int s_click_wait = 0;
bool s_click_confirm = true;        // Cross after the steps (not for a vendor's icon)
int s_hover_target = -1;            // the text the mouse last sent the selection to
constexpr int kClickStepTicks = 4;  // ticks between them, for the menu to follow
bool s_click_held = false;
kbm::GridSeek s_grid;       // a grid's selection frame on its way to a clicked cell
kbm::RingSeek s_ring_seek;  // the Quick Select ring's highlight on its way to a clicked slot
bool s_map_over = false;    // the pointer over the pause menu's map: the wheel zooms it, not steps
bool s_dragging = false;    // the button held since a press on a grid's cell, there
float s_drag_x = 0.0f;
float s_drag_y = 0.0f;
std::array<kbm::GuiSprite, kbm::kGridSpritesMax> s_sprites{};
int s_sprite_count = 0;
std::array<kbm::StripIcon, 16> s_strip{};  // a vendor's strip of item icons
int s_strip_count = 0;
std::array<kbm::PromptSpot, OPENRAC_RAC2_PROMPT_SPOTS_MAX> s_spots{};
int s_spot_count = 0;

// The follow camera turned by the mouse itself (follow_mouse.h): the motion since its last turn,
// the tick it last turned at, and the tilt the mouse holds (the eye's elevation above Ratchet).
// While it runs the mouse is no right stick: no spring between the mouse and the view.
float s_follow_dx = 0.0f;
float s_follow_dy = 0.0f;
std::uint32_t s_follow_tick = kNoTick;
constexpr std::uint32_t kFollowSeenTicks = 2;
bool s_follows = false;   // the mouse turns the follow camera itself now
bool s_gyro_on = false;   // the gyro aimed at the last tick: the frames between follow it too
bool s_mouse_counts = false;  // the mouse counted at the last tick (cursor captured in play)
bool s_pad_look = false;  // the pad's right stick has the camera
kbm::OrbitTilt s_tilt;
kbm::OrbitLocks s_follow_locks;  // what the game locked at the camera's last turn
kbm::Vec3 s_follow_up = kbm::kWorldUp;
float s_follow_range = 0.0f;  // the game's tilt range, 0 for no tilt
// The mouse's turn the ticks since the last frame made (frontend/mouse_turn.h's tick_yaw).
float s_tick_yaw = 0.0f;
// The first-person camera (L1's view, the first-person mode) turned by the mouse itself
// (follow_mouse.h's aligned hook): the motion since its last turn, the tick it last ran at, the
// hero's up and the game's locks then. While it runs the mouse turns it, not the follow camera.
float s_aim_dx = 0.0f;
float s_aim_dy = 0.0f;
std::uint32_t s_aligned_tick = kNoTick;
bool s_aims = false;  // the mouse turns the first-person camera itself now
kbm::Vec3 s_aligned_up = kbm::kWorldUp;
kbm::OrbitLocks s_aligned_locks;
// The first-person view's tilt limit, radians from level (the game's, cam_aligned_update).
constexpr float kAlignedTiltLimit = 1.4660767f;
// The first-person camera's state: the view's tilt (radians, > 0 looks up).
constexpr std::uint32_t kAlignedTilt = 0x10u;
// The camera's data (its +0x70): the offset of the eye it wants from Ratchet, the offset it eases
// towards that, the stick state (the tilt target and its spring's speed) and the locks.
constexpr std::uint32_t kFollowWanted = 0x160u;
constexpr std::uint32_t kFollowEye = 0x170u;
constexpr std::uint32_t kFollowTiltTarget = 0x1C8u;
constexpr std::uint32_t kFollowTiltSpeed = 0x1CCu;
constexpr std::uint32_t kFollowLocks = 0x1D0u;

// A tick's input and where the game is, with the on-screen pad's context.
struct Tick : kbm::TickInput {
    const char* touch = nullptr;
};

// Takes the menus' texts drawn since the last tick.
void take_texts() {
    std::array<openrac_rac2_gui_text, OPENRAC_RAC2_GUI_TEXTS_MAX> drawn{};
    const int n = openrac_rac2_take_gui_texts(drawn.data());

    // No frame drew any: the last ones stand a little longer.
    if (n == 0 && ++s_texts_age <= kTextsKeptTicks) {
        return;
    }

    s_texts_age = 0;
    s_text_count = n;

    // Each text as the menus' pointing reads it.
    for (int i = 0; i < n; ++i) {
        const openrac_rac2_gui_text& d = drawn[static_cast<std::size_t>(i)];
        kbm::MenuText& m = s_texts[static_cast<std::size_t>(i)];
        m = kbm::MenuText{d.element, d.colour, d.x0, d.y0, d.x1, d.y1, d.cell};
    }
}

// The button icons in the texts the last whole frame drew.
void take_spots() {
    std::array<openrac_rac2_prompt_spot, OPENRAC_RAC2_PROMPT_SPOTS_MAX> drawn{};
    const int n = openrac_rac2_take_prompt_spots(drawn.data());
    for (int i = 0; i < n; ++i) {
        const openrac_rac2_prompt_spot& d = drawn[static_cast<std::size_t>(i)];
        const auto at = [](float v) {
            return static_cast<std::int32_t>(std::lround(v));
        };
        s_spots[static_cast<std::size_t>(i)] = kbm::PromptSpot{
            d.cell, kbm::Rect{at(d.x0), at(d.y0), at(d.x1 - d.x0), at(d.y1 - d.y0)}
        };
    }
    s_spot_count = n;
}

// The GUI sprite elements the last whole frame drew.
void take_sprites() {
    static std::array<openrac_rac2_ui_quad, OPENRAC_RAC2_UI_QUADS_MAX> drawn{};
    const int n = openrac_rac2_take_ui_quads(drawn.data());
    s_sprite_count = 0;
    s_strip_count = 0;
    for (int i = 0; i < n; ++i) {
        const openrac_rac2_ui_quad& d = drawn[static_cast<std::size_t>(i)];
        // A textured 40-pixel quad low on the screen: an icon of a vendor's strip.
        const float w = d.x1 - d.x0;
        const float h = d.y1 - d.y0;
        if (d.element == 0 && d.tex0 != 0 && w > 30.0f && w < 50.0f && h > 30.0f && h < 50.0f
            && d.y0 >= 300.0f && s_strip_count < static_cast<int>(s_strip.size())) {
            s_strip[static_cast<std::size_t>(s_strip_count++)] =
                kbm::StripIcon{d.x0, d.y0, d.x1, d.y1};
        }
        if (d.element != 0) {
            s_sprites[static_cast<std::size_t>(s_sprite_count++)] =
                kbm::GuiSprite{d.element, d.x0, d.y0, d.frame};
        }
    }
}

// The map (a level's, in the pause menu): where it was drawn lately, as fractions of the screen,
// and the mouse dragging it.
std::array<float, 4> s_map_box{};
int s_map_age = kTextsKeptTicks + 1;
bool s_map_drag = false;
float s_map_drag_x = 0.0f;
float s_map_drag_y = 0.0f;

// The mouse on the map: a press on it drags it (the map follows the mouse), the wheel over it
// zooms it, as the sticks do (Track/Zoom). Returns whether it took the click.
// Takes where the map was drawn since the last tick (every tick, so an old one never counts).
void take_map() {
    std::array<float, 4> box{};
    if (openrac_rac2_take_map(box.data()) != 0) {
        s_map_box = box;
        s_map_age = 0;
    } else if (s_map_age <= kTextsKeptTicks) {
        ++s_map_age;
    }
}

bool map_mouse(const Tick& t, bool click) {
    const float x = t.in->pointer_x;
    const float y = t.in->pointer_y;
    s_map_over = s_map_age <= kTextsKeptTicks && x >= s_map_box[0] && x < s_map_box[2]
                 && y >= s_map_box[1] && y < s_map_box[3];
    if (s_map_age > kTextsKeptTicks
        || !kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, false)) {
        s_map_drag = false;
    } else if (click && s_map_over) {
        s_map_drag = true;
        s_map_drag_x = x;
        s_map_drag_y = y;
    }

    if (s_map_drag && (x != s_map_drag_x || y != s_map_drag_y)) {
        openrac_rac2_map_move(x - s_map_drag_x, y - s_map_drag_y, 0.0f);
        s_map_drag_x = x;
        s_map_drag_y = y;
    }
    if (s_map_over && t.in->wheel_y != 0.0f) {
        openrac_rac2_map_move(0.0f, 0.0f, t.in->wheel_y);
    }
    return click && s_map_over;
}

// How many of the texts shown are a list's (an element or a plain print, not an icon's line).
int list_texts() {
    int n = 0;
    for (int i = 0; i < s_text_count; ++i) {
        const kbm::MenuText& m = s_texts[static_cast<std::size_t>(i)];
        n += m.shown() && m.element != 0 ? 1 : 0;
    }
    return n;
}

// The navigation to the text at (px, py) of the game's screen.
kbm::TextNav nav_at(float px, float py) {
    return kbm::text_nav(s_texts.data(), s_text_count, px, py);
}

// Whether the pointer is on a text the menus show.
bool on_text(const Tick& t) {
    float sw = kbm::kScreenWidth;
    float sh = kbm::kScreenHeight;
    openrac_rac2_screen_size(&sw, &sh);
    return nav_at(t.in->pointer_x * sw, t.in->pointer_y * sh).hit >= 0;
}

// The mouse in a menu: the item under it (a text of a list) becomes the selected one, the d-pad
// stepping to it; a click on an item not yet selected waits for the selection and then confirms.
// Returns whether the mouse took the click (it is not the game's Cross now).
bool menu_mouse(const Tick& t, bool click) {
    // A clicked item the selection is on its way to: its steps, a few ticks apart, then Cross.
    if (s_click_pending) {
        if (s_click_wait > 0) {
            --s_click_wait;
        } else if (s_pulses.idle()) {
            if (s_click_left > 0 || s_click_confirm) {
                s_pulses.push(s_click_left > 0 ? s_click_step : kbm::kCross);
            }
            s_click_pending = s_click_left-- > 0;
            s_click_wait = kClickStepTicks;
        }
    }

    // A grid's frame on its way to a clicked cell, then the Quick Select ring's highlight to its
    // slot: the next press.
    take_sprites();
    std::uint16_t seek_press = 0;
    if (s_grid.active()) {
        seek_press = s_grid.update(s_sprites.data(), s_sprite_count, s_pulses.idle());
    } else if (s_ring_seek.active()) {
        seek_press = s_ring_seek.update(s_sprites.data(), s_sprite_count, s_pulses.idle());
    }
    if (seek_press != 0) {
        s_pulses.push(seek_press);
    }

    // The mouse has no say out of a menu, or when the player turned it off.
    if (!t.pointer || !s_set.menu_mouse) {
        s_map_over = false;
        s_map_drag = false;
        return false;
    }
    if (map_mouse(t, click)) {
        return true;
    }

    // The game's screen (512 x 416 on the NTSC disc; the size it keeps, should a disc differ).
    float sw = kbm::kScreenWidth;
    float sh = kbm::kScreenHeight;
    openrac_rac2_screen_size(&sw, &sh);
    const float px = t.in->pointer_x * sw;
    const float py = t.in->pointer_y * sh;
    const kbm::TextNav nav = nav_at(px, py);
    // Moved: the pointer's place changed (the motion delta is only kept while captured).
    const bool moved = std::abs(px - s_last_px) >= 0.5f || std::abs(py - s_last_py) >= 0.5f;
    s_last_px = px;
    s_last_py = py;

    // A click on a button icon in a text, or the rest of its line: that button.
    const std::uint16_t icon = kbm::prompt_button_at(s_spots.data(), s_spot_count, px, py);
    if (click && icon != 0) {
        s_pulses.push(icon);
        return true;
    }

    // A vendor: a click on an icon of the strip selects it (the d-pad's steps there), one on the
    // item's panel buys it (Cross); the rest of its screen takes no click.
    if (t.where == Context::Vendor) {
        const int steps = kbm::gc_strip_steps(s_strip.data(), s_strip_count, px, py);
        if (click && steps != kbm::kGcNoIcon && steps != 0 && !s_click_pending) {
            s_click_pending = true;
            s_click_step = steps > 0 ? kbm::kRight : kbm::kLeft;
            s_click_left = std::abs(steps);
            s_click_wait = 0;
            s_click_confirm = false;
        } else if (click && kbm::gc_vendor_panel(px, py) && s_pulses.idle() && !s_click_pending) {
            s_pulses.push(kbm::kCross);
        }
        return click;
    }

    // A click on the item the selection is already walking to: Cross once it is there.
    if (click && nav.hit >= 0 && nav.hit == s_hover_target && s_click_pending && !s_click_confirm) {
        s_click_confirm = true;
        return true;
    }

    // A click on the selected item confirms it: a whole press, however short the click.
    if (click && nav.hit >= 0 && nav.hit == nav.selected && s_pulses.idle() && !s_click_pending) {
        s_pulses.push(kbm::kCross);
        return true;
    }

    // Over an item not selected: a click goes there and confirms; the mouse moved there steps.
    if (nav.hit >= 0 && nav.selected >= 0 && nav.hit != nav.selected) {
        if (click) {
            s_click_pending = true;
            s_click_step = nav.step;
            s_click_left = nav.steps;
            s_click_confirm = true;
            s_click_wait = 0;
            return true;
        }

        // The mouse moved onto it: the selection walks all the way there, a step at a time (not in
        // a list that scrolls: its items would move on under the mouse).
        if (moved && s_pulses.idle() && !s_click_pending && !nav.scrolls) {
            s_hover_target = nav.hit;
            s_click_pending = true;
            s_click_step = nav.step;
            s_click_left = nav.steps;
            s_click_confirm = false;
            s_click_wait = 0;
        }
    }

    // A button held from a grid's cell let go: on a slot of the Quick Select ring, the cell's
    // weapon goes there (dragged); on the cell it was pressed on, the cell is confirmed.
    const bool held = kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, false);
    if (s_dragging && !held) {
        s_dragging = false;
        const int slot = kbm::RingSeek::slot_at(s_sprites.data(), s_sprite_count, px, py);
        if (slot >= 0) {
            s_ring_seek.seek(slot);
        } else if (std::abs(px - s_drag_x) < kbm::kGridCellReach
                   && std::abs(py - s_drag_y) < kbm::kGridCellReach && s_grid.confirm()) {
            s_pulses.push(kbm::kCross);
        }
        return false;
    }

    // On no text: a slot of the Quick Select ring takes the grid's weapon (its highlight goes
    // there, then Cross); a grid's cell takes its frame there, confirmed when the button is let
    // go on it. Once the frame is known, the pointer moved onto a cell takes it there.
    if (nav.hit < 0 && click) {
        const int slot = kbm::RingSeek::slot_at(s_sprites.data(), s_sprite_count, px, py);
        if (slot >= 0) {
            s_ring_seek.seek(slot);
            return true;
        }
        if (s_grid.seek(s_sprites.data(), s_sprite_count, px, py, false)) {
            s_dragging = true;
            s_drag_x = px;
            s_drag_y = py;
            return true;
        }
        return false;
    }
    if (nav.hit < 0 && moved && s_grid.known() && !s_dragging && !s_ring_seek.active()) {
        s_grid.seek(s_sprites.data(), s_sprite_count, px, py, false);
    }

    return false;
}

// The tick's pad from the keyboard and mouse.
void step(const openrac_input* in, std::uint16_t pad_buttons, bool pad_look) {
    Tick t;
    t.in = in;
    const char* hints[OPENRAC_TOUCH_HINTS_MAX] = {};
    if (openrac_game_touch == nullptr || openrac_game_touch(&t.touch, hints) < 0) {
        t.touch = nullptr;
    }
    t.where = kbm2::context_of(t.touch);
    const bool play = t.where == Context::Play;
    kbm::where_mouse_counts(t);
    s_capture.update(*s_mod, s_set.capture, play, in);

    const Context before = s_latch.last;
    kbm::ActionSet held = s_latch.filter(t.where, kbm::actions_held(s_set.binds, t));
    if (before != t.where) {
        s_pulses.clear();
        s_click_pending = false;
        s_grid.reset();
        s_ring_seek.reset();
        s_dragging = false;
    }

    openrac_rac2_ui_quads_tick();
    take_texts();
    take_spots();
    take_map();

    // Out of play the mouse points at the menus' items; a click it takes is not the game's Cross
    // while its button stays down.
    // Going back, or a key's step, ends a clicked item's (or grid cell's, or ring slot's) way
    // there: its Cross must not land on the page gone back to, or on another item.
    const kbm::ActionSet cancels = kbm::bit(Action::MenuBack) | kbm::bit(Action::Pause)
                                   | kbm::bit(Action::MenuUp) | kbm::bit(Action::MenuDown)
                                   | kbm::bit(Action::MenuLeft) | kbm::bit(Action::MenuRight);
    if ((held & cancels) != 0) {
        s_click_pending = false;
        s_grid.reset();
        s_ring_seek.reset();
        s_dragging = false;
    }

    // Going back: the steps still queued would move the page gone back to.
    if ((held & (kbm::bit(Action::MenuBack) | kbm::bit(Action::Pause))) != 0 && !play) {
        s_pulses.clear();
    }

    if (!play) {
        const bool click = t.pointer && kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, true);
        // In a dialog a click no item takes presses nothing: Cross could answer yes. Nor in a
        // menu's list: a click beside the items does not confirm the selected one.
        const bool taken = menu_mouse(t, click);
        const bool beside = click
                            && (t.where == Context::Dialog
                                || (t.where == Context::Menu && s_set.menu_mouse
                                    && list_texts() >= 3 && !on_text(t)));
        s_click_held = taken || beside || s_click_held;

        // The click's button is still down: its confirm presses nothing.
        if (s_click_held && kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, false)) {
            held &= ~kbm::bit(Action::MenuConfirm);
        } else {
            s_click_held = false;
        }
    } else {
        s_click_pending = false;
        s_grid.reset();
        s_ring_seek.reset();
        s_dragging = false;
    }

    kbm::Virtual v;
    // A dialog: Escape goes back (Triangle), never Cross, which could answer yes.
    const bool dialog_escape = t.where == Context::Dialog && (held & kbm::bit(Action::Pause)) != 0;
    v.held = kbm::buttons_for(t.where, dialog_escape ? held & ~kbm::bit(Action::Pause) : held, -1);
    if (dialog_escape) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kTriangle);
    }
    if (kbm2::is_ship(t.touch)) {
        v.held = kbm2::ship_swap(v.held);
    }
    // The title's "Press Start": confirming presses Start as well (not in its menus).
    if (kbm2::confirm_presses_start(t.touch, list_texts())
        && (held & (kbm::bit(Action::MenuConfirm) | kbm::bit(Action::Jump))) != 0) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kStart);
    }
    if (!play) {
        s_dirs.step(s_set.binds, t, held, !s_map_over, s_pulses, v);
    } else {
        s_pulses.clear();
    }

    // The Quick Select: Triangle held while its bind is, the mouse pointing the left stick.
    const bool ring = play && (held & kbm::bit(Action::QuickSelect)) != 0;
    if (ring && !s_ring_open) {
        s_ring.reset();
    }
    s_ring_open = ring;
    kbm::left_stick(t, held, ring, s_set, s_ring, s_keys, v);

    // The first-person camera ran lately (L1's view, the first-person mode): the mouse turns it.
    const bool aligned = s_aligned_tick != kNoTick && s_tick - s_aligned_tick <= kFollowSeenTicks;
    // The gyro aims (core.hpp): "only while aiming" is a strafe, L2 or R2 held, or first person.
    const bool aiming = aligned || ((v.held | pad_buttons) & (kbm::kL2 | kbm::kR2)) != 0;
    const kbm::GyroLook gyro = ring ? kbm::GyroLook{} : kbm::gyro_look(t, s_set, aiming);
    s_gyro_on = gyro.on;
    s_mouse_counts = t.play;
    // The mouse counts with the cursor captured in play; the gyro wherever it aims.
    const bool looks = (t.play || gyro.on) && !ring && !pad_look;
    const float dx = (t.play ? in->mouse_dx : 0.0f) + gyro.dx;
    const float dy = (t.play ? in->mouse_dy : 0.0f) + gyro.dy;

    // In play with the cursor captured the mouse turns the follow camera itself (follow_mouse),
    // and is the right stick for any other camera; the pad's own stick wins.
    const bool follow = s_follow_tick != kNoTick && s_tick - s_follow_tick <= kFollowSeenTicks;
    s_pad_look = pad_look;
    s_aims = looks && aligned;
    s_follows = looks && follow && !aligned;
    if (s_aims) {
        s_aim_dx += dx;
        s_aim_dy += dy;
        s_follow_dx = 0.0f;
        s_follow_dy = 0.0f;
        s_look.reset();
    } else if (s_follows) {
        s_follow_dx += dx;
        s_follow_dy += dy;
        s_look.reset();
    } else {
        s_follow_dx = 0.0f;
        s_follow_dy = 0.0f;
        s_aim_dx = 0.0f;
        s_aim_dy = 0.0f;
        s_tilt.held = s_tilt.held && (t.play || gyro.on) && !pad_look;
        if (looks) {
            s_look.step(dx, dy, false, s_set.look);
            v.right_x = s_look.x;
            v.right_y = s_look.y;
        } else if (!play) {
            s_look.reset();
        }
    }

    s_pad = v;
    kbm::paint_prompts(
        *s_mod,
        s_painter,
        s_mod->relocate(s_disc->font_bank),
        s_mod->relocate(s_disc->font_table),
        s_set,
        kbm2::prompt_actions(t.touch)
    );
}

kbm::Vec3 read_vec(std::uint32_t a) {
    const openrac::Memory m = s_mod->memory();
    return {m.read<float>(a), m.read<float>(a + 4u), m.read<float>(a + 8u)};
}

void write_vec(std::uint32_t a, kbm::Vec3 v) {
    openrac::Memory m = s_mod->memory();
    m.write<float>(a, v.x);
    m.write<float>(a + 4u, v.y);
    m.write<float>(a + 8u, v.z);
}

// The follow camera, just before it turns by the stick (follow_mouse.h): the eye it wants and the
// one it eases towards turned and tilted alike by the mouse's motion (kbm::turn_orbit), so the
// view turns at once; the tilt held through the game's own target (the stick state and the value
// it tilts by, after the game's invert option). The camera's locks stop the mouse as they stop
// the stick.
void follow_mouse(std::uint32_t camera, std::uint32_t turn, std::uint32_t tilt) {
    (void)turn;
    s_follow_tick = s_tick;
    const float dx = s_follow_dx;
    const float dy = s_follow_dy;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    openrac::Memory m = s_mod->memory();
    const std::uint32_t data = m.read<std::uint32_t>(camera + 0x70u);
    float up[3] = {0.0f, 0.0f, 1.0f};
    if (!s_follows || data < 0x00100000u || data >= kRamBytes - 0x200u
        || openrac_rac2_follow_axis(up) == 0) {
        s_tilt.held = false;
        return;
    }
    s_follow_up = kbm::unit_or({up[0], up[1], up[2]}, kbm::kWorldUp);
    const std::int32_t locks = m.read<std::int32_t>(data + kFollowLocks);
    const float range = openrac_rac2_follow_tilt_range();
    s_follow_locks.turn = (locks & 1) != 0;
    s_follow_locks.tilt = (locks & 2) != 0 || !(range > 0.1f && range < 1.5f);
    s_follow_range = s_follow_locks.tilt ? 0.0f : range;

    std::array<kbm::Vec3, 2> offsets =
        {read_vec(data + kFollowWanted), read_vec(data + kFollowEye)};
    const bool moved = kbm::turn_orbit(
        offsets.data(), 2, s_follow_up, range, s_follow_locks, dx, dy, s_set.look, s_tilt
    );
    if (moved) {
        write_vec(data + kFollowWanted, offsets[0]);
        write_vec(data + kFollowEye, offsets[1]);
    }
    s_tick_yaw += s_follow_locks.turn ? 0.0f : kbm::DirectLook::yaw(dx, dy, s_set.look);

    // The held tilt is the game's target: the stick state at it, at rest, and the value it tilts by.
    if (s_tilt.held) {
        const float w = kbm::follow_target_of(s_tilt.elevation, range);
        m.write<float>(data + kFollowTiltTarget, w);
        m.write<float>(data + kFollowTiltSpeed, 0.0f);
        m.write<float>(tilt, kbm::follow_stick_for(w, false));
    }
}

// The first-person camera, just after it turned by the stick (follow_mouse.h): its view turned by
// the mouse's motion about the hero's up and tilted about its side, within the game's tilt limit,
// its tilt kept with it; then its side and up rows built again from the view, as the game does.
// The game's locks stop the mouse as they stop the stick.
void aligned_mouse(
    std::uint32_t camera, std::uint32_t state, const float up[3], int turn_locked, int tilt_locked
) {
    s_aligned_tick = s_tick;
    const float dx = s_aim_dx;
    const float dy = s_aim_dy;
    s_aim_dx = 0.0f;
    s_aim_dy = 0.0f;
    s_aligned_up = kbm::unit_or({up[0], up[1], up[2]}, kbm::kWorldUp);
    s_aligned_locks.turn = turn_locked != 0;
    s_aligned_locks.tilt = tilt_locked != 0;
    if (!s_aims || (dx == 0.0f && dy == 0.0f) || camera < 0x00100000u || camera >= kRamBytes - 0x40u
        || state < 0x00100000u || state >= kRamBytes - 0x20u) {
        return;
    }
    openrac::Memory m = s_mod->memory();
    const kbm::Vec3 u = s_aligned_up;
    kbm::Vec3 view = kbm::unit_or(read_vec(camera), {1.0f, 0.0f, 0.0f});
    if (dx != 0.0f && !s_aligned_locks.turn) {
        const float yaw = kbm::DirectLook::yaw(dx, dy, s_set.look);
        view = kbm::rotate(view, u, yaw);
        s_tick_yaw += yaw;
    }
    const float tilt = m.read<float>(state + kAlignedTilt);
    if (dy != 0.0f && !s_aligned_locks.tilt && std::isfinite(tilt)) {
        // Looking down lowers the view: a turn about its side (view x up) looks up.
        const float now = kbm::clamp(
            tilt - kbm::DirectLook::pitch(dx, dy, s_set.look), -kAlignedTiltLimit, kAlignedTiltLimit
        );
        const kbm::Vec3 side = kbm::unit_or(kbm::cross(view, u), {0.0f, -1.0f, 0.0f});
        view = kbm::rotate(view, side, now - tilt);
        m.write<float>(state + kAlignedTilt, now);
    }

    // The rows as the game builds them (cam_aligned_update_from_input): its side is up x view, the
    // way its FastVecCross takes its operands, and its up view x side. The hero's aim copies them
    // (his block +0x730): the side the other way round put his gun on his other side, and its sound
    // with it, each tick the mouse turned the view.
    view = kbm::unit_or(view, {1.0f, 0.0f, 0.0f});
    const kbm::Vec3 side = kbm::unit_or(kbm::cross(u, view), {0.0f, 1.0f, 0.0f});
    write_vec(camera, view);
    write_vec(camera + 0x10u, side);
    write_vec(camera + 0x20u, kbm::cross(view, side));
}

// The turn the next tick makes for mouse motion it has not read yet (frontend/mouse_turn.h), while
// the mouse turns the follow camera or the first-person camera itself, with Low-latency mouse
// camera on (else the frames are the interpolated camera).
bool picture_turn(float dx, float dy, openrac::frontend::MouseTurn* out) {
    const float tick_yaw = s_tick_yaw;
    s_tick_yaw = 0.0f;
    if (s_mod == nullptr || !s_set.low_latency || (!s_follows && !s_aims) || s_pad_look) {
        return false;
    }
    kbm::add_pending_gyro(s_gyro_on, s_mouse_counts, s_set, dx, dy);
    if (s_aims) {
        kbm::direct_turn(
            dx, dy, s_set.look, s_aligned_up, s_aligned_locks, kAlignedTiltLimit, true, out
        );
        out->tick_yaw = tick_yaw;
        return true;
    }
    const float limit = s_follow_range > 0.0f ? kbm::follow_max_elevation(s_follow_range) : 0.0f;
    kbm::direct_turn(dx, dy, s_set.look, s_follow_up, s_follow_locks, limit, false, out);
    out->tick_yaw = tick_yaw;
    return true;
}

void on_pad(int port, std::uint8_t* report, int size) {
    kbm::on_pad(
        *s_mod, port, report, size, s_tick, s_pad, [](const openrac_input* in, kbm::PadState pad) {
            step(in, pad.buttons, pad.look);
        }
    );
}

// The game's ticks a second: the PAL disc's 50, the others' 60.
float ticks_per_second(const openrac::Mod& mod) {
    return mod.version() == "pal" ? 50.0f : 60.0f;
}

void read_settings(openrac::Mod& mod) {
    s_set = kbm::read_settings(mod, ticks_per_second(mod));
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") {
        throw openrac::Error("this is Going Commando's keyboard and mouse");
    }
    if (!mod.has_input()) {
        throw openrac::Error("needs OpenRAC with mod interface 1.3 (the keyboard and mouse)");
    }
    s_mod = &mod;
    s_disc = nullptr;
    for (const Disc& d : kDiscs) {
        if (mod.version() == d.version) {
            s_disc = &d;
        }
    }
    if (s_disc == nullptr) {
        throw openrac::Error("no addresses for the " + std::string(mod.version()) + " disc");
    }
    read_settings(mod);
    s_pad = kbm::Virtual{};
    s_keys = kbm::KeyStick{};
    s_look.reset();
    s_ring.reset();
    s_latch = kbm::Latch{};
    s_pulses.clear();
    s_dirs.reset();
    s_ring_open = false;
    s_tick = kNoTick;
    s_capture.reset();
    s_painter.forget();
    s_text_count = 0;
    s_click_pending = false;
    s_click_held = false;
    s_grid.reset();
    s_ring_seek.reset();
    s_dragging = false;

    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    s_follow_tick = kNoTick;
    s_tick_yaw = 0.0f;
    s_follows = false;
    s_aim_dx = 0.0f;
    s_aim_dy = 0.0f;
    s_aligned_tick = kNoTick;
    s_aims = false;
    s_aligned_locks = {};
    s_gyro_on = false;
    s_mouse_counts = false;
    s_tilt = {};

    mod.take_input(OPENRAC_INPUT_KEYBOARD | OPENRAC_INPUT_MOUSE);
    openrac_rac2_set_follow_mouse([](std::uint32_t camera, std::uint32_t turn, std::uint32_t tilt) {
        follow_mouse(camera, turn, tilt);
    });
    openrac_rac2_set_aligned_mouse(
        [](std::uint32_t camera, std::uint32_t state, const float up[3], int turn, int tilt) {
            aligned_mouse(camera, state, up, turn, tilt);
        }
    );
    openrac::frontend::set_mouse_turn(picture_turn);
    mod.on_settings([] { read_settings(*s_mod); });
    mod.on_pad([](int port, std::uint8_t* report, int size) { on_pad(port, report, size); });
    mod.info("keyboard and mouse on the " + std::string(mod.version()) + " disc");
}

}  // namespace

namespace openrac::keyboard_mouse {

void query(openrac_mod_info* info) {
    info->api_major = OPENRAC_MOD_API_MAJOR;
    info->api_minor = OPENRAC_MOD_API_MINOR;
    info->id = "keyboard-mouse";
    info->version = "2.2.0";
}

int load(const openrac_api* api, openrac_mod* mod) {
    return openrac::detail::load(api, mod, ::load);
}

}  // namespace openrac::keyboard_mouse
