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

#include "openrac/game_host.h"

namespace openrac::frontend {

// Opens the window. `levels` holds the extracted levels (level_00, level_01, ...).
bool open(const std::string& game_id, const std::filesystem::path& levels, std::string& error);

// Draws a frame from the game's main memory and reads the input; false once the window was closed.
// The game's world renderers are drawing now: the camera they draw with is kept for this frame's
// world (a menu may set its own camera afterwards to draw its frame objects).
void world_drawn(std::span<const std::uint8_t> ram);

// The game drew `count` mobys from `first` with the camera it holds now (the page menu's frame
// objects, drawn with the menu's camera): they are drawn as that camera saw them.
void mobys_drawn(std::span<const std::uint8_t> ram, std::uint32_t first, int count);

// A world effect quad the game drew this frame (openrac_game_effect_quad).
void effect_quad(std::span<const std::uint8_t> ram, const openrac_game_quad& quad);

// A sky sprite the game drew this frame (openrac_game_sky_sprite).
void sky_sprite(const std::uint8_t* record, std::uint64_t tex0);

// A live particle the game drew this frame (openrac_game_particle).
void particle(std::span<const std::uint8_t> ram, const std::uint8_t* record, std::uint32_t pixels,
              std::uint32_t clut, int log2_side);

// `draws`: the layers the game's renderers drew this frame (bit n = renderer::Bucket n; all bits
// when the game does not report them).
bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain, std::uint32_t draws);

// An image the game sent to the GS outside the display list (its library's image transfer): kept
// for every renderer the window makes, as the chip keeps it in its memory. `base` in 256-byte
// blocks, `width_units` in 64 pixels, `pixels` in raster order in `psm`.
void upload_image(
    std::uint32_t base,
    std::uint32_t width_units,
    std::uint8_t psm,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels
);

// Shows one full-screen picture the game puts straight into its display buffer (a movie's frame,
// a boot still), RGBA rows top to bottom, with `black` (0 to 1) of black over it (the game's
// FadeToBlack), and reads the input. Counts as a frame (shots, OPENRAC_PRESS). False once the window
// was closed.
bool show_picture(const std::uint8_t* rgba, int width, int height, float black);

// The pad of a port as the game's pad library reads it: buttons active low, then the right and
// left sticks (x, y each).
bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]);

void close();

}  // namespace openrac::frontend
