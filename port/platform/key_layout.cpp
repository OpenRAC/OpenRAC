// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The keyboard layout (key_layout.h), read from SDL's keymap.
#include "platform/key_layout.h"

#include <array>
#include <atomic>
#include <cctype>

#include <SDL3/SDL.h>

namespace openrac::platform {
namespace {

// As many places as the port's key table (openrac/keys.h, OPENRAC_KEY_COUNT).
constexpr int kPlaces = 512;

// Each place's character, 0 for none; written on the window's thread, read on the game's.
std::array<std::atomic<char>, kPlaces> s_layout{};

}  // namespace

void refresh_key_layout() {
    for (int usage = 1; usage < kPlaces; ++usage) {
        const SDL_Keycode key =
            SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(usage), SDL_KMOD_NONE, false);
        // A printing ASCII character, as a capital; anything else is not the layout's to name.
        const bool printing = key > 0x20 && key < 0x7F;
        const char c = printing ? static_cast<char>(std::toupper(static_cast<int>(key))) : '\0';
        s_layout[static_cast<std::size_t>(usage)].store(c, std::memory_order_relaxed);
    }
}

char key_layout_char(int usage) {
    if (usage <= 0 || usage >= kPlaces) {
        return '\0';
    }

    return s_layout[static_cast<std::size_t>(usage)].load(std::memory_order_relaxed);
}

}  // namespace openrac::platform
