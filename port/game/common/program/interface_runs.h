// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The program's side of interface texture runs (renderer/textures/interface_run.h,
// docs/port/TEXTURE_PACKS.md, "Export without playing"):
//
//   --interface-run FILE   notes the interface textures the game makes while it runs; at exit,
//                          finds their bytes on the disc (the sectors the game read, as they are
//                          and inside WAD streams) and writes FILE: a part of a run, which
//                          port/tools/interface_runs/run.py joins with the other parts.
//   --interface-export DIR [--interface-runs FILE]... [--only font,interface,effects]
//                          rebuilds every texture of the run files from the player's disc into DIR
//                          (Font/, HUD and menus/, Effects/), each checked against its name, and
//                          ends. Prints one JSON object a line for the launcher.
//
// A run file (format 1) is JSON:
//
//   {"format": 1, "game": "rac1-ntsc", "converter": 1,
//    "blobs": {"<hash>": {"size": N, "at": <byte on the disc>}
//              | {"size": N, "wad": <byte of the WAD stream on the disc>, "at": <byte inside it>}},
//    "textures": [{"name": "<hash>", "category": "font", "kind": "pool",
//                  "tex0": "<hex>", "texa": "<hex>", ["tbp": <TEX0's base past 14 bits>,]
//                  "uploads": [{"dbp": 0, "dbw": 4, "dpsm": 19, "x": 0, "y": 0, "w": 256, "h": 256,
//                               "blob": "<hash>"}]}
//                 | {"name": "<hash>", "category": "effects", "kind": "indexed8", "texa": "<hex>",
//                    "w": 64, "h": 64, "indices": "<hash>", "palette": "<hash>"}],
//    "missing": {"font": [], "interface": ["<hash>"], "effects": []}}
//
// "missing" names the textures the run made whose bytes were not all found on the disc (a part
// writes it; run.py keeps those no part found). run.py adds what it knows of the run (when, which
// routes, official or not, counts by category); the program reads only "format", "game",
// "converter", "blobs" and "textures". Hashes are 16 lowercase hex digits.
#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace openrac::program {

// The run file's format, and the categories' folders in an export.
inline constexpr int kInterfaceRunFormat = 1;

// --interface-run: starts noting; `file` is written at exit (finish_interface_run).
void start_interface_run(const std::filesystem::path& file);

// Writes the part's file, when a run was started. main has it run at exit.
void finish_interface_run(const std::filesystem::path& disc);

// Writes one line for the launcher (main has it print to stdout).
using Say = std::function<void(const std::string& line)>;

// --interface-export: the whole export, into `out` or, when that is there, out-2, out-3... Says
// one JSON event a line, then "openrac-output: <folder>". Returns the program's exit code: 0 when
// it wrote something (some textures may be skipped, each counted), 1 when it could do nothing (no
// disc, no run file it can read, no texture that came out as named).
int export_interface(
    const std::filesystem::path& disc,
    const std::string& game,
    const std::filesystem::path& out,
    const std::vector<std::filesystem::path>& runs,
    const std::string& only,
    const Say& say
);

}  // namespace openrac::program
