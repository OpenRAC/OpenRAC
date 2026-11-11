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
 *
 * In a picture wider than 4:3 the still pictures (the splash, the no-card notice, the title behind the
 * main menu) keep their shape in the middle, as Ratchet & Clank's loading pictures do, and the window's
 * sides, which Ratchet & Clank's title world reaches, are not left black: the picture's own edges,
 * blurred (the backdrop), are drawn out to them, darkening towards the window's sides. At 4:3 nothing
 * of it is drawn.
 *
 * The end credits (game state 9, func_L00_002E2958's copies in every level's program) write their
 * picture of the page to the frame buffer every frame, through the same routine, in the middle of
 * the frame's list: those frames have no set-up (func_0027B858) and nothing else clears the frame
 * buffer, so on the console the picture written over it is what wipes the last frame's names. In a
 * level's program the picture is drawn where the console's transfer is (draw_now).
 */
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "rac2.h"

enum {
    PICTURE_WIDTH = 512,
    PICTURE_HEIGHT = 448,
    /* A block of the texture pool no game upload uses. */
    PICTURE_BLOCK = 0x3000,
    PICTURE_WIDTH_UNITS = PICTURE_WIDTH / 64,
    /* The backdrop: the picture an eighth the size each way, blurred, in the blocks after it. Only its
     * edges are drawn (draw_sides). */
    BACKDROP_SHRINK = 8,
    BACKDROP_WIDTH = PICTURE_WIDTH / BACKDROP_SHRINK,
    BACKDROP_HEIGHT = PICTURE_HEIGHT / BACKDROP_SHRINK,
    BACKDROP_BLOCK = PICTURE_BLOCK + PICTURE_WIDTH * PICTURE_HEIGHT * 4 / 256,
    /* Its brightness, of 256: dim enough that the sides read as the picture's surround. */
    BACKDROP_LIGHT = 224,
    /* The chip's registers, by their A+D addresses. */
    REG_PRIM = 0x00,
    REG_RGBAQ = 0x01,
    REG_UV = 0x03,
    REG_XYZ2 = 0x05,
    REG_TEX0_1 = 0x06,
    REG_TEX1_1 = 0x14,
    REG_TEST_1 = 0x47,
    REG_NOP = 0x7F,
};

/* Markers in the display list (renderer::GifInterpreter::kFrameMarker, kWholeMarker, kWorldMarker). */
#define FRAME_MARKER (0x4653ull << 48)
#define WHOLE_MARKER (0x5754ull << 48)
#define WORLD_MARKER (0x574Cull << 48) /* kWorldMarker */

static gaddr picture;
static gaddr backdrop;
static int backdrop_lines;
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

/* The backdrop from the picture's `lines` rows: each 8 x 8 square averaged, then three passes of a
 * 3-tap box blur each way (about a Gaussian over the picture's 50 pixels), darkened. */
static void make_backdrop(int lines) {
    if (backdrop == 0) {
        backdrop = openrac_guest_static(BACKDROP_WIDTH * BACKDROP_HEIGHT * 4);
    }
    const uint8_t* from = picture_pixels();
    uint8_t* to = (uint8_t*)G(backdrop);
    const int rows = (lines + BACKDROP_SHRINK - 1) / BACKDROP_SHRINK;
    static int sums[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < BACKDROP_WIDTH; ++x) {
            int r = 0, g = 0, b = 0, n = 0;
            for (int py = y * BACKDROP_SHRINK; py < (y + 1) * BACKDROP_SHRINK && py < lines; ++py) {
                const uint8_t* p = from + ((size_t)py * PICTURE_WIDTH + (size_t)x * BACKDROP_SHRINK) * 4;
                for (int px = 0; px < BACKDROP_SHRINK; ++px, p += 4, ++n) {
                    r += p[0];
                    g += p[1];
                    b += p[2];
                }
            }
            sums[y][x][0] = r / n;
            sums[y][x][1] = g / n;
            sums[y][x][2] = b / n;
        }
    }
    static int pass[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    for (int round = 0; round < 3; ++round) {
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < BACKDROP_WIDTH; ++x) {
                const int l = x > 0 ? x - 1 : x, r = x + 1 < BACKDROP_WIDTH ? x + 1 : x;
                for (int c = 0; c < 3; ++c) {
                    pass[y][x][c] = (sums[y][l][c] + sums[y][x][c] * 2 + sums[y][r][c]) / 4;
                }
            }
        }
        for (int y = 0; y < rows; ++y) {
            const int u = y > 0 ? y - 1 : y, d = y + 1 < rows ? y + 1 : y;
            for (int x = 0; x < BACKDROP_WIDTH; ++x) {
                for (int c = 0; c < 3; ++c) {
                    sums[y][x][c] = (pass[u][x][c] + pass[y][x][c] * 2 + pass[d][x][c]) / 4;
                }
            }
        }
    }
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < BACKDROP_WIDTH; ++x) {
            uint8_t* p = to + ((size_t)y * BACKDROP_WIDTH + (size_t)x) * 4;
            for (int c = 0; c < 3; ++c) {
                p[c] = (uint8_t)(sums[y][x][c] * BACKDROP_LIGHT / 256);
            }
            p[3] = 0x80;
        }
    }
    backdrop_lines = rows;
}

void func_002FC990(int a0, int a1, int a2);
static void draw_now(void);

/* The picture to the frame buffer's pages, 32 lines at a time on the console. Nothing draws it there:
 * the splash and the no-card notice (func_0026EAC8) are the frame buffer's own contents. The window has
 * no frame buffer to write into, so the picture is kept and drawn as a sprite once the frame's set-up
 * has gone out (func_0027B858); the credits' picture, in a level's program, at once (draw_now). */
long long func_002FCB00(int data, int destination, int width, int height) {
    if (width == PICTURE_WIDTH && height > 0 && height <= PICTURE_HEIGHT) {
        /* The splash and the no-card notice write the same picture every frame (func_0026EA00's
         * fade): it goes to the texture pool once and is drawn from the next frame on. A picture
         * new every frame would never be drawn (func_002FC990 only sends a new one). */
        const size_t bytes = (size_t)width * (size_t)height * 4;
        const int same = !picture_of_frame && picture_lines == height
                         && memcmp(picture_pixels(), G((gaddr)data), bytes) == 0;
        memcpy(picture_pixels(), G((gaddr)data), bytes);
        picture_lines = height;
        picture_new = picture_new || !same;
        picture_in_frame = 1;
        picture_of_frame = 0;
    }
    const openrac_game_image image =
        {destination >> 8, width >> 6, 0, 0, 0, width, height, (gaddr)data};
    openrac_game_load_image(&image);
    if (picture_in_frame && openrac_guest_overlay() >= 0) {
        draw_now();
    }
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

/* One side of the backdrop, a band of the whole window from `outer` to `seam` (its 16ths of the
 * chip's pixels): the picture's blurred edge drawn out to the window's side and darkening there, its
 * texels from `edge` (at the seam) to `far` (at the window's side), 16ths of a texel. */
static void draw_side(uint64_t outer, uint64_t seam, uint64_t edge, uint64_t far, uint64_t top,
                      uint64_t bottom) {
    const uint64_t rows = (uint64_t)backdrop_lines << 4;
    const uint64_t dim = 0x80383838u, lit = 0x80808080u;
    func_002FC560(REG_RGBAQ, dim);
    func_002FC560(REG_UV, far | 8ull << 16);
    func_002FC560(REG_XYZ2, outer | top << 16);
    func_002FC560(REG_RGBAQ, lit);
    func_002FC560(REG_UV, edge | 8ull << 16);
    func_002FC560(REG_XYZ2, seam | top << 16);
    func_002FC560(REG_RGBAQ, dim);
    func_002FC560(REG_UV, far | (rows - 8) << 16);
    func_002FC560(REG_XYZ2, outer | bottom << 16);
    func_002FC560(REG_RGBAQ, lit);
    func_002FC560(REG_UV, edge | (rows - 8) << 16);
    func_002FC560(REG_XYZ2, seam | bottom << 16);
}

/* The window's sides past the picture, the window `wider` times the picture's width: on each side the
 * backdrop's two texels at that edge (the picture's outer 16 columns, blurred) drawn out to the
 * window's side, as the whole window's (the marker), before the picture. */
static void draw_sides(float wider, uint64_t top, uint64_t bottom) {
    const uint64_t left = 0x7000, right = left + (PICTURE_WIDTH << 4);
    /* The picture's sides in the whole window, in 16ths of the chip's pixels. */
    const uint64_t band = (uint64_t)((float)(PICTURE_WIDTH << 3) * (1.0f - 1.0f / wider));
    const uint64_t texels = (uint64_t)BACKDROP_WIDTH << 4;
    const uint64_t tex0 = (uint64_t)(BACKDROP_BLOCK) | (uint64_t)(BACKDROP_WIDTH / 64) << 14 | 6ull << 26
                          | 6ull << 30;  /* modulated (TFX 0): the vertex colours darken it outwards */
    func_002FC560(REG_NOP, WHOLE_MARKER | 1);
    func_002FC560(REG_TEX0_1, tex0);
    func_002FC560(REG_TEX1_1, 0x60);
    func_002FC560(REG_PRIM, 0x11C); /* a Gouraud strip, textured, texel coordinates */
    draw_side(left, left + band, 8, 8 + 2 * 16, top, bottom);
    func_002FC560(REG_PRIM, 0x11C);
    draw_side(right, right - band, texels - 8, texels - 8 - 2 * 16, top, bottom);
    func_002FC560(REG_NOP, WHOLE_MARKER);
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
        /* A frame read back already covers the whole window (the marker). */
        if (!picture_of_frame) {
            make_backdrop(picture_lines);
            const openrac_game_image under =
                {BACKDROP_BLOCK, BACKDROP_WIDTH / 64, 0, 0, 0, BACKDROP_WIDTH, backdrop_lines, backdrop};
            openrac_game_load_image(&under);
        }
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
    const float wider = openrac_game_picture_wider();
    if (!picture_of_frame && backdrop_lines > 0 && wider > 1.001f) {
        draw_sides(wider, top, bottom);
    }
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

/* The credits' picture where the console's transfer is in the list, under the names drawn after it.
 * A level's tick that draws no world goes over the picture shown last (frontend/tick_picture.h,
 * Overdraw), and only the part of its list after the world's marker is drawn over it: the marker goes
 * first, so the picture covers the last tick's names as the console's transfer does. Nothing in these
 * frames sets the chip's TEST before the picture (the transfer needs none); the last tick's left the
 * depth test at GREATER, which a sprite at depth 0 fails, so the picture's own goes with it: depth
 * always passes, no alpha test. The names set theirs. A new picture reaches the texture pool after
 * this tick's list, so its first tick shows the page before's (the first page's, under the credits'
 * fade from black). */
static void draw_now(void) {
    picture_in_frame = 0;
    if (picture_new) {
        func_002FC990(0, 0, 0); /* to the texture pool */
    }
    func_002FC560(REG_NOP, WORLD_MARKER);
    func_002FC560(REG_TEST_1, 0x30000); /* ZTE on, ZTST ALWAYS */
    func_002FC990(0, 0, 0);
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
