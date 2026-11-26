/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Up Your Arsenal's adapter to the shared 2D layer (ui_adapter.c). */
#pragma once

#include <stdint.h>

#include "openrac/guest.h"
#include "openrac/ui_item.h"

/* Gives the shared 2D layer this game's adapter (once; later calls do nothing). */
void rac3_ui_adapter_install(void);

/* The running program's 2D offset words (12.4, x then y): what the game adds to 2D positions. */
gaddr rac3_ui_offset(void);

/* The running program's frame block, whose halfwords at +0x150 and +0x152 are its screen's width
 * and height in pixels: what SetScissor keeps its scissor within. */
gaddr rac3_ui_frame(void);

/* The whole screen's 2D offset (12.4): where the chip places the left and top edges of the
 * screen's pixel (0, 0), which a native quad's corners are taken from. A split-screen view's own
 * offset is that plus the view's top left. */
void rac3_ui_origin(int* x, int* y);

/* The top left of the view being drawn, in the whole screen's pixels: how far the program's 2D
 * offset has moved its pixel (0, 0) (0, 0 outside a split-screen view). */
void rac3_ui_view_corner(float* x, float* y);

/* The font FontSelect chose last in the running program (0, 1). */
int rac3_ui_font_index(void);

/* Glyph page `page` (0 to 15) of font `font` (0, 1) of the running program, as an image in game
 * memory: nonzero when the font has it. */
int rac3_ui_font_page(int font, int page, openrac_game_ui_image* out);

/* The glyph page of font `font` whose image and palette the game put at the handles
 * `image_handle` and `palette_handle` (the values its tables at `images` and `palettes` hold for
 * the font's pages and palettes, which a glyph's builder is given): -1 when none is. */
int rac3_ui_font_page_of(
    int font, uint32_t image_handle, uint32_t palette_handle, gaddr images, gaddr palettes
);
