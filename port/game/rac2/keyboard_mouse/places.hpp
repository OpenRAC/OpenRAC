// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Where Going Commando is, as its keyboard and mouse sees it: the on-screen pad's context
// (game/rac2/host/touch.c) as one of the keyboard and mouse's contexts (kbm.hpp), and the ship's
// layout. Pure, for the tests.

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "game/common/keyboard_mouse/kbm.hpp"

namespace kbm2 {

// The touch context's name ("foot", "menu", ...; null for none) as a context: the levels' play
// ones are Play, the title and its menus Menu, the pause menu and the Galactic Map Menu, a vendor
// Vendor, a dialog Dialog, a minigame Menu (Circle tries again, Triangle quits), anything else
// (a scene, a movie, a load) Cutscene.
inline kbm::Context context_of(const char* touch) {
    const std::string_view c = touch != nullptr ? touch : "";
    if (c == "foot" || c == "grind" || c == "swim" || c == "giant_clank" || c == "default"
        || c == "ship") {
        return kbm::Context::Play;
    }
    if (c == "front_end" || c == "menu" || c == "map" || c == "minigame") {
        return kbm::Context::Menu;
    }
    if (c == "vendor") {
        return kbm::Context::Vendor;
    }
    if (c == "dialog") {
        return kbm::Context::Dialog;
    }
    return kbm::Context::Cutscene;
}

// The ship fires with Square and fires missiles with Circle: there the fire key presses Square and
// the wrench key Circle, so the left click still fires.
inline bool is_ship(const char* touch) {
    return touch != nullptr && std::string_view(touch) == "ship";
}

inline std::uint16_t ship_swap(std::uint16_t b) {
    const bool square = (b & kbm::kSquare) != 0;
    const bool circle = (b & kbm::kCircle) != 0;
    b = static_cast<std::uint16_t>(b & ~(kbm::kSquare | kbm::kCircle));
    return static_cast<std::uint16_t>(
        b | (square ? kbm::kCircle : 0) | (circle ? kbm::kSquare : 0)
    );
}

// The button icons' actions (kbm::prompt_actions), with the ship's swap: cells 1 and 3 are Square
// and Circle.
inline std::array<kbm::Action, 8> prompt_actions(const char* touch) {
    auto a = kbm::prompt_actions(context_of(touch), -1);
    if (is_ship(touch)) {
        std::swap(a[1], a[3]);
    }
    return a;
}

}  // namespace kbm2
