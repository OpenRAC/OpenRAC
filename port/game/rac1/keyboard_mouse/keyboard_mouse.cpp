// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Keyboard and mouse for Ratchet & Clank, built into the port (keyboard_mouse.h, docs/port/
// KEYBOARD_MOUSE.md): the keys and the mouse, as the player bound them, pressed into the first
// pad's report each tick, on top of a gamepad's. It loads through the mod interface, as a mod with
// no library or folder. What a key presses follows where the game is (kbm::Context): in play the
// console's layout; in the title, the menus and the dialogs confirm and back; in a scene, a movie
// or the ship, a skip. In play the mouse turns the camera itself (kbm::DirectLook: the follow
// camera through its hook point, the first-person view before its update), and while the Quick
// Select is held open picks its slot by the way it moves; in the menus the cursor is free, points
// at the game's own menu items, clicks, and its wheel scrolls. Takes the keyboard from the port
// (its own WASD table stops) and the cursor while in play.
//
// What every game does alike (the settings, binds, sticks, cursor, pad report and the follow
// camera's orbit turn) is game/common/keyboard_mouse's (core.hpp, kbm.hpp); this file adds Ratchet &
// Clank's own: where the game is, its menus under the mouse (menus.hpp) and where its cameras keep
// what the mouse turns. The settings are the shared openrac-mod.json's, kept as kbm_<key> lines in
// the version's settings file and read again whenever they change. While the player is on the
// keyboard and mouse the game's button icons show the keys (key_prompts.h).

#include "keyboard_mouse.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

#include "discs.h"
#include "game/common/keyboard_mouse/core.hpp"
#include "game/rac1/host/prompt_spots.h"
#include "game/rac1/keyboard_mouse/menus.hpp"
#include "openrac/games/rac1.hpp"
#include "openrac/guest.h"
#include "openrac/mod.hpp"

namespace {

using openrac::gaddr;
namespace rac1 = openrac::rac1;
using kbm::Action;
using kbm::Context;

// ---- Constants ----
//
// The game's addresses and structure offsets are in discs.h (KBM_*, k_kbm_discs). What is here is
// what this file adds.

// The start of the console's memory a game pointer may point at (assumed).
constexpr gaddr kGameMemoryStart = 0x00100000u;

// The end of the console's 32 MB (assumed).
constexpr gaddr kGameMemoryEnd = 0x02000000u;

// In the menus' page record, the page shown is at +4 (assumed: discs.h states it).
constexpr gaddr kMenuPageShownOffset = 4u;

// In the pad's sticks, the right stick's y is at +4 (assumed: discs.h lists right x, y, ...). The
// follow camera reads its tilt from it.
constexpr gaddr kPadRightYOffset = 4u;

// In a hero's moby, his up vector is at +0xE0 (assumed: discs.h states it).
constexpr gaddr kMobyUpOffset = 0xE0u;

// In a moby (games/rac1 structs/moby.h): its state (u8), its rot's y and z (the turn about its
// side and its heading, floats) and its pvars (the address of its class's own data).
constexpr gaddr kMobyStateOffset = 0x20u;
constexpr gaddr kMobyRotYOffset = 0x44u;
constexpr gaddr kMobyRotZOffset = 0x48u;
constexpr gaddr kMobyVarsOffset = 0x78u;

// How many rows of a text list are read at most (assumed): a list that reaches it is no list.
constexpr int kMaxListRows = 32;

// The follow camera's locks (assumed: discs.h states it): bit 0 stops the stick turning it, bit 1
// means the game holds the tilt.
constexpr std::int32_t kFollowLockTurn = 1;
constexpr std::int32_t kFollowLockTilt = 2;

// How many ticks the follow camera's hook point may go unseen before the follow camera is taken as
// no longer in use (assumed).
constexpr std::uint32_t kFollowSeenTicks = 2u;

// A tick number that no tick has: nothing seen yet (assumed).
constexpr std::uint32_t kNoTick = 0xFFFFFFFFu;

// How many ticks the camera's swing behind Ratchet lasts (assumed).
constexpr int kResetTicks = 30;

// The elevation the swing asks of the camera, in radians: 12 degrees (assumed).
constexpr float kResetPitch = 0.20943952f;

// How many bytes of game stack the swing's heading takes: four floats.
constexpr int kHeadingFrameBytes = 16;

// How many d-pad presses toward the cell under the mouse are queued at once: one of the pulse
// queue's four places is left for a click's Cross.
constexpr int kGridMaxQueuedSteps = 3;

// How many ticks a click waits for the focus to move onto the item before it confirms (assumed).
constexpr int kClickWaitTicks = 4;

// The game's ticks a second, to turn the ramp setting into ticks: 60 on every disc, the PAL one
// being kept in its own 60 Hz mode (differences.json, pal-60-hz).
constexpr float kTicksPerSecond = 60.0f;

// ---- Types ----

// The tick's input.
using Tick = kbm::TickInput;

// ---- State ----
//
// Everything below is touched by the game's thread only: the pad callback and the follow camera's
// hook point both run on it, in the order of a tick. load() sets it up before either runs.

// The mod and the disc's addresses, set by load().
openrac::Mod* s_mod = nullptr;
const kbm_disc* s_disc = nullptr;

// The player's settings, read by load() and again when they change (on_settings).
kbm::Settings s_set;
// The game's button icons as the player's keys while the keyboard and mouse are in use.
openrac::key_prompts::Painter s_painter;
// Whether the lock-strafe mod is loaded: the strafe key presses L2 only then. Asked at the first
// tick, once every mod has loaded (-1 before).
int s_strafe_mod = -1;

// What the keys and the mouse hold now: the pad they make, the left stick's keys, the right stick's
// mouse, the Quick Select's pointer, the actions blocked after the game changed where it is, the
// menus' queue of presses, the menus' mouse steps, and each menu direction's key repeat.
kbm::Virtual s_pad;
kbm::KeyStick s_keys;
kbm::MouseLook s_look;
kbm::RingPointer s_ring;
kbm::Latch s_latch;
kbm::Pulser s_pulses;
kbm::Stepper s_stepper;
kbm::MenuDirections s_dirs;

// Whether the Quick Select was held open at the last tick.
bool s_ring_open = false;

// The port's number of the tick last stepped, so that a tick steps once however often the game
// reads the pad.
std::uint32_t s_tick = kNoTick;

// The mouse's turn of a turret the hero sits in: the tick's motion, for the turret's update to
// apply (aim_turret); the ticks in play so far (a pause does not count, so the hero is still
// seated after it); and the tick in play a manned turret last took it.
float s_turret_dx = 0.0f;
float s_turret_dy = 0.0f;
std::uint32_t s_play_ticks = 0;
std::uint32_t s_turret_tick = kNoTick;

// The cursor captured in play, free everywhere else.
kbm::Capture s_capture;

// The camera's swing behind Ratchet: how many ticks it has left.
int s_reset_ticks = 0;

// The mouse's turn of the follow camera: the tick's motion, for its hook point to apply; the tick
// the hook point last ran (the follow camera is the one in use); whether the mouse holds the tilt,
// and where (the eye's elevation).
float s_follow_dx = 0.0f;
float s_follow_dy = 0.0f;
std::uint32_t s_follow_tick = kNoTick;
kbm::OrbitTilt s_tilt;

// Whether the gamepad's right stick is off centre: the camera is the pad's.
bool s_pad_look = false;

// What the frames between ticks need to show the mouse's turn before the next tick makes it
// (picture_turn): the camera the mouse turned itself at the last tick, the axis it turned about,
// and the follow camera's locks then.
enum class Looks {
    None,
    Follow,
    FirstPerson
};
Looks s_looks = Looks::None;
// The gyro aimed at the last tick (core.hpp's gyro_look): the frames between ticks follow it too.
bool s_gyro_on = false;
// The mouse counted at the last tick (the cursor captured in play).
bool s_mouse_counts = false;
kbm::Vec3 s_looks_up = kbm::kWorldUp;
std::int32_t s_follow_locks = 0;
// The mouse's turn the ticks since the last frame made (frontend/mouse_turn.h's tick_yaw).
float s_tick_yaw = 0.0f;

// A click on an item not yet focused: the ticks until Cross, once the focus has moved, and the
// buttons it presses.
int s_click_wait = 0;

// The vendor's strip row a click asked for (-1 none) and the steps left to get there; whether a
// click that pressed a button of its own is still down (its confirm then presses nothing more).
int s_vendor_target = -1;
int s_vendor_tries = 0;
bool s_click_held = false;

// Whether the map is focused this tick (the wheel zooms it then).
bool s_map_shown = false;
std::uint16_t s_click_button = 0;

// The game's own functions the camera's reset calls, found by load().
float (*s_fast_cos)(float) = nullptr;
float (*s_fast_sin)(float) = nullptr;
void (*s_look_along)(gaddr, float, float) = nullptr;
// The updates of the menu widgets the d-pad moves the focus between (discs.h): an item grid's and a
// text list's, as the port's functions; null where this disc's build has none.
void* s_grid_update = nullptr;
void* s_list_update = nullptr;
// The map widget's handler (DrawMapScreen) as the port's function; null where the build has none.
void* s_map_screen = nullptr;

// ---- Game memory ----

// A reference to a value in the game's memory.
template <class T>
T& ref(gaddr address) {
    return s_mod->memory().ref<T>(address);
}

// A value read from the game's memory.
template <class T>
T read(gaddr address) {
    return s_mod->memory().read<T>(address);
}

// The address of the hero, on the disc in use.
gaddr hero() {
    return s_disc->hero;
}

// A game address worth reading: in the console's 32 MB or the port's statics (the menu rows the
// port adds, such as Exit and the mods' rows, live there), aligned.
bool plausible(gaddr address) {
    const bool console = address >= kGameMemoryStart && address < kGameMemoryEnd;
    const bool port = address >= GUEST_STATICS && address - GUEST_STATICS < GUEST_STATICS_SIZE;
    return (console || port) && (address & 3u) == 0;
}

// Three floats from the game's memory as a vector.
kbm::Vec3 read_vec(gaddr address) {
    return {read<float>(address), read<float>(address + 4u), read<float>(address + 8u)};
}

// A vector as three floats in the game's memory.
void write_vec(gaddr address, kbm::Vec3 v) {
    s_mod->memory().write<float>(address, v.x);
    s_mod->memory().write<float>(address + 4u, v.y);
    s_mod->memory().write<float>(address + 8u, v.z);
}

// The camera in use, 0 for none; its id in *id.
gaddr current_camera(int* id) {
    *id = -1;

    // Between levels there is no camera block to read.
    if (s_mod->loaded_level() < 0) {
        return 0;
    }

    const gaddr cameras = s_mod->relocate_level(0, s_disc->cameras);
    const gaddr camera = read<std::uint32_t>(cameras + KBM_CAMERAS_CURRENT);

    // A pointer that is not in the game's memory means there is no camera.
    if (!plausible(camera)) {
        return 0;
    }

    *id = read<std::int16_t>(camera + KBM_CAMERA_ID);
    return camera;
}

// Where the game is: a level's mode word (its program's copy) with a level loaded, else the boot
// program's (the title and the main menu). The freeze dialog's kind goes in *dialog_kind, -1 when
// the game is in no dialog.
Context context_now(int* dialog_kind) {
    const int level = s_mod->loaded_level();
    const int level_mode =
        level >= 0 ? read<std::int32_t>(s_mod->relocate_level(0, s_disc->level_mode)) : -1;
    const int boot_mode = level < 0 ? read<std::int32_t>(s_disc->boot_mode) : -1;
    const Context c = kbm::context_of(level, level_mode, boot_mode);
    *dialog_kind = c == Context::Dialog ? read<std::int32_t>(s_mod->relocate(s_disc->dialog)) : -1;
    return c;
}

// ---- Weapons ----

// The item a Quick Select slot holds, put in hand as the Quick Select does (the hero takes it on
// his next update); nothing for an empty slot or the item already in hand.
void take_item(std::int32_t item) {
    // An empty slot, or the item already in hand, needs no change.
    if (item != 0 && item != ref<std::int32_t>(hero() + KBM_HERO_ITEM)) {
        ref<std::int32_t>(hero() + KBM_HERO_PENDING) = item;
    }
}

// The item ids in the Quick Select's slots, 0 for an empty one.
std::array<std::int32_t, kbm::kQuickSelectSlots> quick_select_slots() {
    std::array<std::int32_t, kbm::kQuickSelectSlots> slots{};

    // Read each of the slots in turn.
    for (std::size_t i = 0; i < slots.size(); ++i) {
        slots[i] = ref<std::int32_t>(s_disc->quick_select + static_cast<gaddr>(4 * i));
    }

    return slots;
}

// The number keys and the wheel: the item in a slot, or the next one round, taken at once. Only on
// foot in play, and not while the Quick Select is open (it picks the slot itself).
void weapons(const Tick& t) {
    // Weapons change only in play, on foot, and with the Quick Select shut.
    if (t.where != Context::Play || ref<std::uint8_t>(hero() + KBM_HERO_VEHICLE) != 0
        || s_ring_open) {
        return;
    }

    const auto slots = quick_select_slots();

    // Check each number key.
    for (int n = 0; n < kbm::kQuickSelectSlots; ++n) {
        // A number key went down: its slot's item, and nothing else this tick.
        if (kbm::pressed_now(
                s_set.binds, t, static_cast<Action>(static_cast<int>(Action::Weapon1) + n)
            )) {
            take_item(slots[static_cast<std::size_t>(n)]);
            return;
        }
    }

    const int step = kbm::pressed_now(s_set.binds, t, Action::WeaponNext)   ? 1
                     : kbm::pressed_now(s_set.binds, t, Action::WeaponPrev) ? -1
                                                                            : 0;

    // The wheel turned one way or the other.
    if (step != 0) {
        // From the item asked for and not yet in hand, so turning the wheel twice goes two on.
        const std::int32_t pending = ref<std::int32_t>(hero() + KBM_HERO_PENDING);
        const std::int32_t current =
            pending != 0 ? pending : ref<std::int32_t>(hero() + KBM_HERO_ITEM);
        take_item(kbm::cycle_item(slots, current, step));
    }
}

// ---- The mouse over the menus ----

// The menus' page shown now, 0 for none (a page changing, or no menu).
gaddr menu_page() {
    const gaddr page =
        read<std::uint32_t>(s_mod->relocate(s_disc->page_menu) + kMenuPageShownOffset);
    return plausible(page) ? page : 0;
}

// A text list's rows: how many, and the selected one; false for a widget that is no text list.
bool text_list(gaddr widget, int* count, int* selected) {
    const gaddr items = read<std::uint32_t>(widget + KBM_LIST_ITEMS);

    // A pointer that is not in the game's memory means there is no list.
    if (!plausible(items)) {
        return false;
    }

    int n = 0;

    // Count the rows: each item's text is nonzero, and the list ends on a zero.
    while (n < kMaxListRows
           && read<std::int16_t>(items + KBM_LIST_ITEM_SIZE * static_cast<gaddr>(n)) != 0) {
        ++n;
    }

    const std::int32_t sel = read<std::int32_t>(widget + KBM_LIST_SELECTED);

    // No rows, no end to them, or a selection outside them: this is not a text list.
    if (n == 0 || n == kMaxListRows || sel < 0 || sel >= n) {
        return false;
    }

    *count = n;
    *selected = sel;
    return true;
}

// A widget's links to the widgets the d-pad moves the focus to from it, into out (kbm::
// reachable_widgets): an item grid's four, a text list's two, none for a widget of another kind.
// The kind is its update's function (+0), in whichever program the menus are loaded from.
int focus_links(gaddr widget, std::uint32_t* out) {
    const gaddr update = read<std::uint32_t>(widget);

    // A widget with no update, or one this disc's build has no functions to tell apart: no links.
    if (!plausible(update) || (s_grid_update == nullptr && s_list_update == nullptr)) {
        return 0;
    }

    void* const fn = reinterpret_cast<void*>(s_mod->function_at<void (*)()>(update));

    // An item grid: up, down, left and right.
    if (fn != nullptr && fn == s_grid_update) {
        for (int i = 0; i < KBM_GRID_LINK_COUNT; ++i) {
            out[i] = read<std::uint32_t>(widget + KBM_GRID_LINKS + 4u * static_cast<gaddr>(i));
        }

        return KBM_GRID_LINK_COUNT;
    }

    // A text list: up and down.
    if (fn != nullptr && fn == s_list_update) {
        out[0] = read<std::uint32_t>(widget + KBM_LIST_UP);
        out[1] = read<std::uint32_t>(widget + KBM_LIST_DOWN);
        return 2;
    }

    return 0;
}

// A widget's rectangle on the screen, as the game wrote it this frame.
kbm::Rect rect_of(gaddr widget) {
    kbm::Rect r;
    r.x = read<std::int32_t>(widget + KBM_WIDGET_RECT);
    r.y = read<std::int32_t>(widget + KBM_WIDGET_RECT + 4u);
    r.w = read<std::int32_t>(widget + KBM_WIDGET_RECT + 8u);
    r.h = read<std::int32_t>(widget + KBM_WIDGET_RECT + 12u);
    return r;
}

// Whether a widget is an item grid: its update is the grid's (discs.h).
bool is_grid(gaddr widget) {
    const gaddr update = read<std::uint32_t>(widget);
    return s_grid_update != nullptr && plausible(update)
           && reinterpret_cast<void*>(s_mod->function_at<void (*)()>(update)) == s_grid_update;
}

// An item grid's layout as the game draws it (kbm::GridLayout); false where it cannot be read (no
// grid, no menu moby or frame), so the mouse steps the d-pad over it instead.
bool grid_layout(gaddr widget, kbm::GridLayout* g) {
    // Only an item grid has this layout.
    if (!is_grid(widget)) {
        return false;
    }

    const gaddr moby = read<std::uint32_t>(widget + KBM_WIDGET_MOBY);
    const gaddr frame = plausible(moby) ? read<std::uint32_t>(moby + KBM_MENU_MOBY_FRAME) : 0;

    // The frame the grid is laid out in is where its menu moby points.
    if (!plausible(frame)) {
        return false;
    }

    const gaddr icon = s_mod->relocate(s_disc->grid_icon_size);
    g->rect = rect_of(widget);
    g->frame_w = read<float>(frame + KBM_FRAME_SIZE);
    g->frame_h = read<float>(frame + KBM_FRAME_SIZE + 4u);
    g->icon_w = read<float>(icon);
    g->icon_h = read<float>(icon + 4u);
    g->margin_x = read<float>(widget + KBM_GRID_MARGINS);
    g->margin_y = read<float>(widget + KBM_GRID_MARGINS + 4u);
    g->rows = read<std::int32_t>(widget + KBM_GRID_ROWS);
    g->cols = read<std::int32_t>(widget + KBM_GRID_COLS);
    g->flags = read<std::uint32_t>(widget + KBM_GRID_FLAGS);
    return kbm::grid_layout_valid(*g);
}

// The mouse over the focused item grid: the selection steps to the cell under it, the presses
// for the whole way queued at once (a row at a time, then a column). Returns the menu_mouse
// result: false for a click that must wait for the selection to get there.
bool mouse_on_grid(
    gaddr widget, const kbm::GridLayout& g, bool moved, bool click, float px, float py
) {
    const int cell = kbm::grid_cell_at(g, px, py);
    int at = read<std::int32_t>(widget + KBM_GRID_SELECTED);
    s_stepper.reset();

    // Between the cells, or already on the one under the mouse: nothing to step.
    if (cell < 0 || cell == at || at < 0 || at >= g.rows * g.cols) {
        return true;
    }

    // Steps already on their way (the selection is not where they lead yet): let them land.
    if (!s_pulses.idle() || !(moved || click)) {
        return !click;
    }

    // Queue the presses to the cell, leaving room for a click's Cross behind them.
    for (int n = 0; n < kGridMaxQueuedSteps; ++n) {
        const std::uint16_t dir = kbm::grid_step(at, cell, g.rows, g.cols);

        // There: no more to press.
        if (dir == 0) {
            break;
        }

        s_pulses.push(dir);
        at += dir == kbm::kUp ? -g.cols : dir == kbm::kDown ? g.cols : dir == kbm::kLeft ? -1 : 1;
    }

    return !click;
}

// A page's items the mouse may point at, into widgets and rects: the focused one, and those the
// d-pad could take the focus to. Any other (a label, the item preview) the game never focuses, and
// some of its widgets break when it is (the Quick Select's preview reads the focus as an item
// grid). Returns how many.
int page_items(
    gaddr page,
    std::array<gaddr, KBM_PAGE_WIDGET_COUNT>& widgets,
    std::array<kbm::Rect, KBM_PAGE_WIDGET_COUNT>& rects
) {
    std::array<gaddr, KBM_PAGE_WIDGET_COUNT> present{};
    int present_count = 0;

    // Collect the page's widgets that are there (the slots are not all used).
    for (int i = 0; i < KBM_PAGE_WIDGET_COUNT; ++i) {
        const gaddr w = read<std::uint32_t>(page + KBM_PAGE_WIDGETS + 4u * static_cast<gaddr>(i));

        // Only the page's widgets that are there.
        if (plausible(w)) {
            present[static_cast<std::size_t>(present_count++)] = w;
        }
    }

    const gaddr focused = read<std::uint32_t>(page + KBM_PAGE_FOCUS);
    const std::uint32_t reachable = kbm::reachable_widgets(
        focused, present.data(), present_count, [](std::uint32_t w, std::uint32_t* out) {
            return plausible(w) ? focus_links(w, out) : 0;
        }
    );
    int count = 0;

    // Only the focused widget and those the focus can move to.
    for (int i = 0; i < present_count; ++i) {
        const gaddr w = present[static_cast<std::size_t>(i)];

        // Neither the focused widget nor one the focus can move to.
        if (w != focused && (reachable & (1u << i)) == 0) {
            continue;
        }

        widgets[static_cast<std::size_t>(count)] = w;
        rects[static_cast<std::size_t>(count)] = rect_of(w);
        ++count;
    }

    return count;
}

// The mouse over a menu page's item, at (px, py) in the game's screen: the item becomes the focused
// one, or, once focused, a text list's row or an item grid's cell under the mouse is stepped to.
// Returns the menu_mouse result where the mouse is over an item (false: the click must wait for
// the focus or the selection to move), and nothing where it is over none, or over a widget whose
// rows it cannot read, which the d-pad steps instead.
std::optional<bool> mouse_on_page(gaddr page, bool moved, bool click, float px, float py) {
    std::array<kbm::Rect, KBM_PAGE_WIDGET_COUNT> rects{};
    std::array<gaddr, KBM_PAGE_WIDGET_COUNT> widgets{};
    const int count = page_items(page, widgets, rects);
    const gaddr focused = read<std::uint32_t>(page + KBM_PAGE_FOCUS);
    const int hit = kbm::hit_widget(rects.data(), count, px, py);

    // The mouse is over no item.
    if (hit < 0) {
        return std::nullopt;
    }

    const gaddr widget = widgets[static_cast<std::size_t>(hit)];

    // The item under the mouse is not the focused one: ask the game to move the focus to it, where
    // the mouse moved or clicked and no move is already asked for.
    if (widget != focused) {
        // Where the mouse moved or clicked, and no focus move is asked for yet, ask for this one.
        if ((moved || click) && read<std::uint32_t>(page + KBM_PAGE_PENDING) == 0) {
            ref<std::uint32_t>(page + KBM_PAGE_PENDING) = widget;
        }

        return !click;
    }

    kbm::GridLayout grid;

    // The focused item grid: the selection goes to the cell under the mouse, not by its motion.
    if (grid_layout(widget, &grid)) {
        return mouse_on_grid(widget, grid, moved, click, px, py);
    }

    int rows = 0;
    int selected = 0;

    // A text list that the mouse moved over: step its selection to the row under the mouse, once
    // the last step is done.
    if (moved && text_list(widget, &rows, &selected) && rows > 1 && s_pulses.idle()) {
        const int row = kbm::list_row(
            rects[static_cast<std::size_t>(hit)],
            rows,
            read<std::uint32_t>(widget + KBM_LIST_FLAGS),
            py
        );

        // The row is on the list and not the selected one: one step of the d-pad toward it.
        if (row >= 0 && row != selected) {
            s_pulses.push(row < selected ? kbm::kUp : kbm::kDown);
        }

        s_stepper.reset();
        return true;
    }

    // At rest, or over a text list that does not step: nothing more to do. Over another widget the
    // d-pad steps below.
    if (!moved || text_list(widget, &rows, &selected)) {
        return true;
    }

    return std::nullopt;
}

bool beside_items(const Tick& t);

// The mouse in a menu: over a page's item, it is the one the game focuses (as the d-pad moves the
// focus between a page's items: the next tick's focus change plays the game's own sound); over a
// row of the focused list or a cell of the focused item grid, the d-pad steps the selection to it.
// Beside a page's items nothing moves. Elsewhere (the dialogs) its motion steps the d-pad. A click
// on an item not yet focused focuses it first. Returns false when the click must wait for the focus
// to move.
bool menu_mouse(const Tick& t, bool click) {
    const openrac_input* in = t.in;
    const bool moved = in->mouse_dx != 0.0f || in->mouse_dy != 0.0f;

    // The mouse has no say out of a menu, or when the player turned it off.
    if (!t.pointer || !s_set.menu_mouse) {
        return true;
    }

    const float px = in->pointer_x * kbm::kScreenWidth;
    const float py = in->pointer_y * kbm::kScreenHeight;
    const gaddr page = (t.where == Context::Menu) ? menu_page() : 0;

    // A menu page is shown: the mouse may point at its items.
    if (page != 0) {
        const std::optional<bool> result = mouse_on_page(page, moved, click, px, py);

        // The mouse is over an item, which has settled what happens: take its answer.
        if (result.has_value()) {
            return *result;
        }

        // Beside every item: nothing is selected, the selection stays where it is.
        if (beside_items(t)) {
            s_stepper.reset();
            return true;
        }
    }

    // Elsewhere (a dialog, or an item whose rows it cannot read) the mouse's motion steps the
    // d-pad, except on the title, which has nothing to step, and at the vendor, where the mouse
    // points (vendor_mouse).
    if (moved && t.where != Context::Title && t.where != Context::Vendor && !s_map_shown) {
        const std::uint16_t dir = s_stepper.step(in->mouse_dx, in->mouse_dy);

        // A step of the d-pad was due: press it.
        if (dir != 0) {
            s_pulses.push(dir);
        }
    }

    return true;
}

// ---- The mouse on the map ----

// A drag of the map under way, and where the mouse was at the last tick (the game's screen).
bool s_map_drag = false;
float s_map_drag_x = 0.0f;
float s_map_drag_y = 0.0f;

// The focused map: the mouse's left button held down over it drags it, the wheel zooms it (the
// game's own pan and zoom of the loaded map, kbm::map_pan and map_zoom).
void map_mouse(const Tick& t) {
    const gaddr m = s_mod->relocate(s_disc->map_state);
    const std::int32_t loaded = read<std::int32_t>(m + KBM_MAP_LOADED);

    // No map shown: nothing to move.
    if (read<std::int32_t>(m + KBM_MAP_READY) == 0 || loaded < 0 || loaded >= KBM_MAP_COUNT) {
        s_map_drag = false;
        return;
    }

    const gaddr at = static_cast<gaddr>(loaded) * 4u;
    const float zoom = read<float>(m + KBM_MAP_ZOOM + at);
    const float px = t.in->pointer_x * kbm::kScreenWidth;
    const float py = t.in->pointer_y * kbm::kScreenHeight;
    const gaddr page = menu_page();
    const gaddr focused = page != 0 ? read<std::uint32_t>(page + KBM_PAGE_FOCUS) : 0;
    const bool over = plausible(focused) && rect_of(focused).contains(px, py);

    // The button let go ends a drag; pressed over the map, one starts.
    if (!kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, false)) {
        s_map_drag = false;
    } else if (kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, true) && over) {
        s_map_drag = true;
        s_map_drag_x = px;
        s_map_drag_y = py;
    }

    // Dragged: the map follows the mouse.
    if (s_map_drag && std::isfinite(zoom) && (px != s_map_drag_x || py != s_map_drag_y)) {
        ref<std::int32_t>(m + KBM_MAP_PAN_X + at) = kbm::map_pan(
            read<std::int32_t>(m + KBM_MAP_PAN_X + at), px - s_map_drag_x, zoom, false
        );
        ref<std::int32_t>(m + KBM_MAP_PAN_Y + at) =
            kbm::map_pan(read<std::int32_t>(m + KBM_MAP_PAN_Y + at), py - s_map_drag_y, zoom, true);
        s_map_drag_x = px;
        s_map_drag_y = py;
    }

    // The wheel over the map zooms it.
    if (over && t.in->wheel_y != 0.0f && std::isfinite(zoom)) {
        ref<float>(m + KBM_MAP_ZOOM + at) = kbm::map_zoom(zoom, t.in->wheel_y);
    }
}

// ---- Clicks on the game's button icons ----

// The icons the game's text showed last (prompt_spots.h), kept while no frame drew new ones for a
// few ticks (a tick with no frame between).
std::array<kbm::PromptSpot, OPENRAC_RAC1_PROMPT_SPOTS_MAX> s_spots{};
int s_spot_count = 0;
int s_spots_age = 0;
constexpr int kSpotsKeptTicks = 3;

// Takes the icons drawn since the last tick.
void take_prompt_spots() {
    std::array<openrac_rac1_prompt_spot, OPENRAC_RAC1_PROMPT_SPOTS_MAX> drawn{};
    const int n = openrac_rac1_take_prompt_spots(drawn.data());

    // No frame drew any: the last ones stand a little longer.
    if (n == 0 && ++s_spots_age <= kSpotsKeptTicks) {
        return;
    }

    s_spots_age = 0;
    s_spot_count = n;

    // Each icon and its line, in the game's screen.
    for (int i = 0; i < n; ++i) {
        const openrac_rac1_prompt_spot& d = drawn[static_cast<std::size_t>(i)];
        kbm::PromptSpot& p = s_spots[static_cast<std::size_t>(i)];
        p.cell = d.cell;
        p.rect = kbm::Rect{d.x0, d.y0, d.x1 - d.x0, d.y1 - d.y0};
    }
}

// The button of the icon a click is on (the icon, or the text after it on its line), 0 for none
// and over a menu item, which the click is for (a row that shows an icon is chosen, not pressed).
std::uint16_t prompt_click(const Tick& t) {
    const float px = t.in->pointer_x * kbm::kScreenWidth;
    const float py = t.in->pointer_y * kbm::kScreenHeight;
    const gaddr page = t.where == Context::Menu ? menu_page() : 0;

    // Over a menu item: the item's.
    if (page != 0) {
        std::array<kbm::Rect, KBM_PAGE_WIDGET_COUNT> rects{};
        std::array<gaddr, KBM_PAGE_WIDGET_COUNT> widgets{};
        const int count = page_items(page, widgets, rects);

        // The item under the mouse has the click.
        if (kbm::hit_widget(rects.data(), count, px, py) >= 0) {
            return 0;
        }
    }

    return kbm::prompt_button_at(s_spots.data(), s_spot_count, px, py);
}

// Whether the pointer is on a menu page beside all its items: a click there confirms nothing.
bool beside_items(const Tick& t) {
    const gaddr page = t.where == Context::Menu ? menu_page() : 0;

    // No page: nothing to be beside.
    if (page == 0) {
        return false;
    }

    std::array<kbm::Rect, KBM_PAGE_WIDGET_COUNT> rects{};
    std::array<gaddr, KBM_PAGE_WIDGET_COUNT> widgets{};
    const int count = page_items(page, widgets, rects);
    const float px = t.in->pointer_x * kbm::kScreenWidth;
    const float py = t.in->pointer_y * kbm::kScreenHeight;
    return count > 0 && kbm::hit_widget(rects.data(), count, px, py) < 0;
}

// ---- The mouse at the vendor ----

// The vendor's screen, its monitors in the scene (kbm::vendor_hit): a click on the Buy monitor
// presses Cross, on the Exit monitor Triangle, on an icon of the strip the d-pad steps the
// selection to it, a step at a time as the game takes them. A click elsewhere presses nothing.
// Returns the button the click presses, 0 for none.
std::uint16_t vendor_mouse(const Tick& t, bool click) {
    const gaddr v = s_mod->relocate_level(0, s_disc->vendor);
    const bool open = read<std::int32_t>(v + KBM_VENDOR_STATE) == KBM_VENDOR_OPEN;
    const std::int32_t count = read<std::int32_t>(v + KBM_VENDOR_ROWS);
    const std::int32_t cursor = read<std::int32_t>(v + KBM_VENDOR_CURSOR);
    const bool buying = read<std::int32_t>(v + KBM_VENDOR_PURCHASE) != 0;

    // Opening, closing, or rows that make no sense: the mouse does nothing.
    if (!open || count <= 0 || count > 16 || cursor < 0 || cursor >= count) {
        s_vendor_target = -1;
        return 0;
    }

    // A purchase under way reads left and right as the amount: no steps then.
    if (buying) {
        s_vendor_target = -1;
    }

    // On the way to a clicked icon: the next step once the last one is done.
    if (s_vendor_target >= 0 && s_pulses.idle()) {
        const std::uint16_t dir = kbm::vendor_step(cursor, s_vendor_target, count);

        // There, or out of tries (the game takes no steps while the strip slides too far).
        if (dir == 0 || s_vendor_tries-- <= 0) {
            s_vendor_target = -1;
        } else {
            s_pulses.push(dir);
        }
    }

    // Only a click does more.
    if (!click) {
        return 0;
    }

    const kbm::VendorHit hit = kbm::vendor_hit(
        t.in->pointer_x, t.in->pointer_y, count, cursor, read<std::int32_t>(v + KBM_VENDOR_SCROLL)
    );

    // The monitor clicked: its button, or the icon's row to step to.
    switch (hit.spot) {
        case kbm::VendorSpot::Buy:
            return kbm::kCross;
        case kbm::VendorSpot::Exit:
            return kbm::kTriangle;
        case kbm::VendorSpot::Item:
            // Not while a purchase is under way, and not the selected icon.
            if (!buying && hit.steps != 0) {
                s_vendor_target = ((cursor + hit.steps) % count + count) % count;
                s_vendor_tries = 2 * count + 8;
            }
            return 0;
        case kbm::VendorSpot::None:
            break;
    }

    return 0;
}

// ---- The camera from the mouse ----

// The first-person view turned by the tick's motion, before the game's update of it (which turns
// it by the sticks, 0 from the mouse, and squares its rows up again): the way it looks turned
// about Ratchet's up, and tilted, its tilt kept where the game keeps it.
void first_person_look(gaddr camera, float dx, float dy) {
    const gaddr state = read<std::uint32_t>(camera + KBM_CAMERA_STATE);
    const gaddr moby = read<std::uint32_t>(hero() + KBM_HERO_MOBY);
    const kbm::Vec3 up = plausible(moby)
                             ? kbm::unit_or(read_vec(moby + kMobyUpOffset), kbm::kWorldUp)
                             : kbm::kWorldUp;
    // The frames between ticks turn the view about the same axis (picture_turn).
    s_looks_up = up;

    // The view has no state to tilt, or the mouse did not move: nothing to turn.
    if (!plausible(state) || (dx == 0.0f && dy == 0.0f)) {
        return;
    }

    kbm::Vec3 look = read_vec(camera);
    const float yaw = kbm::DirectLook::yaw(dx, dy, s_set.look);
    look = kbm::rotate(look, up, yaw);
    s_tick_yaw += yaw;
    const float tilt = read<float>(state + KBM_FIRST_PERSON_TILT);

    // The game's tilt is a number: turn the view by how much the tilt changes, within the limit.
    if (std::isfinite(tilt)) {
        const float wanted = tilt + kbm::DirectLook::pitch(dx, dy, s_set.look);
        const float limit = kbm::kFirstPersonTiltLimit;
        const float now = kbm::clamp(wanted, -limit, limit);
        const float moved = kbm::clamp(tilt, -limit, limit);
        // Looking down lowers the view's elevation.
        look = kbm::with_elevation(look, up, kbm::elevation(look, up) - (now - moved));
        s_mod->memory().write<float>(state + KBM_FIRST_PERSON_TILT, now);
    }

    write_vec(camera, look);
}

// The follow camera's tilt target where the mouse holds the eye (s_tilt): its eased target there
// and at rest, and the stick's y it reads this tick asking for the same.
void hold_tilt(gaddr state) {
    const float w = kbm::follow_target_of(s_tilt.elevation);
    s_mod->memory().write<float>(state + KBM_FOLLOW_TILT, w);
    s_mod->memory().write<float>(state + KBM_FOLLOW_TILT_RATE, 0.0f);
    const bool inverts = read<std::int32_t>(s_mod->relocate(s_disc->tilt_invert)) != 0;
    const gaddr stick_y = s_mod->relocate(s_disc->pad_sticks) + kPadRightYOffset;
    s_mod->memory().write<float>(stick_y, kbm::follow_stick_for(w, inverts));
}

// The follow camera's orbit, its smoothed copy and the eye turned by the mouse's motion (dx, dy)
// about up, and tilted, as far as the game's locks allow (kbm::turn_orbit); the tilt then held
// where the mouse leaves it.
void turn_follow_camera(gaddr state, float dx, float dy, std::int32_t locks) {
    const gaddr cameras = s_mod->relocate_level(0, s_disc->cameras);
    const kbm::Vec3 up = kbm::unit_or(read_vec(cameras + KBM_CAMERAS_UP), kbm::kWorldUp);
    const gaddr orbit = state + KBM_FOLLOW_ORBIT;
    const gaddr smooth = state + KBM_FOLLOW_ORBIT_SMOOTH;
    const kbm::Vec3 target = read_vec(state + KBM_FOLLOW_TARGET);
    std::array<kbm::Vec3, 3> v =
        {read_vec(orbit), read_vec(smooth), kbm::minus(read_vec(state + KBM_FOLLOW_EYE), target)};
    const kbm::OrbitLocks lock{(locks & kFollowLockTurn) != 0, (locks & kFollowLockTilt) != 0};
    kbm::turn_orbit(v.data(), 3, up, kbm::kFollowTiltRange, lock, dx, dy, s_set.look, s_tilt);
    s_tick_yaw += lock.turn ? 0.0f : kbm::DirectLook::yaw(dx, dy, s_set.look);

    write_vec(orbit, v[0]);
    write_vec(smooth, v[1]);
    write_vec(state + KBM_FOLLOW_EYE, kbm::plus(target, v[2]));

    // The mouse holds the tilt: write the game's target for it.
    if (s_tilt.held) {
        hold_tilt(state);
    }
}

// The follow camera turned by the tick's motion, from its hook point, before the game moves it on:
// the orbit (and its smoothed copy, and the eye's last place with them, so that neither the
// smoothing nor the follow pulls it back) turned about up, then tilted. The tilt stays where the
// mouse leaves it (Camera tilt: Stays) through the game's own target: its eased target set there
// at rest and the stick's y it reads written so that it asks for the same, whatever the game's
// invert option. The game's locks (a camera volume's) stop the mouse as they stop the stick.
void follow_look(gaddr camera) {
    s_follow_tick = s_tick;
    const float dx = s_follow_dx;
    const float dy = s_follow_dy;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    const gaddr state = read<std::uint32_t>(camera + KBM_CAMERA_STATE);

    // The camera has no state, or the gamepad's stick is turning it: the mouse lets go.
    if (!plausible(state) || s_pad_look) {
        s_tilt.held = false;
        return;
    }

    const std::int32_t locks = read<std::int32_t>(state + KBM_FOLLOW_LOCKS);
    s_follow_locks = locks;
    s_looks_up = kbm::unit_or(
        read_vec(s_mod->relocate_level(0, s_disc->cameras) + KBM_CAMERAS_UP), kbm::kWorldUp
    );

    // The mouse is at rest: only keep the tilt it holds, if it still may.
    if (dx == 0.0f && dy == 0.0f) {
        // The tilt is not kept (Levels out), or the game holds it: the mouse lets go.
        if (!s_set.look.pitch_holds || (locks & kFollowLockTilt) != 0) {
            s_tilt.held = false;
        }

        // Keep holding the tilt where the mouse left it.
        if (s_tilt.held) {
            hold_tilt(state);
        }

        return;
    }

    turn_follow_camera(state, dx, dy, locks);
}

// ---- The tick ----

// Whether a widget's handler is DrawMapScreen, in whichever program's copy (a level's own copy is
// at another address than the executable's).
bool is_map_screen(gaddr handler) {
    return s_map_screen != nullptr && plausible(handler)
           && reinterpret_cast<void*>(s_mod->function_at<void (*)()>(handler)) == s_map_screen;
}

// Whether the map is focused in the menu: its handler (the widget's update or draw) is
// DrawMapScreen.
bool map_focused(const Tick& t) {
    // The map is a page of the menus only.
    if (t.where != Context::Menu) {
        return false;
    }

    const gaddr page = menu_page();
    const gaddr focused = page != 0 ? read<std::uint32_t>(page + KBM_PAGE_FOCUS) : 0;
    return plausible(focused)
           && (is_map_screen(read<std::uint32_t>(focused))
               || is_map_screen(read<std::uint32_t>(focused + 4u)));
}

// The camera, in play and out of the ring: the follow camera and the first-person view turned by
// the mouse (and the gyro) itself; any other camera (a scene's own) by them as the right stick.
void mouse_camera(const Tick& t, bool play, bool ring, std::uint16_t pad_buttons, kbm::Virtual& v) {
    s_looks = Looks::None;
    int id = -1;
    const gaddr camera = play ? current_camera(&id) : 0;

    // The gyro aims (core.hpp): "only while aiming" is the first-person view, or L1 or L2 held
    // (first person, the lock-strafe mod's strafe).
    const bool aiming = (camera != 0 && id == KBM_CAMERA_FIRST_PERSON)
                        || ((v.held | pad_buttons) & (kbm::kL1 | kbm::kL2)) != 0;
    const kbm::GyroLook gyro = ring ? kbm::GyroLook{} : kbm::gyro_look(t, s_set, aiming);
    s_gyro_on = gyro.on;
    s_mouse_counts = t.play;

    // In play with the cursor captured (or the gyro aiming) and the Quick Select shut, the mouse
    // and the gyro turn the camera.
    if ((t.play || gyro.on) && !ring) {
        const float dx = (t.play ? t.in->mouse_dx : 0.0f) + gyro.dx;
        const float dy = (t.play ? t.in->mouse_dy : 0.0f) + gyro.dy;
        const bool follow =
            id == 0 || (s_follow_tick != kNoTick && s_tick - s_follow_tick <= kFollowSeenTicks);

        const bool turret =
            s_turret_tick != kNoTick && s_play_ticks - s_turret_tick <= kFollowSeenTicks;

        // In a turret the mouse aims the turret (aim_turret), not the camera; else which camera is
        // in use decides how the mouse turns it.
        if (turret) {
            s_turret_dx = dx;
            s_turret_dy = dy;
            s_look.reset();
        } else if (camera != 0 && id == KBM_CAMERA_FIRST_PERSON) {
            first_person_look(camera, dx, dy);
            s_look.reset();
            s_looks = Looks::FirstPerson;
        } else if (camera != 0 && follow) {
            s_looks = Looks::Follow;
            s_follow_dx = dx;
            s_follow_dy = dy;
            s_look.reset();
        } else {
            const bool first_person = ((v.held | pad_buttons) & kbm::kL1) != 0;
            s_look.step(dx, dy, first_person, s_set.look);
            v.right_x = s_look.x;
            v.right_y = s_look.y;
        }

        // The player turns the camera sideways: the swing behind Ratchet stops.
        if (s_reset_ticks > 0 && dx != 0.0f) {
            s_reset_ticks = 0;
        }
    } else if (!play) {
        // Out of play the mouse's look rests, and the tilt it held is let go.
        s_look.x = 0.0f;
        s_look.y = 0.0f;
        s_tilt.held = false;
    }
}

// The tick's pad from the keyboard and mouse: once a tick, when the port has read them anew.
void step(const openrac_input* in, std::uint16_t pad_buttons, bool pad_look) {
    Tick t;
    t.in = in;
    int dialog_kind = -1;
    t.where = context_now(&dialog_kind);
    const bool play = t.where == Context::Play;
    kbm::where_mouse_counts(t);
    s_pad_look = pad_look;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    s_turret_dx = 0.0f;
    s_turret_dy = 0.0f;
    s_play_ticks += play ? 1u : 0u;

    s_capture.update(*s_mod, s_set.capture, play, in);
    take_prompt_spots();

    kbm::ActionSet held = kbm::actions_held(s_set.binds, t);
    const kbm::Context before = s_latch.last;
    held = s_latch.filter(t.where, held);

    // The game changed where it is: what was queued or half done in the old place is dropped.
    if (before != t.where) {
        s_pulses.clear();
        s_stepper.reset();
        s_click_wait = 0;
        s_vendor_target = -1;
        s_click_held = false;
    }

    const bool map = map_focused(t);
    s_map_shown = map && t.pointer && s_set.menu_mouse;
    kbm::Virtual v;
    // A click on a menu item not yet focused waits for the focus; then it confirms.
    const bool click = t.pointer && (held & kbm::bit(Action::MenuConfirm)) != 0
                       && kbm::pressed_now(s_set.binds, t, Action::MenuConfirm);

    // A click of the mouse (not a key) on one of the game's button icons, or the text after it,
    // presses that button instead (prompt_click); at the vendor, a click presses the button of the
    // monitor it is on, and nothing elsewhere. The click's confirm then stays off while the
    // mouse's button is down.
    const bool mouse_click = click && kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, true);
    std::uint16_t click_press = 0;
    bool click_taken = false;

    // The vendor's monitors.
    if (t.where == Context::Vendor && t.pointer && s_set.menu_mouse) {
        click_press = vendor_mouse(t, mouse_click);
        click_taken = mouse_click;
    } else if (!play) {
        click_press = mouse_click && s_set.menu_mouse ? prompt_click(t) : 0;
        // On the map a click drags it (map_mouse): it presses only an icon's button.
        click_taken = click_press != 0 || (map && mouse_click && s_set.menu_mouse)
                      || (mouse_click && s_set.menu_mouse && beside_items(t));

        // The focused map moves with the mouse.
        if (map && t.pointer && s_set.menu_mouse) {
            map_mouse(t);
        }

        // Out of play the mouse points at the menus; a click on an item not yet focused is held
        // back, and pressed again once the focus has moved.
        if (!click_taken && !menu_mouse(t, click)) {
            held &= ~kbm::bit(Action::MenuConfirm);
            s_click_wait = kClickWaitTicks;
            s_click_button =
                kbm::buttons_for(t.where, kbm::bit(Action::MenuConfirm), dialog_kind, map);
        }
    }

    s_click_held = click_taken || s_click_held;

    // The click's button is still down: it is not the game's Cross.
    if (s_click_held && kbm::mouse_bind(s_set.binds, t, Action::MenuConfirm, false)) {
        held &= ~kbm::bit(Action::MenuConfirm);
    } else {
        s_click_held = false;
    }

    // The button the click pressed, once.
    if (click_press != 0) {
        s_pulses.push(click_press);
    }

    if (s_strafe_mod < 0) {
        s_strafe_mod = 0;
        for (const openrac::ModEntry& m : s_mod->mods()) {
            s_strafe_mod |= m.id == std::string_view("lock-strafe") ? 1 : 0;
        }
    }
    v.held = kbm::buttons_for(t.where, held, dialog_kind, map, s_strafe_mod == 1);

    // Going back meanwhile: the click that waited, and the steps queued, press nothing on the
    // page gone back to.
    if ((held & (kbm::bit(Action::MenuBack) | kbm::bit(Action::Pause))) != 0 && !play) {
        s_click_wait = 0;
        s_pulses.clear();
        s_vendor_target = -1;
    }

    // A click that waited has waited long enough: press it now.
    if (s_click_wait > 0 && --s_click_wait == 0) {
        s_pulses.push(s_click_button);
    }

    // In menus the directions are the d-pad; in play nothing is queued.
    if (!play) {
        s_dirs.step(s_set.binds, t, held, !s_map_shown, s_pulses, v);
    } else {
        s_pulses.clear();
    }

    // The Quick Select: Triangle held while its bind is, the mouse pointing the left stick at a slot.
    const bool ring = play && (held & kbm::bit(Action::QuickSelect)) != 0;

    // The Quick Select just opened: its pointer starts at the centre.
    if (ring && !s_ring_open) {
        s_ring.reset();
    }

    s_ring_open = ring;
    kbm::left_stick(t, held, ring, s_set, s_ring, s_keys, v);
    mouse_camera(t, play, ring, pad_buttons, v);

    // The camera's reset: the mouse's look starts afresh, and the camera swings behind Ratchet.
    if (play && kbm::pressed_now(s_set.binds, t, Action::CameraReset)) {
        s_look.reset();
        s_tilt.held = false;  // the swing levels it
        s_reset_ticks = kResetTicks;
    }

    weapons(t);
    s_pad = v;
    kbm::paint_prompts(
        *s_mod,
        s_painter,
        s_mod->relocate(s_disc->font_bank),
        s_mod->relocate(s_disc->font_table),
        s_set,
        kbm::prompt_actions(t.where, dialog_kind, map, s_strafe_mod == 1)
    );
}

// The first pad's report, before the game reads it: the keys and the mouse on top of the pad's.
// Runs on the game's thread, as often as the game reads the pad.
void on_pad(int port, std::uint8_t* report, int size) {
    kbm::on_pad(
        *s_mod, port, report, size, s_tick, s_pad, [](const openrac_input* in, kbm::PadState pad) {
            step(in, pad.buttons, pad.look);
        }
    );
}

// The camera looking along the way Ratchet faces, as the game's own camera volumes ask for a
// heading: the swing behind him.
void look_behind_ratchet() {
    const float facing = s_mod->memory().read<float>(hero() + KBM_HERO_FACING);
    openrac::StackFrame frame(*s_mod, kHeadingFrameBytes);
    const gaddr dir = frame.address();
    s_mod->memory().write<float>(dir, s_fast_cos(facing));
    s_mod->memory().write<float>(dir + 4u, s_fast_sin(facing));
    s_mod->memory().write<float>(dir + 8u, 0.0f);
    s_mod->memory().write<float>(dir + 12u, 0.0f);
    s_look_along(dir, kResetPitch, 0.0f);
}

// The follow camera's per-tick requests: after the game's (and any mod's before this one), the
// mouse's turn (follow_look); while the camera's reset lasts, instead, a request to look along the
// way Ratchet faces, as the game's own camera volumes ask for a heading.
template <class Original>
void camera_overrides(Original original, gaddr camera) {
    original(camera);
    int kind = 0;

    // Out of play the follow camera is not the player's: drop what the mouse held.
    if (context_now(&kind) != Context::Play) {
        s_reset_ticks = 0;
        s_tilt.held = false;
        s_follow_dx = 0.0f;
        s_follow_dy = 0.0f;
        return;
    }

    // No swing running: the mouse turns the camera.
    if (s_reset_ticks <= 0) {
        s_reset_ticks = 0;
        follow_look(camera);
        return;
    }

    s_follow_tick = s_tick;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    --s_reset_ticks;
    look_behind_ratchet();
}

// ---- Turrets ----

// A turret the hero sits in, as its update aims it with the left stick (games/rac1: Batalia's
// batalia_cannon_update, Gaspar's turret_update, Hoven's hoven_turret_update): the moby's state
// while manned; where its pitch is (in the moby, or at an offset in its pvars) and whether a larger
// pitch looks up; the range the update clamps the pitch to (min > max: the level's data holds it);
// and, where the update may skip its aiming, the offset of the aim quaternion it builds from the
// pitch each tick it aims.
struct TurretAim {
    std::uint8_t manned;
    bool pitch_in_vars;
    gaddr pitch;
    bool larger_looks_up;
    float pitch_min;
    float pitch_max;
    std::optional<gaddr> built_aim;
};

// Batalia's cannon: rot.y, negative up, clamped to -45..20 degrees.
const TurretAim kBataliaCannon{5, false, kMobyRotYOffset, false, -0.7853982f, 0.34906584f, {}};
// Gaspar's turrets: pvars +0x44, positive up, clamped to +-30 degrees.
const TurretAim kGasparTurret{5, true, 0x44u, true, -0.5235988f, 0.5235988f, {}};
// Hoven's turret: pvars +0x74, positive up, clamped to the level's data; it skips its aiming in the
// planet buster's death camera and its scenes, so the quaternion at pvars +0x10 tells whether it
// aimed.
const TurretAim kHovenTurret{4, true, 0x74u, true, 1.0f, 0.0f, 0x10u};

// An angle brought back to -pi..pi, as the game's FastAddRots keeps its headings.
float wrap_angle(float a) {
    constexpr float kTwoPi = 6.2831855f;
    return std::remainder(a, kTwoPi);
}

// A turret's update: while the hero sits in it and the mouse moved, the mouse's motion turns it
// directly (kbm::DirectLook, as the camera), before the game's update reads the stick and clamps.
// Without the mouse's motion the update runs untouched, so the pad aims as it always did.
template <class Original>
void aim_turret(Original original, gaddr moby, const TurretAim& aim) {
    // Not manned: the turret is the game's.
    if (read<std::uint8_t>(moby + kMobyStateOffset) != aim.manned) {
        original(moby);
        return;
    }

    s_turret_tick = s_play_ticks;
    const float dx = s_turret_dx;
    const float dy = s_turret_dy;
    s_turret_dx = 0.0f;
    s_turret_dy = 0.0f;
    const gaddr vars = read<std::uint32_t>(moby + kMobyVarsOffset);

    // The mouse did not move (or the turret has no data): nothing to add.
    if ((dx == 0.0f && dy == 0.0f) || !plausible(vars)) {
        original(moby);
        return;
    }

    const gaddr yaw_at = moby + kMobyRotZOffset;
    const gaddr pitch_at = (aim.pitch_in_vars ? vars : moby) + aim.pitch;
    const float old_yaw = read<float>(yaw_at);
    const float old_pitch = read<float>(pitch_at);
    // DirectLook's pitch is positive down.
    const float up = -kbm::DirectLook::pitch(dx, dy, s_set.look);
    const float yaw = wrap_angle(old_yaw + kbm::DirectLook::yaw(dx, dy, s_set.look));
    float pitch = old_pitch + (aim.larger_looks_up ? up : -up);

    // The game's own clamp, where it is known: a skipped aim still leaves the pitch in range.
    if (aim.pitch_min <= aim.pitch_max) {
        pitch = std::fmin(std::fmax(pitch, aim.pitch_min), aim.pitch_max);
    }

    s_mod->memory().write<float>(yaw_at, yaw);
    s_mod->memory().write<float>(pitch_at, pitch);

    // No aim to check: the update clamps the pitch, or it is already in range.
    if (!aim.built_aim) {
        original(moby);
        return;
    }

    // The update may skip its aiming (and its clamp): a quaternion it did not build tells, and then
    // the turn is taken back.
    constexpr std::uint32_t kUnbuilt = 0x7FC0DEADu;
    constexpr gaddr kWords = 4u;
    const gaddr quat = vars + *aim.built_aim;
    std::array<std::uint32_t, kWords> saved{};
    for (gaddr i = 0; i < kWords; ++i) {
        saved[i] = read<std::uint32_t>(quat + 4u * i);
        s_mod->memory().write<std::uint32_t>(quat + 4u * i, kUnbuilt);
    }

    original(moby);
    bool built = false;
    for (gaddr i = 0; i < kWords; ++i) {
        built = built || read<std::uint32_t>(quat + 4u * i) != kUnbuilt;
    }

    // Aimed: the update took the turn and clamped it.
    if (built) {
        return;
    }

    for (gaddr i = 0; i < kWords; ++i) {
        s_mod->memory().write<std::uint32_t>(quat + 4u * i, saved[i]);
    }

    // Taken back only where the update left the value as the mouse set it.
    if (read<float>(yaw_at) == yaw) {
        s_mod->memory().write<float>(yaw_at, old_yaw);
    }

    if (read<float>(pitch_at) == pitch) {
        s_mod->memory().write<float>(pitch_at, old_pitch);
    }
}

// ---- The frames between ticks ----

// The turn the next tick makes for mouse motion (dx, dy) it has not read yet, while the mouse
// turns the follow camera or the first-person view itself (frontend/mouse_turn.h). Not during the
// swing behind Ratchet or while the gamepad's stick has the camera; a lock of the game's stops the
// turn here as it stops the mouse. Only with Low-latency mouse camera on: else the frames are the
// interpolated camera.
bool picture_turn(float dx, float dy, openrac::frontend::MouseTurn* out) {
    const float tick_yaw = s_tick_yaw;
    s_tick_yaw = 0.0f;
    if (s_mod == nullptr || !s_set.low_latency || s_looks == Looks::None || s_pad_look
        || s_reset_ticks > 0) {
        return false;
    }

    kbm::add_pending_gyro(s_gyro_on, s_mouse_counts, s_set, dx, dy);
    const bool first_person = s_looks == Looks::FirstPerson;
    const std::int32_t locks = first_person ? 0 : s_follow_locks;
    const kbm::OrbitLocks lock{(locks & kFollowLockTurn) != 0, (locks & kFollowLockTilt) != 0};
    const float limit = first_person ? kbm::kFirstPersonTiltLimit : kbm::follow_max_elevation();
    kbm::direct_turn(dx, dy, s_set.look, s_looks_up, lock, limit, first_person, out);
    out->tick_yaw = tick_yaw;
    return true;
}

// ---- Loading ----

// One of the game's functions by the disc's name, which must be there.
template <class Fn>
Fn game_function(const char* name) {
    return s_mod->required_function<Fn>(name);
}

// The player's settings: the binds (warning of any two actions that share one) and the tunings.
void read_settings(openrac::Mod& mod) {
    s_set = kbm::read_settings(mod, kTicksPerSecond);
}

// Puts everything the keys and the mouse hold back to rest, as at the first tick.
void reset_state() {
    s_pad = kbm::Virtual{};
    s_gyro_on = false;
    s_mouse_counts = false;
    s_keys = kbm::KeyStick{};
    s_look.reset();
    s_ring.reset();
    s_latch = kbm::Latch{};
    s_pulses.clear();
    s_stepper.reset();
    s_dirs.reset();
    s_ring_open = false;
    s_tick = kNoTick;
    s_capture.reset();
    s_reset_ticks = 0;
    s_click_wait = 0;
    s_vendor_target = -1;
    s_click_held = false;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    s_follow_tick = kNoTick;
    s_turret_dx = 0.0f;
    s_turret_dy = 0.0f;
    s_play_ticks = 0;
    s_turret_tick = kNoTick;
    s_tilt = {};
    s_pad_look = false;
    s_looks = Looks::None;
    s_looks_up = kbm::kWorldUp;
    s_follow_locks = 0;
    s_tick_yaw = 0.0f;
    s_painter.forget();
}

// The mod's entry: finds the disc's addresses, reads the settings, takes the keyboard and the mouse
// from the port, and hooks the pad and the follow camera. Throws when the game, the port or the
// disc is not one it works with.
void load(openrac::Mod& mod) {
    // This mod is for Ratchet & Clank only.
    if (mod.game() != "rac1") {
        throw openrac::Error("this mod is for Ratchet & Clank");
    }

    // The port must offer the keyboard and mouse (interface 1.3).
    if (!mod.has_input()) {
        throw openrac::Error("needs OpenRAC with mod interface 1.3 (the keyboard and mouse)");
    }

    s_mod = &mod;
    s_disc = nullptr;
    s_strafe_mod = -1;

    // Take the disc whose addresses match the game's version.
    for (const kbm_disc& d : k_kbm_discs) {
        // The disc's addresses whose version this is.
        if (mod.version() == d.version) {
            s_disc = &d;
        }
    }

    // No disc matched: the mod has no addresses to work with.
    if (s_disc == nullptr) {
        throw openrac::Error("no addresses for the " + std::string(mod.version()) + " disc");
    }

    read_settings(mod);

    s_fast_cos = game_function<float (*)(float)>(s_disc->fast_cos);
    s_fast_sin = game_function<float (*)(float)>(s_disc->fast_sin);
    s_look_along = game_function<void (*)(gaddr, float, float)>(s_disc->camera_look_along);
    // Without these the mouse moves no focus in the menus (it steps the d-pad there instead).
    s_grid_update = reinterpret_cast<void*>(mod.function<void (*)()>(s_disc->grid_update));
    s_list_update = reinterpret_cast<void*>(mod.function<void (*)()>(s_disc->list_update));
    s_map_screen = reinterpret_cast<void*>(mod.function<void (*)()>(s_disc->map_screen));

    reset_state();

    mod.take_input(OPENRAC_INPUT_KEYBOARD | OPENRAC_INPUT_MOUSE);
    // Changed in the game's menu or the launcher while it runs: used from the next tick.
    mod.on_settings([] { read_settings(*s_mod); });
    mod.on_pad([](int port, std::uint8_t* report, int size) { on_pad(port, report, size); });
    mod.hook<rac1::FollowCamera_ApplyOverrides>([](auto original, std::uint32_t camera) {
        camera_overrides(original, camera);
    });
    mod.hook<rac1::Moby_Class440_Update>([](auto original, std::uint32_t moby) {
        aim_turret(original, moby, kBataliaCannon);
    });
    mod.hook<rac1::Moby_Turret_Update>([](auto original, std::uint32_t moby) {
        aim_turret(original, moby, kGasparTurret);
    });
    mod.hook<rac1::Moby_HovenTurret_Update>([](auto original, std::uint32_t moby) {
        aim_turret(original, moby, kHovenTurret);
    });
    openrac::frontend::set_mouse_turn(picture_turn);
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
