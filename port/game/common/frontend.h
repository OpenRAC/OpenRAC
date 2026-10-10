// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window of a native game: once a frame, what the game holds in its memory is drawn by the
// port's renderer, and the keyboard and controllers are read for the game's pad library.
//
// Only built where the window libraries are (port/cmake/Window.cmake); the program runs without a
// window otherwise. What a game's memory means (where its camera and objects are) is the game's:
// so far Ratchet & Clank (PAL), through viewer/game_state.h, with each level's geometry from the
// extractor's port export (editor/port.py).

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace openrac::frontend {

// Opens the window. `levels` holds the extracted levels (level_00, level_01, ...).
bool open(const std::string& game_id, const std::filesystem::path& levels, std::string& error);

// Draws a frame from the game's main memory and reads the input; false once the window was closed.
bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain);

// The pad of a port as the game's pad library reads it: buttons active low, then the right and
// left sticks (x, y each).
bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]);

void close();

}  // namespace openrac::frontend
