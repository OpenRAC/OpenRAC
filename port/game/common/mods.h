// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native mod loader (docs/port/MODS.md): at start, the launcher's list of mods
// (<data>/plugins/plugins.json) checked (mod_manifest.h), each library that passes opened and
// loaded through the interface of sdk/include/openrac/openrac_mod.h, its hooks bound to the game's
// hook points (hostgen's hooks.c), and what was done written to status.json. Then, each tick and
// each pad report, the callbacks the mods asked for. With no mod loaded nothing here runs: the hook
// points' slots stay null and the per-tick call is one test of a flag.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"
#include "openrac/script_binding.h"

namespace openrac::mods {

// One hook point as hostgen's hooks.c lists it (openrac_game_hook_points): the same layout.
struct HookPoint {
    const char* name;
    const char* symbol;
    const char* signature;
    void** slot;
    void* original;
};

// The game's tick and display-list state needed by a warm snapshot. The executable supplies these
// callbacks through Host so the mod loader stays usable without linking the game program (tests,
// manifest tools). `overlay` is the game's host-side mirror of the guest level.
struct SavestateRuntime {
    std::uint64_t frame = 0;
    gaddr display_chain = 0;
    unsigned draws = 0;
    bool reports_draws = false;
    bool vsync_since_kick = false;
    int overlay = -1;
};

// What the loader needs of the program.
struct Host {
    std::string game;             // "rac1"
    std::string version;          // "pal"
    std::string openrac_version;  // build_info::version()
    const HookPoint* hooks = nullptr;
    int hook_count = 0;
    std::filesystem::path list;  // plugins.json; the mods' folders are beside it
    bool purist = false;         // --no-mods: nothing is read or loaded
    int (*loaded_level)(void) = nullptr;
    // Where the pad library looks for a callback on each report (game_host.h,
    // openrac_game_pad_report_hook): set when a mod asks for one.
    void (**pad_hook)(int port, unsigned char* report, int size) = nullptr;
    // The time status.json says it was written ("2026-10-21T14:02:11+02:00").
    std::string now;
    // The game's binding for script mods (openrac/script_binding.h): null where it has none, and
    // then a script mod is refused.
    const openrac_script_binding* scripts = nullptr;
    // The keyboard and mouse (interface 1.2): the state for the tick about to run (null: a test's,
    // or no window, and then nothing is held), the port's keyboard table given up to a mod, and the
    // cursor captured or let go as a mod asks.
    const openrac_input* (*input)(void) = nullptr;
    void (*take_input)(std::uint32_t devices) = nullptr;
    void (*capture_mouse)(bool capture) = nullptr;
    // The graphics context (interface 1.4): what the port draws with (null: a test's, or no
    // window, and then the backend is none); the views a mod asked for, given once load is over
    // (null: none, as at reset); the Vulkan device setup a mod asked for, likewise.
    const openrac_graphics* (*graphics)(void) = nullptr;
    void (*set_views)(const openrac_view_source* source) = nullptr;
    void (*set_vulkan_setup)(const openrac_vulkan_setup* setup) = nullptr;
    // A mod's menu page (interface 1.5): where its rectangles and text go, while its callback runs
    // (null: a test's, or no window, and then nothing is drawn).
    void (*menu_rect)(int x, int y, int width, int height, std::uint32_t rgba) = nullptr;
    void (*menu_text)(int x, int y, int size, std::uint32_t rgba, const char* text) = nullptr;
    // The running game's essential host-side tick state and post-restore audio/renderer rebuild.
    bool (*capture_savestate_runtime)(SavestateRuntime* state) = nullptr;
    void (*restore_savestate_runtime)(const SavestateRuntime* state) = nullptr;
    // The game's play (interface 1.8, game_host.h; null: none, and the play calls say so) and the
    // memory card's folder its saves are in (openrac_game_card_dir).
    const openrac_game_play* play = nullptr;
    std::filesystem::path cards;
};

// Loads the mods the list has on. Returns the session header's line: "lock-strafe 1.0.0", "none",
// "none (purist mode)". Called once, after the game's functions are registered, before it runs:
// after the window opens, or before it when makes_vulkan_device says so.
std::string load(const Host& host);

// Whether an enabled mod of the list `list` (plugins.json) says in its manifest that it makes the
// Vulkan device ("vulkan_device": true, interface 1.4): then load() runs before the window opens,
// which makes the device, and otherwise after it, as before. Reads the manifests only; false in
// purist mode or with no list.
bool makes_vulkan_device(const std::filesystem::path& list, bool purist);

// Each loaded mod's openrac_mod_unload, last first (at exit).
void unload();

// The mods' frame callbacks, after each game tick.
extern bool g_frame_callbacks;
void run_frame_callbacks();
inline void frame() {
    if (g_frame_callbacks) {
        run_frame_callbacks();
    }
}

// Called after the game's vertical-blank handlers. Snapshot APIs are valid only while these
// callbacks run.
void tick_boundary(std::uint32_t tick);

// How many mods are loaded.
int loaded_count();

// A loaded mod's id ("multiplayer"), for the log; "a mod" for none.
const char* mod_id(const openrac_mod* mod);

// Play (interface 1.8, mod_play.h): the start or travel a mod asked for, done now, on the game's
// thread after the frame's menus (frontend/frame.cpp, beside the developer menu's start); nothing
// when none is pending.
void play_update();

// Whether a mod asked for the scene playing to be skipped (openrac_api.play_skip), for this pad
// read: the skip is pressed for a while (frontend/input.cpp, skip_after_mods).
bool play_skipping();

// The line a mod shows over the game's picture (openrac_api.play_notice), empty for none; the
// debug overlay draws it. From any thread.
std::string play_notice();

// The front-end menu entries the mods added (interface 1.5), in load order: how many, and an
// entry's label ("" past the list).
int menu_entry_count();
std::string menu_entry_label(int index);

// Runs entry `index`'s page for a frame: its callback with `frame`, its drawing sent to the host's
// menu_rect and menu_text. True when the page closes (the mod said so, or there is no such entry).
bool run_menu_page(int index, const openrac_menu_frame& frame);

// ---- Pages of the game's own menus (interface 1.7) ----
//
// What the mods put on their pages and what they ask of the screen, kept here; the front end
// (mod_ui.h) shows them with the game's widgets, tells which page is on the screen and where its
// cursor is, and sends the pages their events.

// One item of a page, as its mod last set it (openrac_ui_item), its texts cut to
// OPENRAC_UI_TEXT_MAX.
struct UiItem {
    std::uint32_t kind = OPENRAC_UI_BUTTON;
    std::uint32_t flags = 0;
    std::string label;
    std::vector<std::string> choices;  // OPENRAC_UI_CHOICE's values
    std::uint32_t choice = 0;          // the one shown
};

// Whether the cursor stops on an item: any but a line of text or a disabled one.
bool takes_cursor(const UiItem& item);

// A page (openrac_api.ui_page) and what is on it.
struct UiPage {
    std::uint32_t flags = 0;  // OPENRAC_UI_OWN_BACK
    std::string title;
    std::vector<UiItem> items;
    std::vector<std::string> footer;  // none: the game's hints
    std::uint64_t revision = 0;       // grows with each ui_set
    int cursor = 0;                   // the item under the cursor
};

// A main-menu item (openrac_api.ui_main_item): after which of the game's, its label, its page.
struct UiMainItem {
    std::string after;
    std::string label;
    int page = 0;
};

// An item of the pause menu's Options page (openrac_api.ui_pause_item, since 1.9): its label,
// its page.
struct UiPauseItem {
    std::string label;
    int page = 0;
};

// What the mods asked of the screen during their events since the last take, for the front end.
struct UiRequests {
    bool open = false;  // ui_open: `page`, or OPENRAC_UI_GO_BACK
    int page = 0;
    bool confirm = false;  // ui_confirm: a question on `confirm_page`
    int confirm_page = 0;
    std::string question;
    std::uint32_t tag = 0;
    std::vector<int> sounds;  // ui_sound, in order
};

// The pages the mods made, by id (null past them), and the main-menu items, in order.
int ui_page_count();
const UiPage* ui_page(int page);
int ui_main_item_count();
const UiMainItem* ui_main_item(int index);
// The pause menu's Options items, in order.
int ui_pause_item_count();
const UiPauseItem* ui_pause_item(int index);

// The front end's side: the page on the screen (-1 none; whose mod may then open, ask and play),
// whether a question is open on it, where a page's cursor is now.
void ui_set_shown(int page, bool question_open);
// Whether the title's main menu is on the screen, settled (a mod may open its main-menu item's
// page from it, since 1.9).
void ui_set_main_menu(bool up);
void ui_set_cursor(int page, int item);
// A choice the player stepped: shown so until the mod sets the page again.
void ui_set_choice(int page, int item, std::uint32_t choice);

// Sends `event` to `page`'s callback (its cursor filled in). Nothing for a page there is not.
void ui_send(int page, openrac_ui_event event);

// What the mods asked for since the last call, emptied.
UiRequests take_ui_requests();

// For tests: unloads every mod, puts each hook point's slot back to null and forgets the
// callbacks, so that load() can run again.
void reset();

}  // namespace openrac::mods
