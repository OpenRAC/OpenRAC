/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The 2D layer's quads as the game's data alone (text, HUD, menus): what a wrapped 2D routine
 * hands the window in place of drawing. No record carries a word of the console's graphics chip:
 * corners are the game's screen pixels, the texture is an image as the game keeps it in its own
 * memory (a font page, a HUD picture), the colour is the game's. The window decodes each image to
 * RGBA once per content and draws the quads in the order they come (docs/port/FRAME_RECORDER.md).
 */
#ifndef OPENRAC_UI_ITEM_H
#define OPENRAC_UI_ITEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An image of the 2D layer in game memory: `pixels` and `palette` are game addresses (palette 0
 * for 32-bit pixels), `bits` 4, 8 or 32 per pixel in rows top first (4: the low nibble first), the
 * palette 16 or 256 32-bit colours (RGBA, alpha 0x80 opaque); `swizzled`: a 256-colour palette in
 * the game's stored order, whose entries 8 to 15 and 16 to 23 of each 32 trade places. */
typedef struct openrac_game_ui_image {
    uint32_t pixels;
    uint32_t palette;
    uint16_t width;
    uint16_t height;
    uint8_t bits;
    uint8_t swizzled;
    uint8_t reserved[2];
} openrac_game_ui_image;

/* openrac_game_ui_item::flags: a glyph of text, drawn from the page's distance field, sharp; no
 * image, the colours alone; shaded between the corners' colours, else each triangle flat in the
 * colour of its last corner (2, then 3); cut to `clip`; the image sampled texel by texel, not
 * filtered. */
#define OPENRAC_UI_GLYPH 1u
#define OPENRAC_UI_FLAT 2u
#define OPENRAC_UI_SHADED 4u
#define OPENRAC_UI_CLIP 8u
#define OPENRAC_UI_NEAREST 16u

/* A quad of the 2D layer: four corners in strip order (0-1-2, 1-2-3) in the game's screen pixels,
 * from the top left of the screen the game draws its 2D in (`screen_width` x `screen_height`: 512 x
 * 448 on PAL); each corner's texel of `image` (in its pixels) and colour (0xAABBGGRR, 0x80 full, as
 * the game keeps it); `clip` x0, y0, x1, y1 in the same pixels (with OPENRAC_UI_CLIP). */
typedef struct openrac_game_ui_item {
    float x[4], y[4];
    float u[4], v[4];
    uint32_t rgba[4];
    float clip[4];
    float screen_width;
    float screen_height;
    uint32_t flags;
    openrac_game_ui_image image;
} openrac_game_ui_item;

#ifdef __cplusplus
}
#endif

#endif
