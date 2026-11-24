// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// --moby-states FILE: the game loads the --level, runs its mobys' updates for a number of ticks
// and writes how each placed moby is then (spawned or not, hidden, its state and place) to FILE as JSON, and
// the program stops. OpenRAC Editor runs it on opening a level (its tools/moby_states.py) to show the
// mobys as the game starts them. The run is as fast as the machine allows: no frame pacing, no
// movies, the game started straight in the level. The command line and the file are a contract
// with the editor: docs/port/MOBY_STATES.md.

#pragma once

#include <filesystem>

namespace openrac::program::moby_states {

// Asks for the states of the level's mobys after `ticks` ticks in the level, written to `out`.
// Called while the command line is read, before the game starts.
void request(const std::filesystem::path& out, long ticks);

// Whether a run asked for them.
bool requested();

// Each tick: once the level has run long enough, writes the file and ends the program.
void tick();

}  // namespace openrac::program::moby_states
