/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The boot pictures (the memory card warning before the title, the loading pictures): the game
 * copies a raw picture from the boot lump straight into the frame buffer it draws into
 * (libraries.tsv, port = wrap). What these pictures are and where they come from follows ReRAC's
 * notes (ISC License; rc-formats frontend.rs, BootPictures, and docs/plan/progression.md,
 * "Boot -> title"). */
#include "rac1.h"

#include <stdint.h>

/* The display list's write cursor, the PAL flag (448 lines, else 416) and the screen's height in
 * lines (the second word of the viewport at 0x13E600). */
#define DISPLAY_LIST RAC1_ADDR(0x00161000u)
#define PAL_LINES RAC1_ADDR(0x0015EE80u)
#define SCREEN_LINES RAC1_ADDR(0x0013E604u)

/*
 * The game sends a 512-pixel-wide PSMCT32 picture at `image` to the draw buffer in bands of 128
 * rows with GIF IMAGE transfers (the DMA reads the pixels where they are). The chip then holds the
 * picture in the frame, which the game fades and copies to the display buffer like any frame.
 *
 * The port's 2D path keeps every image transfer as a texture and draws only primitives into its
 * frame, so a transfer into the frame buffer never shows. After the game's own packets, this
 * records the picture as one image (openrac_frame_picture_image) over the whole 2D screen, as the
 * console's display stretched it: one texture the window scales as a whole, which a texture pack
 * can replace with one file.
 */
static void draw_picture(gaddr image, int rows) {
    if (rows <= 0 || rows > 512) {
        return;
    }
    openrac_frame_anchor_at(OPENRAC_DATA(DISPLAY_LIST));
    const openrac_game_ui_image picture =
        {(uint32_t)image & 0x01FFFFFFu, 0, 512, (uint16_t)rows, 32, 0, {0, 0}};
    openrac_frame_picture_image(&picture, NULL, 0x80808080u, OPENRAC_BLEND_OPAQUE);
}

#if !RAC1_OMIT(func_00201AF0)
/* draw_bootImage (func_00201AF0, matched C in the decompilation): the picture shown while a level
 * loads, 448 lines on PAL (416 otherwise), then a TEXFLUSH. */
void func_00201AF0(unsigned int image) {
    func_00201AF0__game(image);
    draw_picture(image, GREF(int32_t, OPENRAC_DATA(PAL_LINES)) ? 448 : 416);
}
#endif

#if !RAC1_OMIT(func_001F7680)
/* func_001F7680 (the decompilation's functional C): the same transfer of a screen-high picture,
 * every frame of the boot loop (the memory card warning, the loading pictures), then a
 * texture flush (VU1_texFlush). */
void func_001F7680(unsigned int image) {
    func_001F7680__game(image);
    draw_picture((gaddr)image, GREF(int32_t, OPENRAC_DATA(SCREEN_LINES)));
}
#endif
