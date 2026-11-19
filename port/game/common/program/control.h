// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// --control: another program drives the game a tick at a time over stdin and stdout, for tools
// that look at a level in the game (tools/game_mcp.py, the game's MCP server for AI assistants).
// Each line on stdin is one JSON command; each answer is one line on stdout starting with
// "@openrac-control ". The game is held between commands: it runs only the ticks a command asks
// for, as fast as the machine allows. The commands and their answers: docs/port/GAME_MCP.md.
// It reads the game through the play host table (game_host.h) and never changes its logic: what
// it asks for (the hero placed, a level, a scene, the pad) is what a player or a mod could do.

#pragma once

#include <cstdint>
#include <string>

namespace openrac::program::control {

// Asks for the control channel. Called while the command line is read.
void request();

// Whether a run asked for it.
bool requested();

// Each tick, at its end: answers the commands, and holds the game until one asks for ticks.
void tick();

// Whether the program that drives the game asked to quit, or went: the program then ends as a
// run with --frames does.
bool quitting();

// The pad of port 0 as each tick reads it (frontend/input.cpp): the buttons and sticks a running
// command holds. Buttons are active low, as the pad reports them; sticks right x, y, left x, y.
void pad(std::uint16_t& buttons, std::uint8_t analog[4]);

// The file a picture of the next drawn frame goes to, once (frontend/interpolation.cpp), or empty.
std::string take_shot();

// The picture asked for was written (or not).
void shot_taken(bool ok);

}  // namespace openrac::program::control
