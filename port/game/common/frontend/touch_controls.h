// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the on-screen pad shows in the Arc style (platform/touch_pad.h): the game's context, asked
// each frame (game_host.h, openrac_game_touch), looked up in the action sets (touch_actions.h):
// game/common/touch/sets/common.json, then the game's own (<game>.json), then the mods' (mod_touch.h:
// a mod may put a context of its own over the game's, a vehicle of its own, and raise hints). The
// icons are game/common/touch/icons/*.svg and the mods' own.
//
// A context the game moves into is taken once it has held for a few frames, so that a state the
// game passes through for a frame does not flash a set of buttons. A context with no set shows the
// "default" set (a game that reports none: rac3, rac4).

#pragma once

#include "platform/touch_pad.h"

namespace openrac::frontend::touch_controls {

// Once a frame, before the pad is drawn: the player's style (settings touch_style: 0 Arc, the
// default, or 1 Classic), the game's context, the accent.
void update(platform::TouchPad& touch, int style);

// While a movie Start skips plays (program/movie.cpp): the "cutscene" set, its skip button, over the
// game's context (the intro logos play before the game reports one).
void set_movie(bool skippable);

}  // namespace openrac::frontend::touch_controls
