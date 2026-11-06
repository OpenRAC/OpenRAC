// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the mods ask of the on-screen pad's buttons (mod interface 1.13, docs/port/MODS.md "Touch
// controls"): sets of actions over the game's (a manifest's "touch" file, touch_define), icons of
// their own (touch_icon), a context of their own over the game's (touch_push: a vehicle of the
// mod's) and hints (touch_hint). Kept here, with or without a window; the window's pad reads them
// each frame (game/common/frontend/touch_controls.cpp).

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openrac::mods::touch {

struct Snapshot {
    unsigned generation = 0;  // changes when a set or an icon comes or goes
    bool files = false;       // sets and icons filled (snapshot()'s `known` was not `generation`)
    std::vector<std::pair<std::string, std::string>> sets;   // mod, JSON; in the order given
    std::vector<std::pair<std::string, std::string>> icons;  // name, SVG
    std::string context;             // the last mod's pushed context, empty for none
    std::vector<std::string> hints;  // up now, every mod's
};

// False with `error` set for text that is not a set (a JSON object with "contexts") or an SVG. A
// set's actions are checked when the pad reads it (touch_controls.cpp), which logs a bad one.
bool define(std::string_view mod, std::string_view json, std::string& error);
bool icon(std::string_view mod, std::string_view name, std::string_view svg, std::string& error);
// A manifest's "touch" file (`file`, inside the mod's folder `root`): its sets, and its "icons"
// (name -> an .svg inside the mod). False with `error` set when it cannot be read or is not a set;
// an icon that cannot be read is skipped with a line in `error` and the rest taken.
bool load_file(
    std::string_view mod,
    const std::filesystem::path& root,
    std::string_view file,
    std::string& error
);

// "" gives the mod's context back.
void push(std::string_view mod, std::string_view context);
void hint(std::string_view mod, std::string_view hint, bool on);

// A copy of what the mods ask now: the sets and icons only when they changed since `known` (a
// generation from an earlier snapshot), the context and hints always.
Snapshot snapshot(unsigned known = ~0u);

// Everything a mod asked, gone, its icons too (the mod unloaded; tests).
void forget(std::string_view mod);

// Everything every mod asked, gone (the mods unloaded).
void clear();

}  // namespace openrac::mods::touch
