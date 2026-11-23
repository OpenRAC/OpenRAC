/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The 2D layer every game shares (port/ui, docs/port/FRAME_RECORDER.md section 6): a game's
 * wrapped 2D routines hand it what they would draw, from the game's own data, and it records the
 * native quads (openrac/ui_item.h) through the frame recorder (openrac/frame.h). A game says where
 * its data is through its adapter; the core knows no game.
 */
#ifndef OPENRAC_UI_H
#define OPENRAC_UI_H

#include <stdint.h>

#include "openrac/ui_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What a game tells the 2D layer. Fields a game does not have stay NULL. */
typedef struct openrac_ui_adapter {
    /* The screen the game draws its 2D in, in its pixels (512 x 448 on PAL): what 2D coordinates
     * span. NULL: 512 x 448. */
    void (*screen)(float* width, float* height);
    /* Page `page` of font `font` as the game keeps it: nonzero when there is one. */
    int (*font_page)(int font, int page, openrac_game_ui_image* out);
    /* Image `index` of the game's table `table` (its HUD bank, menu art, icons): nonzero when there
     * is one. */
    int (*image)(int table, int index, openrac_game_ui_image* out);
} openrac_ui_adapter;

/* The game's adapter, kept (not copied) from now on. */
void openrac_ui_set_adapter(const openrac_ui_adapter* adapter);

/* The game's 2D screen, as its adapter says (512 x 448 without one). */
void openrac_ui_screen(float* width, float* height);

/* A glyph of text: the cell `cell` (u0, v0, u1, v1, in the page's texels) of `page`, drawn over
 * `rect` (x0, y0, x1, y1, in the game's 2D pixels), each corner in its colour (`rgba`, strip order:
 * top left, top right, bottom left, bottom right; 0xAABBGGRR, 0x80 full). Drawn sharp at any size
 * from the page's distance field. Nonzero when the window draws it; 0: the caller draws its way. */
typedef struct openrac_ui_glyph {
    openrac_game_ui_image page;
    float cell[4];
    float rect[4];
    uint32_t rgba[4];
} openrac_ui_glyph;

int openrac_ui_draw_glyph(const openrac_ui_glyph* glyph);

/* A quad of an image, or of colour alone: four corners (strip order: 0-1-2, 1-2-3) in the game's
 * 2D pixels, each corner's texel of `image` and colour; `flags` OPENRAC_UI_FLAT (no image: the
 * colours alone), OPENRAC_UI_SHADED (shaded between the corners' colours, else each triangle in
 * the colour of its last corner), OPENRAC_UI_NEAREST (the image texel by texel). Nonzero when the
 * window draws it; 0: the caller draws its way. */
typedef struct openrac_ui_quad {
    openrac_game_ui_image image;
    float x[4], y[4];
    float u[4], v[4];
    uint32_t rgba[4];
    uint32_t flags;
} openrac_ui_quad;

int openrac_ui_draw_quad(const openrac_ui_quad* quad);

/* An axis-aligned rectangle `rect` (x0, y0, x1, y1) of the texels `cell` (u0, v0, u1, v1) of
 * `image`, each corner in its colour (strip order), as openrac_ui_draw_quad. Without an image
 * (`image` NULL) it is drawn in its colours alone, as with OPENRAC_UI_FLAT. */
int openrac_ui_draw_rect(
    const openrac_game_ui_image* image,
    const float rect[4],
    const float cell[4],
    const uint32_t rgba[4],
    uint32_t flags
);

/* What the 2D layer draws from here on is cut to `rect` (x0, y0, x1, y1 in the game's 2D pixels,
 * x1 and y1 past the last pixel kept; NULL: the whole target), as the game's own scissor cuts it. */
void openrac_ui_scissor(const float* rect);

/* Image `index` of the game's table `table`, through its adapter: nonzero when there is one. */
int openrac_ui_image(int table, int index, openrac_game_ui_image* out);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_UI_H */
