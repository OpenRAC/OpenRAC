// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' play calls (openrac_mod.h, interface 1.8: multiplayer), on the game's play
// (game_host.h, openrac_game_play): its phase and level, the hero's pose, the figures the mods made
// (whose they are, in which level load), the start or travel a mod asked for, kept until the frame
// is over (play_update), and the memory card's saves. mods.cpp puts them in the table.

#pragma once

#include <filesystem>

#include "openrac/game_host.h"
#include "openrac/openrac_mod.h"

namespace openrac::mods {

// Fills the play members of `api`, on `play` (null: the game has none) with the saves in `cards`
// (the memory card's folder, openrac_game_card_dir).
void fill_play_api(
    openrac_api& api, const openrac_game_play* play, const std::filesystem::path& cards
);

// Forgets the figures and a pending start or travel (mods::reset, the tests).
void reset_play();

// Takes figures of `mod` back: they go with the mod that made them (a mod that failed to load).
void drop_figures(const openrac_mod* mod);

}  // namespace openrac::mods
