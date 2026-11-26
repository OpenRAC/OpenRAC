// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// [debug tools] OPENRAC_INPUT_LATENCY: the input latency, measured rather than felt. A virtual
// gamepad (SDL's, so the press takes the path a real controller's does: SDL's joystick update, the
// window's events, platform::Input) presses one button at random moments, unrelated to the ticks
// as a player's thumb is. For each press the log gets: how long until the port's input has it
// (sampled), how long until the game's own pad read
// (openrac_game_pad, port 0) first sees it, and how long until the first two frames shown whose
// tick is the one that read it or later (on the console the second: the frame a tick builds is
// sent at the start of the next). docs/port/DEBUGGING.md has the switch.

#pragma once

#include <cstdint>

namespace openrac::frontend::input_latency {

// The port's input sampled the controllers (window events read, or the pads refreshed before a
// tick): port 0's `buttons`, active low.
void sampled(std::uint16_t buttons);

// The game read port 0's pad (frontend::pad): `buttons` as it hands them on, active low, during
// window frame `index`. Starts the probe on its first call when the variable is set.
void pad_read(std::uint16_t buttons, std::uint64_t index);

// A frame was swapped onto the window: `tick`, the window frame it was captured on.
void shown(std::uint64_t tick);

}  // namespace openrac::frontend::input_latency
