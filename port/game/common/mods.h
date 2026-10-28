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
    // The running game's essential host-side tick state and post-restore audio/renderer rebuild.
    bool (*capture_savestate_runtime)(SavestateRuntime* state) = nullptr;
    void (*restore_savestate_runtime)(const SavestateRuntime* state) = nullptr;
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

// For tests: unloads every mod, puts each hook point's slot back to null and forgets the
// callbacks, so that load() can run again.
void reset();

}  // namespace openrac::mods
