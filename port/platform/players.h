// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which controller is which player: the game's two pad ports, and what reads into each.
//
// Each player names what it reads from, in the global controls file (docs/port/CONTROLS.md):
//
//   player1 = auto, keyboard-fallback
//   player2 = auto
//
// "auto" is the first controller no other player has; a controller's id one controller, whichever
// order it was connected in; "keyboard" the keyboard, always; "keyboard-fallback" the keyboard
// while the player has no controller; "none" nothing. (The on-screen pad is always player 1's.) A
// player may name several controllers, the first one connected counting, and "auto" after them as
// a fallback. The defaults: the first controller is player 1, the second player 2, and the
// keyboard is player 1's only while no controller is theirs.
//
// A controller's id is SDL's GUID for its model, and its serial number where SDL reports one,
// so it is the same from one run to the next; two of one model without serials are told apart
// by the order they were connected in ("<guid>#2"). SDL's instance numbers, new each time a
// controller connects, are never kept.

#pragma once

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openrac::platform {

inline constexpr int kPlayers = 2;  // the game's pad ports, input.h's kPadPorts

// The keyboard, always; and the keyboard while the player has no controller.
inline constexpr std::string_view kKeyboard = "keyboard";
inline constexpr std::string_view kKeyboardFallback = "keyboard-fallback";

// What each player reads from, as the file says it: "auto", ids, "keyboard", "keyboard-fallback" or
// "none".
struct Players {
    std::array<std::vector<std::string>, kPlayers> wants;

    bool operator==(const Players&) const = default;
};

// The defaults: player 1 the first controller (the keyboard while there is none), player 2 the
// next.
Players default_players();

// The settings key of a player, "player1" or "player2".
std::string player_key(int player);

// One player's value as the file writes it ("keyboard, auto"), read into names; empty or "none"
// reads as nothing.
std::vector<std::string> read_player(std::string_view value);

// The names as the file writes them, "none" for nothing.
std::string player_value(const std::vector<std::string>& names);

// Which controller, by its place in `connected` (the controllers' ids, in the order they were
// connected), each player reads, -1 for none; and which player has the keyboard, -1 none.
struct Assignment {
    std::array<int, kPlayers> pad{-1, -1};
    int keyboard = -1;

    bool operator==(const Assignment&) const = default;
};

// Assigns the controllers: first the players that name a connected controller's id get it, then
// the players that say "auto" take the first ones left, in the players' order.
Assignment assign(const Players& players, std::span<const std::string> connected);

// The id of a controller of model `guid` (SDL's GUID as text) with serial number `serial` (empty
// when SDL has none), not one of `taken` (the connected controllers' ids).
std::string controller_id(
    std::string_view guid, std::string_view serial, std::span<const std::string> taken
);

}  // namespace openrac::platform
