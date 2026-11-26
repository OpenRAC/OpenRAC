// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shoulder tabs for the key prompts' tests (game/common/key_prompts.h): in a font of the test's own
// with a grey palette (entry i is grey i), the L1, L2, R1 and R2 cells get a tab shaded along x and
// y, each with a bright letter of its own, as the game's are.

#pragma once

#include <cstdint>

inline void draw_tabs(std::uint8_t* font) {
    for (int t = 0; t < 4; ++t) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 24; ++x) {
                std::uint8_t v = static_cast<std::uint8_t>(60 + 2 * x + 3 * y);
                // Each tab's letter: a stroke at its own column and one row all four share.
                if ((x == 6 + 3 * t && y >= 4 && y <= 11) || (y == 7 && x >= 5 && x <= 8)) {
                    v = 250;
                }
                font[(96 + y) * 256 + 96 + 24 * t + x] = v;
            }
        }
    }
}
