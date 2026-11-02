/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the picture behind the menus. The title's picture (func_0026EA00, a 512 x 416 image
 * from the disc) goes straight to the frame buffer's pages (func_002FCB00). When a menu opens the game
 * saves the frame buffer to main memory, 64 x 64 tiles of PSMCT24 read from those pages (func_002FC8D0),
 * and every frame it writes the tiles back over the buffer it has just cleared (func_002FC990), then
 * draws the menu over them: retail's state at the main menu holds that picture at +0x20 of the menu
 * block (0x001F2840). The window has no chip memory to write into, so the picture is kept whole, RGBA,
 * and drawn as one sprite where the tiles would have landed. In a level the picture is the frame the
 * window drew, which it keeps at its own resolution and shape: a marker in the list draws that over
 * the chip's 512 x 416 copy, or the world under a pause menu is soft and squeezed into the 4:3 middle.
 */
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"

enum {
    PICTURE_WIDTH = 512,
    PICTURE_HEIGHT = 448,
    /* A block of the texture pool no game upload uses. */
    PICTURE_BLOCK = 0x3000,
    PICTURE_WIDTH_UNITS = PICTURE_WIDTH / 64,
    /* The chip's registers, by their A+D addresses. */
    REG_PRIM = 0x00,
    REG_RGBAQ = 0x01,
    REG_UV = 0x03,
    REG_XYZ2 = 0x05,
    REG_TEX0_1 = 0x06,
    REG_TEX1_1 = 0x14,
    REG_NOP = 0x7F,
};

/* A marker in the display list (renderer::GifInterpreter::kFrameMarker). */
#define FRAME_MARKER (0x4653ull << 48)

static gaddr picture;
static int picture_lines;
static int picture_new;
static int picture_in_frame;
static int picture_of_frame; /* the picture is the frame read back, not the title's */

static uint8_t* picture_pixels(void) {
    if (picture == 0) {
        picture = openrac_guest_static(PICTURE_WIDTH * PICTURE_HEIGHT * 4);
    }
    return (uint8_t*)G(picture);
}

/* The picture to the frame buffer's pages, 32 lines at a time on the console. Nothing draws it there:
 * the splash and the no-card notice (func_0026EAC8) are the frame buffer's own contents. The window has
 * no frame buffer to write into, so the picture is kept and drawn as a sprite once the frame's set-up
 * has gone out (func_0027B858). */
long long func_002FCB00(int data, int destination, int width, int height) {
    if (width == PICTURE_WIDTH && height > 0 && height <= PICTURE_HEIGHT) {
        memcpy(picture_pixels(), G((gaddr)data), (size_t)width * (size_t)height * 4);
        picture_lines = height;
        picture_new = 1;
        picture_in_frame = 1;
        picture_of_frame = 0;
    }
    const openrac_game_image image =
        {destination >> 8, width >> 6, 0, 0, 0, width, height, (gaddr)data};
    openrac_game_load_image(&image);
    return 0;
}

/* Saves the frame: in a level, what the window last drew; in the front end the title's picture stays. */
void func_002FC8D0(gaddr a0, int a1, int a2) {
    (void)a0;
    (void)a2;
    if (openrac_guest_overlay() < 0) {
        return;
    }
    const openrac_game_image image =
        {a1 >> 8,
         PICTURE_WIDTH_UNITS,
         0,
         0,
         0,
         PICTURE_WIDTH,
         PICTURE_HEIGHT - 32,
         (picture_pixels(), picture)};
    openrac_game_store_image(&image);
    picture_lines = PICTURE_HEIGHT - 32;
    picture_new = 1;
    picture_of_frame = 1;
}

/* Writes the saved frame back: the picture, a sprite over the 416 lines of the draw buffer. */
void func_002FC990(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    if (picture_lines == 0) {
        return;
    }
    if (picture_new) {
        picture_new = 0;
        const openrac_game_image image =
            {PICTURE_BLOCK, PICTURE_WIDTH_UNITS, 0, 0, 0, PICTURE_WIDTH, picture_lines, picture};
        openrac_game_load_image(&image);
        /* It reaches the texture pool after this frame's list: drawn now, the frame shows the pool's
         * missing-texture checkers for a tick, at the client's first picture. */
        if (picture_of_frame) {
            func_002FC560(REG_NOP, FRAME_MARKER);
        }
        return;
    }
    const uint64_t tex0 = (uint64_t)(PICTURE_BLOCK) | (uint64_t)PICTURE_WIDTH_UNITS << 14
                          | 9ull << 26 | 9ull << 30 | 1ull << 35;
    const uint64_t left = 0x7000, top = 0x7300, right = left + (PICTURE_WIDTH << 4),
                   bottom = top + (416 << 4);
    func_002FC560(REG_TEX0_1, tex0);
    func_002FC560(REG_TEX1_1, 0x60);
    func_002FC560(REG_PRIM, 0x116);
    func_002FC560(REG_RGBAQ, 0x80808080u);
    func_002FC560(REG_UV, 0);
    func_002FC560(REG_XYZ2, left | top << 16);
    func_002FC560(REG_UV, (uint64_t)(PICTURE_WIDTH << 4) | (uint64_t)(picture_lines << 4) << 16);
    func_002FC560(REG_XYZ2, right | bottom << 16);
    if (picture_of_frame) {
        func_002FC560(REG_NOP, FRAME_MARKER);
    }
}

void func_0027B858__game(void);

/* The frame's set-up; then the picture the frame buffer holds, if one was just written. */
void func_0027B858(void) {
    func_0027B858__game();
    if (picture_in_frame) {
        picture_in_frame = 0;
        func_002FC990(0, 0, 0);
    }
}
