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
// The settings are openrac-mod.json's (shared with Going Commando's), kept as kbm_<key> lines in
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
#include "game/common/frontend/active_input.h"
#include "game/common/frontend/mouse_turn.h"
#include "game/common/key_labels.h"
#include "game/common/key_prompts.h"
#include "game/common/keyboard_mouse/kbm.hpp"
#include "game/rac1/host/prompt_spots.h"
#include "openrac/games/rac1.hpp"
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

// How many rows of a text list are read at most (assumed): a list that reaches it is no list.
constexpr int kMaxListRows = 32;

// The follow camera's locks (assumed: discs.h states it): bit 0 stops the stick turning it, bit 1
// means the game holds the tilt.
constexpr std::int32_t kFollowLockTurn = 1;
constexpr std::int32_t kFollowLockTilt = 2;

// How far the follow camera's eye may be tilted from level, in radians (assumed).
constexpr float kFollowEyeElevationLimit = 1.5f;

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

// The size of a pad report that holds the buttons and the four stick bytes, the least the mod
// writes into (assumed).
constexpr int kMinReportSize = 6;

// How far a gamepad's right stick byte may be from its centre before the camera is the pad's: the
// game's dead zone of 0.3 of the stick's travel (assumed).
constexpr int kPadLookDeadBytes = 38;

// The game's ticks a second, to turn the ramp setting into ticks: the PAL disc's 50, NTSC's 60.
float ticks_per_second(const openrac::Mod& mod) {
    return mod.version() == "pal" ? 50.0f : 60.0f;
}

// How many directions the menus' d-pad has (up, down, left, right).
constexpr int kMenuDirections = 4;

// ---- Types ----

// The tick's input.
struct Tick {
    const openrac_input* in = nullptr;
    Context where = Context::Play;
    bool play = false;     // play binds of the mouse count (in play, the cursor captured)
    bool pointer = false;  // menu binds of the mouse count (out of play, the cursor over the game)
};

// ---- State ----
//
// Everything below is touched by the game's thread only: the pad callback and the follow camera's
// hook point both run on it, in the order of a tick. load() sets it up before either runs.

// The mod and the disc's addresses, set by load().
openrac::Mod* s_mod = nullptr;
const kbm_disc* s_disc = nullptr;

// The player's settings, read by load() and again when they change (on_settings).
kbm::Bindings s_binds;
kbm::MoveTuning s_move;
kbm::LookTuning s_look_tuning;
float s_ring_dead_zone = 30.0f;
bool s_capture = true;
bool s_menu_mouse = true;
bool s_prompts = true;
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
std::array<kbm::Repeat, kMenuDirections> s_repeat;

// Whether the Quick Select was held open at the last tick.
bool s_ring_open = false;

// The port's number of the tick last stepped, so that a tick steps once however often the game
// reads the pad.
std::uint32_t s_tick = kNoTick;

// What was last asked of the port for the cursor (1 captured, 0 free), -1 before the first ask.
int s_asked_capture = -1;

// The camera's swing behind Ratchet: how many ticks it has left.
int s_reset_ticks = 0;

// The mouse's turn of the follow camera: the tick's motion, for its hook point to apply; the tick
// the hook point last ran (the follow camera is the one in use); whether the mouse holds the tilt,
// and where (the eye's elevation).
float s_follow_dx = 0.0f;
float s_follow_dy = 0.0f;
std::uint32_t s_follow_tick = kNoTick;
bool s_tilt_held = false;
float s_tilt = 0.0f;

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
kbm::Vec3 s_looks_up = kbm::kWorldUp;
std::int32_t s_follow_locks = 0;

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

// A game address worth reading: in the console's 32 MB, aligned.
bool plausible(gaddr address) {
    return address >= kGameMemoryStart && address < kGameMemoryEnd && (address & 3u) == 0;
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

// ---- The tick's input ----

// Whether one of an action's binds is held, or went down since the last tick (a tap shorter than a
// tick still presses for one): a mouse bind only where the mouse counts, any while the port's
// overlay leaves the input to the game.
bool active(const Tick& t, Action a) {
    const bool menu = kbm::kActions[static_cast<std::size_t>(a)].group == kbm::Group::Menu;

    // Menu actions are off in play, where the play layout is the keys'.
    if (menu && t.where == Context::Play) {
        return false;
    }

    // The port's overlay (its own menu) takes the input.
    if (t.in->overlay != 0) {
        return false;
    }

    // Look at each of the action's binds.
    for (int b : s_binds.of(a)) {
        // An empty bind slot holds nothing.
        if (b == 0) {
            continue;
        }

        // A mouse bind counts only where the mouse does: in play the captured cursor, in a menu
        // the cursor over the game.
        if (kbm::is_mouse(b) && !(menu ? t.pointer : t.play)) {
            continue;
        }

        // Held, or down since the last tick: the action is on.
        if (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }

    return false;
}

// Whether one of an action's binds went down this tick (not held from before), where its input
// counts: used for the presses that act once, such as taking an item or clicking.
bool pressed_now(const Tick& t, Action a) {
    // The port's overlay takes the input.
    if (t.in->overlay != 0) {
        return false;
    }

    const bool menu = kbm::kActions[static_cast<std::size_t>(a)].group == kbm::Group::Menu;

    // Look at each of the action's binds.
    for (int b : s_binds.of(a)) {
        // A set bind, whose input counts where the mouse does, that went down this tick.
        if (b != 0 && !(kbm::is_mouse(b) && !(menu ? t.pointer : t.play))
            && openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }

    return false;
}

// Whether one of an action's mouse binds is down (`now`: went down this tick), with the cursor over
// the game out of play: a click, not the same action from a key.
bool mouse_bind(const Tick& t, Action a, bool now) {
    // The port's overlay takes the input, and the mouse counts only with the cursor over the game.
    if (t.in->overlay != 0 || !t.pointer) {
        return false;
    }

    // Look at each of the action's binds.
    for (int b : s_binds.of(a)) {
        // A mouse bind that is down (or went down this tick).
        if (b != 0 && kbm::is_mouse(b)
            && (now ? openrac_bind_pressed(t.in, b)
                    : openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }

    return false;
}

// An action's wheel binds that turned this tick (a notch is one press, sent as a pulse), where the
// mouse counts in a menu.
bool wheel_notch(const Tick& t, Action a) {
    // Look at each of the action's binds.
    for (int b : s_binds.of(a)) {
        // A wheel bind (they come after the mouse's buttons) that turned, with the cursor over the
        // game and the overlay shut.
        if (b >= OPENRAC_BIND_WHEEL_UP && t.pointer && t.in->overlay == 0
            && openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }

    return false;
}

// Whether one of an action's binds other than a wheel's is down: a key held makes a menu direction
// repeat, a wheel notch is only one step.
bool held_not_wheel(const Tick& t, Action a) {
    // The port's overlay takes the input.
    if (t.in->overlay != 0) {
        return false;
    }

    // Look at each of the action's binds.
    for (int b : s_binds.of(a)) {
        // An empty slot, a wheel bind, or a mouse button with the cursor off the game holds
        // nothing.
        if (b == 0 || b >= OPENRAC_BIND_WHEEL_UP || (kbm::is_mouse(b) && !t.pointer)) {
            continue;
        }

        // Held, or down since the last tick: the action is on.
        if (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }

    return false;
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
        if (pressed_now(t, static_cast<Action>(static_cast<int>(Action::Weapon1) + n))) {
            take_item(slots[static_cast<std::size_t>(n)]);
            return;
        }
    }

    const int step = pressed_now(t, Action::WeaponNext)   ? 1
                     : pressed_now(t, Action::WeaponPrev) ? -1
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

// The mouse in a menu: over a page's item, it is the one the game focuses (as the d-pad moves the
// focus between a page's items: the next tick's focus change plays the game's own sound); over a
// row of the focused list or a cell of the focused item grid, the d-pad steps the selection to it.
// Elsewhere (the vendor, the dialogs) its motion steps the d-pad. A click on an item not yet
// focused focuses it first. Returns false when the click must wait for the focus to move.
bool menu_mouse(const Tick& t, bool click) {
    const openrac_input* in = t.in;
    const bool moved = in->mouse_dx != 0.0f || in->mouse_dy != 0.0f;

    // The mouse has no say out of a menu, or when the player turned it off.
    if (!t.pointer || !s_menu_mouse) {
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
    }

    // Elsewhere (and over a grid) the mouse's motion steps the d-pad, except on the title, which
    // has nothing to step, and at the vendor, where the mouse points (vendor_mouse).
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
    if (!mouse_bind(t, Action::MenuConfirm, false)) {
        s_map_drag = false;
    } else if (mouse_bind(t, Action::MenuConfirm, true) && over) {
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
    look = kbm::rotate(look, up, kbm::DirectLook::yaw(dx, dy, s_look_tuning));
    const float tilt = read<float>(state + KBM_FIRST_PERSON_TILT);

    // The game's tilt is a number: turn the view by how much the tilt changes, within the limit.
    if (std::isfinite(tilt)) {
        const float wanted = tilt + kbm::DirectLook::pitch(dx, dy, s_look_tuning);
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
    const float w = kbm::follow_target_of(s_tilt);
    s_mod->memory().write<float>(state + KBM_FOLLOW_TILT, w);
    s_mod->memory().write<float>(state + KBM_FOLLOW_TILT_RATE, 0.0f);
    const bool inverts = read<std::int32_t>(s_mod->relocate(s_disc->tilt_invert)) != 0;
    const gaddr stick_y = s_mod->relocate(s_disc->pad_sticks) + kPadRightYOffset;
    s_mod->memory().write<float>(stick_y, kbm::follow_stick_for(w, inverts));
}

// The follow camera's orbit, its smoothed copy and the eye turned by the mouse's motion (dx, dy)
// about up, and tilted, as far as the game's locks allow; the tilt then held where the mouse leaves
// it.
void turn_follow_camera(gaddr state, float dx, float dy, std::int32_t locks) {
    const gaddr cameras = s_mod->relocate_level(0, s_disc->cameras);
    const kbm::Vec3 up = kbm::unit_or(read_vec(cameras + KBM_CAMERAS_UP), kbm::kWorldUp);
    const gaddr orbit = state + KBM_FOLLOW_ORBIT;
    const gaddr smooth = state + KBM_FOLLOW_ORBIT_SMOOTH;
    const kbm::Vec3 target = read_vec(state + KBM_FOLLOW_TARGET);
    kbm::Vec3 o = read_vec(orbit);
    kbm::Vec3 m = read_vec(smooth);
    kbm::Vec3 e = kbm::minus(read_vec(state + KBM_FOLLOW_EYE), target);

    // The mouse moved sideways and the game does not lock the turn: turn all three about up.
    if (dx != 0.0f && (locks & kFollowLockTurn) == 0) {
        const float yaw = kbm::DirectLook::yaw(dx, dy, s_look_tuning);
        o = kbm::rotate(o, up, yaw);
        m = kbm::rotate(m, up, yaw);
        e = kbm::rotate(e, up, yaw);
    }

    // The game holds the tilt (a camera volume's): the mouse leaves it be. Else a mouse that moved
    // up or down tilts it. Else, if the player chose Levels out, the mouse does not hold it.
    if ((locks & kFollowLockTilt) != 0) {
        s_tilt_held = false;  // the game holds the tilt
    } else if (dy != 0.0f) {
        // A tilt not held yet (or one that is not kept) starts from where the eye now is.
        if (!s_tilt_held || !s_look_tuning.pitch_holds) {
            s_tilt = kbm::elevation(o, up);
        }

        s_tilt_held = true;
        // Looking down lifts the eye above Ratchet.
        const float limit = kbm::follow_max_elevation();
        const float pitch = kbm::DirectLook::pitch(dx, dy, s_look_tuning);
        const float now = kbm::clamp(s_tilt + pitch, -limit, limit);
        const float moved = now - s_tilt;
        s_tilt = now;
        const auto tilt = [&](kbm::Vec3 v) {
            const float elevation = kbm::elevation(v, up) + moved;
            const float e =
                kbm::clamp(elevation, -kFollowEyeElevationLimit, kFollowEyeElevationLimit);
            return kbm::with_elevation(v, up, e);
        };
        o = tilt(o);
        m = tilt(m);
        e = tilt(e);
    } else if (!s_look_tuning.pitch_holds) {
        s_tilt_held = false;  // the mouse not tilting: the game levels the camera out
    }

    write_vec(orbit, o);
    write_vec(smooth, m);
    write_vec(state + KBM_FOLLOW_EYE, kbm::plus(target, e));

    // The mouse holds the tilt: write the game's target for it.
    if (s_tilt_held) {
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
        s_tilt_held = false;
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
        if (!s_look_tuning.pitch_holds || (locks & kFollowLockTilt) != 0) {
            s_tilt_held = false;
        }

        // Keep holding the tilt where the mouse left it.
        if (s_tilt_held) {
            hold_tilt(state);
        }

        return;
    }

    turn_follow_camera(state, dx, dy, locks);
}

// ---- The tick ----

// Asks the port to capture the cursor in play, and to free it everywhere else, when that changes.
// The port frees it too while the window is not focused or its overlay is open.
void update_capture(bool play, const openrac_input* in) {
    const int want = s_capture && play && in->focused != 0 && in->overlay == 0 ? 1 : 0;

    // Ask the port only when the answer changed, not each tick.
    if (want != s_asked_capture) {
        s_mod->capture_mouse(want != 0);
        s_asked_capture = want;
    }
}

// The actions held this tick, before the latch holds back those held since before the game
// changed where it is.
kbm::ActionSet actions_held(const Tick& t) {
    kbm::ActionSet held = 0;

    // Check each action in turn.
    for (int a = 0; a < kbm::kActionCount; ++a) {
        // A held action joins the set.
        if (active(t, static_cast<Action>(a))) {
            held |= kbm::ActionSet{1} << a;
        }
    }

    return held;
}

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

// The menus' directions into v: the d-pad, a held key repeating; a wheel notch is one step.
void menu_directions(const Tick& t, kbm::ActionSet held, kbm::Virtual& v) {
    const Action dirs[kMenuDirections] =
        {Action::MenuUp, Action::MenuDown, Action::MenuLeft, Action::MenuRight};
    const std::uint16_t pads[kMenuDirections] = {kbm::kUp, kbm::kDown, kbm::kLeft, kbm::kRight};

    // Take each direction in turn.
    for (int i = 0; i < kMenuDirections; ++i) {
        const bool on = (held & kbm::bit(dirs[i])) != 0 && held_not_wheel(t, dirs[i]);
        // The wheel zooms the map (map_mouse), and steps elsewhere.
        const bool notch =
            (held & kbm::bit(dirs[i])) != 0 && !on && !s_map_shown && wheel_notch(t, dirs[i]);

        // A wheel notch is one queued press; a key is held, and repeats while it stays down.
        if (notch) {
            s_pulses.push(pads[i]);
        } else if (s_repeat[static_cast<std::size_t>(i)].step(on)) {
            v.held = static_cast<std::uint16_t>(v.held | pads[i]);
        }
    }

    v.held = static_cast<std::uint16_t>(v.held | s_pulses.step());
}

// The left stick into v: while the Quick Select is held, Triangle held and the mouse pointing the
// stick at a slot; else the movement keys.
void left_stick(const Tick& t, kbm::ActionSet held, bool ring, kbm::Virtual& v) {
    const auto key = [&](Action a) {
        return (held & kbm::bit(a)) != 0;
    };

    // The Quick Select is open: the mouse points at a slot, and the movement keys rest.
    if (ring) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kTriangle);

        // The mouse counts only with the cursor captured in play.
        if (t.play) {
            s_ring.add(t.in->mouse_dx, t.in->mouse_dy, s_ring_dead_zone);
        }

        const auto [x, y] = s_ring.stick(s_ring_dead_zone);
        v.left_x = x;
        v.left_y = y;
        s_keys = kbm::KeyStick{};
    } else {
        // The movement keys are the left stick everywhere: in a menu the game reads it as the d-pad.
        s_keys.step(
            key(Action::MoveForward),
            key(Action::MoveBack),
            key(Action::MoveLeft),
            key(Action::MoveRight),
            key(Action::Walk),
            s_move
        );
        v.left_x = s_keys.x;
        v.left_y = s_keys.y;
    }
}

// The camera, in play and out of the ring: the follow camera and the first-person view turned by
// the mouse itself; any other camera (a scene's own) by the mouse as the right stick.
void mouse_camera(const Tick& t, bool play, bool ring, std::uint16_t pad_buttons, kbm::Virtual& v) {
    const openrac_input* in = t.in;
    s_looks = Looks::None;

    // In play with the cursor captured and the Quick Select shut, the mouse turns the camera.
    if (t.play && !ring) {
        int id = -1;
        const gaddr camera = current_camera(&id);
        const bool follow =
            id == 0 || (s_follow_tick != kNoTick && s_tick - s_follow_tick <= kFollowSeenTicks);

        // Which camera is in use decides how the mouse turns it.
        if (camera != 0 && id == KBM_CAMERA_FIRST_PERSON) {
            first_person_look(camera, in->mouse_dx, in->mouse_dy);
            s_look.reset();
            s_looks = Looks::FirstPerson;
        } else if (camera != 0 && follow) {
            s_looks = Looks::Follow;
            s_follow_dx = in->mouse_dx;
            s_follow_dy = in->mouse_dy;
            s_look.reset();
        } else {
            const bool first_person = ((v.held | pad_buttons) & kbm::kL1) != 0;
            s_look.step(in->mouse_dx, in->mouse_dy, first_person, s_look_tuning);
            v.right_x = s_look.x;
            v.right_y = s_look.y;
        }

        // The player turns the camera sideways: the swing behind Ratchet stops.
        if (s_reset_ticks > 0 && in->mouse_dx != 0.0f) {
            s_reset_ticks = 0;
        }
    } else if (!play) {
        // Out of play the mouse's look rests, and the tilt it held is let go.
        s_look.x = 0.0f;
        s_look.y = 0.0f;
        s_tilt_held = false;
    }
}

// The game's button icons show the keys that press them while the player is on the keyboard and
// mouse, and the game's own again on the gamepad. Only the fonts' pictures change, never logic.
void update_prompts(kbm::Context where, int dialog_kind, bool map) {
    namespace kp = openrac::key_prompts;
    constexpr std::uint32_t kRamBytes = 32u << 20;
    kp::Fonts fonts;
    const bool found = kp::find_fonts(
        s_mod->memory().base(),
        kRamBytes,
        s_mod->relocate(s_disc->font_bank),
        s_mod->relocate(s_disc->font_table),
        fonts
    );
    if (!found) {
        s_painter.forget();
        return;
    }
    kp::Labels labels{};
    if (s_prompts && (openrac::frontend::keyboard_mouse_in_use())) {
        const auto actions = kbm::prompt_actions(where, dialog_kind, map, s_strafe_mod == 1);
        for (std::size_t i = 0; i < actions.size(); ++i) {
            const int bind = actions[i] != Action::Count ? s_binds.of(actions[i])[0] : 0;
            if (bind != 0) {
                labels[i] = openrac_bind_to_name(bind);
            }
        }
        // The sticks: the movement keys and the mouse.
        labels[kp::LeftStick] = kbm::move_label(s_binds);
        labels[kp::RightStick] = "Mouse";
    }
    s_painter.update(fonts, labels);
}

// The tick's pad from the keyboard and mouse: once a tick, when the port has read them anew.
void step(const openrac_input* in, std::uint16_t pad_buttons, bool pad_look) {
    Tick t;
    t.in = in;
    int dialog_kind = -1;
    t.where = context_now(&dialog_kind);
    const bool play = t.where == Context::Play;
    t.play = play && in->captured != 0 && in->overlay == 0;
    s_pad_look = pad_look;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    t.pointer = !play && in->pointer_inside != 0 && in->focused != 0 && in->overlay == 0
                && in->size >= offsetof(openrac_input, reserved2);

    update_capture(play, in);
    take_prompt_spots();

    kbm::ActionSet held = actions_held(t);
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
    s_map_shown = map && t.pointer && s_menu_mouse;
    kbm::Virtual v;
    // A click on a menu item not yet focused waits for the focus; then it confirms.
    const bool click = t.pointer && (held & kbm::bit(Action::MenuConfirm)) != 0
                       && pressed_now(t, Action::MenuConfirm);

    // A click of the mouse (not a key) on one of the game's button icons, or the text after it,
    // presses that button instead (prompt_click); at the vendor, a click presses the button of the
    // monitor it is on, and nothing elsewhere. The click's confirm then stays off while the
    // mouse's button is down.
    const bool mouse_click = click && mouse_bind(t, Action::MenuConfirm, true);
    std::uint16_t click_press = 0;
    bool click_taken = false;

    // The vendor's monitors.
    if (t.where == Context::Vendor && t.pointer && s_menu_mouse) {
        click_press = vendor_mouse(t, mouse_click);
        click_taken = mouse_click;
    } else if (!play) {
        click_press = mouse_click && s_menu_mouse ? prompt_click(t) : 0;
        // On the map a click drags it (map_mouse): it presses only an icon's button.
        click_taken = click_press != 0 || (map && mouse_click && s_menu_mouse)
                      || (mouse_click && s_menu_mouse && beside_items(t));

        // The focused map moves with the mouse.
        if (map && t.pointer && s_menu_mouse) {
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
    if (s_click_held && mouse_bind(t, Action::MenuConfirm, false)) {
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
        menu_directions(t, held, v);
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
    left_stick(t, held, ring, v);
    mouse_camera(t, play, ring, pad_buttons, v);

    // The camera's reset: the mouse's look starts afresh, and the camera swings behind Ratchet.
    if (play && pressed_now(t, Action::CameraReset)) {
        s_look.reset();
        s_tilt_held = false;  // the swing levels it
        s_reset_ticks = kResetTicks;
    }

    weapons(t);
    s_pad = v;
    update_prompts(t.where, dialog_kind, map);
}

// The first pad's report, before the game reads it: the keys and the mouse on top of the pad's.
// Runs on the game's thread, as often as the game reads the pad.
void on_pad(int port, std::uint8_t* report, int size) {
    // Only the first pad, and a report that holds the buttons and sticks.
    if (port != 0 || size < kMinReportSize) {
        return;
    }

    const openrac_input* in = s_mod->api().input();

    // The port has read the input anew: step once for this tick, however often the game reads.
    if (in->tick != s_tick) {
        s_tick = in->tick;
        const std::uint16_t pad_held = static_cast<std::uint16_t>(~(report[0] | (report[1] << 8)));
        // The pad's right stick past the game's dead zone (0.3): the camera is the pad's.
        const bool pad_look = std::abs(report[2] - kbm::kStickRestByte) > kPadLookDeadBytes
                              || std::abs(report[3] - kbm::kStickRestByte) > kPadLookDeadBytes;
        step(in, pad_held, pad_look);
    }

    kbm::merge(s_pad, report, size);
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
        s_tilt_held = false;
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

// ---- The frames between ticks ----

// The turn the next tick makes for mouse motion (dx, dy) it has not read yet, while the mouse
// turns the follow camera or the first-person view itself (frontend/mouse_turn.h). Not during the
// swing behind Ratchet or while the gamepad's stick has the camera; a lock of the game's stops the
// turn here as it stops the mouse.
bool picture_turn(float dx, float dy, openrac::frontend::MouseTurn* out) {
    if (s_mod == nullptr || s_looks == Looks::None || s_pad_look || s_reset_ticks > 0) {
        return false;
    }

    *out = {};
    out->first_person = s_looks == Looks::FirstPerson;
    const std::int32_t locks = out->first_person ? 0 : s_follow_locks;

    // The game lets the mouse turn the view sideways.
    if ((locks & kFollowLockTurn) == 0) {
        out->yaw = kbm::DirectLook::yaw(dx, dy, s_look_tuning);
    }

    // The game lets the mouse tilt it.
    if ((locks & kFollowLockTilt) == 0) {
        out->pitch = kbm::DirectLook::pitch(dx, dy, s_look_tuning);
    }

    out->up[0] = s_looks_up.x;
    out->up[1] = s_looks_up.y;
    out->up[2] = s_looks_up.z;
    out->pitch_limit = out->first_person ? kbm::kFirstPersonTiltLimit : kbm::follow_max_elevation();
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
    s_binds = kbm::read_bindings([&](const char* key) { return mod.setting<std::string>(key); });

    // Warn of each pair of actions that share a bind.
    for (const kbm::Conflict& c : kbm::conflicts(s_binds)) {
        mod.warn(
            std::string(openrac_bind_to_name(c.bind)) + " is bound to both "
            + kbm::kActions[static_cast<std::size_t>(c.a)].key + " and "
            + kbm::kActions[static_cast<std::size_t>(c.b)].key + ": it does both"
        );
    }

    s_move.walk = static_cast<float>(mod.setting<double>("walk_speed"));
    s_move.rise_ticks =
        static_cast<float>(mod.setting<double>("stick_ramp_ms")) * ticks_per_second(mod) / 1000.0f;
    s_look_tuning.sensitivity = static_cast<float>(mod.setting<double>("mouse_sensitivity"));
    s_look_tuning.sensitivity_y = static_cast<float>(mod.setting<double>("mouse_sensitivity_y"));
    s_look_tuning.invert_y = mod.setting<bool>("invert_y");
    s_look_tuning.pitch_holds = mod.setting<std::string>("camera_tilt") != "return";
    s_ring_dead_zone = static_cast<float>(mod.setting<double>("quick_select_dead_zone"));
    s_capture = mod.setting<bool>("capture_cursor");
    s_menu_mouse = mod.setting<bool>("menu_mouse");
    s_prompts = mod.setting<bool>("key_prompts");
}

// Puts everything the keys and the mouse hold back to rest, as at the first tick.
void reset_state() {
    s_pad = kbm::Virtual{};
    s_keys = kbm::KeyStick{};
    s_look.reset();
    s_ring.reset();
    s_latch = kbm::Latch{};
    s_pulses.clear();
    s_stepper.reset();
    s_repeat = {};
    s_ring_open = false;
    s_tick = kNoTick;
    s_asked_capture = -1;
    s_reset_ticks = 0;
    s_click_wait = 0;
    s_vendor_target = -1;
    s_click_held = false;
    s_follow_dx = 0.0f;
    s_follow_dy = 0.0f;
    s_follow_tick = kNoTick;
    s_tilt_held = false;
    s_pad_look = false;
    s_looks = Looks::None;
    s_looks_up = kbm::kWorldUp;
    s_follow_locks = 0;
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
    openrac::frontend::set_mouse_turn(picture_turn);
    mod.info("keyboard and mouse on the " + std::string(mod.version()) + " disc");
}

}  // namespace

namespace openrac::keyboard_mouse {

void query(openrac_mod_info* info) {
    info->api_major = OPENRAC_MOD_API_MAJOR;
    info->api_minor = OPENRAC_MOD_API_MINOR;
    info->id = "keyboard-mouse";
    info->version = "2.1.0";
}

int load(const openrac_api* api, openrac_mod* mod) {
    return openrac::detail::load(api, mod, ::load);
}

}  // namespace openrac::keyboard_mouse
