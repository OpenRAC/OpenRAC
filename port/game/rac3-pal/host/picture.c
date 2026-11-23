/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's full-screen pictures, drawn by the port. func_003A9BE8 (pixels, the frame's
 * address in bytes, width, height) puts a picture from EE memory straight into the frame buffer as
 * 512x32 image transfers and returns its TEX0; the main menu calls it each frame with its
 * background (512x416 at 0x00EA5010, unpacked once at boot from the front end's data: a still
 * picture, not a video) before it draws the menu over it. Here the picture goes to the window as a
 * texture of its own (openrac_game_load_picture, kept at PICTURE_BLOCK and refilled only when its
 * pixels change) and the frame gets a full-screen 2D record reading it, in the display list where
 * the transfers were, as the 2D layer's quads do (lib/ui_quad.h); the game's darkening and boxes
 * are drawn over it as before. With no window to draw it (or the 2D layer replayed) the game's own
 * routine runs.
 *
 * In a picture wider than 4:3 (the player's Picture setting) the record keeps its shape in the
 * middle, as the rest of the 2D layer does, and the window's sides are filled as Going Commando's
 * are behind its title (rac2/host/picture.c): the picture's own edges, shrunk and blurred (the
 * backdrop, a second texture), drawn out to the window's sides and darkening towards them, between
 * two kWholeMarker markers so that they span the whole window. At 4:3 nothing of it is drawn. */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"
#include "ui2d.h"

long long func_003A9BE8__game(int pixels, int frame, int width, int height);
long long func_L01_004C61A0__game(int pixels, int frame, int width, int height);
long long func_003A9830__frontbin__game(int pixels, int frame, int width, int height);

/* The display list's cursor the boot program's routine writes its transfers at (a level's copy
 * writes the level's: ui2d.h). */
#define LIST_CURSOR RAC3_ADDR(0x001DA050u)
/* The front end's register packet (A+D): XYOFFSET_1 at +0x50, the offset the menu's 2D is drawn
 * with (0x7000, 0x7200 in 12.4). */
#define FRONT_END_REGS RAC3_ADDR(0x00142120u)

/* The block the window keeps the picture at: one no game texture can start at in that shape (a
 * 512-wide picture from 0x3F00 would run past the GS's 4 MB), so it is never an upload of the
 * game's; the frame's own block (0x1E00) is where the game draws the frame. */
#define PICTURE_BLOCK 0x3F00

/* The backdrop: the picture an eighth the size each way, blurred and darkened (BACKDROP_LIGHT of
 * 256), kept at a block of its own past the picture's, which no game texture can start at either
 * (64 x 64 texels from 0x3FE0 would run past the GS's 4 MB). */
#define BACKDROP_BLOCK 0x3FE0
#define BACKDROP_SHRINK 8
#define BACKDROP_WIDTH (512 / BACKDROP_SHRINK)
#define BACKDROP_HEIGHT (448 / BACKDROP_SHRINK)
#define BACKDROP_LIGHT 224

/* Markers in the display list (renderer::GifInterpreter::kWholeMarker): between `| 1` and `| 0`
 * the draws span the whole window instead of keeping their shape in its middle. */
#define WHOLE_MARKER (0x5754ull << 48)

/* The picture the window holds: its block, a hash of its pixels, and whether a frame used it. */
static int s_block = -1;
static uint32_t s_hash;
static int s_used;

/* The backdrop the window holds, made from the picture whose hash it was (s_backdrop_hash), and its
 * rows. */
static int s_backdrop_rows;
static uint32_t s_backdrop_hash;
static uint8_t s_backdrop[BACKDROP_WIDTH * BACKDROP_HEIGHT * 4];

static uint32_t picture_hash(const uint8_t* p, uint32_t bytes) {
    uint32_t h = 2166136261u;
    for (uint32_t k = 0; k < bytes; k += 4u * 61u) {
        uint32_t w;
        memcpy(&w, p + k, sizeof w);
        h = (h ^ w) * 16777619u;
    }
    return h;
}

/* The TEX0 the routine returns: the frame's block, the width in 64-pixel units, PSMCT32, and the
 * picture's size as powers of two (rounded up). */
static uint64_t picture_tex0(uint32_t frame, int width, int height) {
    uint64_t tw = 0, th = 0;
    while ((1 << tw) < width) {
        tw++;
    }
    while ((1 << th) < height) {
        th++;
    }
    return (uint64_t)((frame >> 8) & 0x3FFFu) | ((uint64_t)((width >> 6) & 0x3F) << 14) | (tw << 26)
           | (th << 30) | (1ull << 34);
}

/* The backdrop from a 512-wide picture's `lines` rows, as Going Commando's (rac2/host/picture.c,
 * make_backdrop): each 8 x 8 square averaged, then three passes of a 3-tap box blur each way (about
 * a Gaussian over the picture's 50 pixels), darkened. Its rows. */
static int make_backdrop(const uint8_t* from, int lines) {
    static int sums[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    static int pass[BACKDROP_HEIGHT][BACKDROP_WIDTH][3];
    const int rows = (lines + BACKDROP_SHRINK - 1) / BACKDROP_SHRINK;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < BACKDROP_WIDTH; ++x) {
            int r = 0, g = 0, b = 0, n = 0;
            for (int py = y * BACKDROP_SHRINK; py < (y + 1) * BACKDROP_SHRINK && py < lines; ++py) {
                const uint8_t* p = from + ((size_t)py * 512u + (size_t)x * BACKDROP_SHRINK) * 4u;
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
            uint8_t* p = s_backdrop + ((size_t)y * BACKDROP_WIDTH + (size_t)x) * 4u;
            for (int c = 0; c < 3; ++c) {
                p[c] = (uint8_t)(sums[y][x][c] * BACKDROP_LIGHT / 256);
            }
            p[3] = 0x80;
        }
    }
    return rows;
}

/* A marker at the list's cursor: an A+D write of `value` to the NOP register, as openrac_ui_send
 * writes a record's. */
static void put_marker(gaddr cursor, uint64_t value) {
    const gaddr at = GREF(gaddr, cursor);
    GREF(uint32_t, at + 0) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xEull;
    GREF(uint64_t, at + 32) = value;
    GREF(uint64_t, at + 40) = 0x7Full;
    GREF(gaddr, cursor) = at + 48;
}

/* One side of the backdrop, a band of the whole window from `outer` to `seam` (16ths of the chip's
 * pixels): the backdrop's texels from `far` (at the window's side, dim) to `edge` (at the
 * picture's side, lit), 16ths of a texel, shaded between them. */
static void draw_side(gaddr cursor, int outer, int seam, int far, int edge, int top, int bottom) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof q);
    openrac_ui_rect(&q, outer, top, seam, bottom, far, 8, edge, (s_backdrop_rows << 4) - 8);
    q.tex0 = (uint64_t)BACKDROP_BLOCK | ((uint64_t)(BACKDROP_WIDTH / 64) << 14) | (6ull << 26)
             | (6ull << 30) | (1ull << 34);
    q.flags = 4u | 16u | 32u; /* a colour a corner, clamped, shaded */
    q.clamp = 1u | (1u << 2);
    q.corners[0] = q.corners[2] = 0x80383838u; /* the window's side */
    q.corners[1] = q.corners[3] = 0x80808080u; /* the picture's */
    openrac_ui_send(&q, cursor);
}

/* The window's sides past the picture (x0 to x1, y0 to y1), the window `wider` times the
 * picture's width: on each side the backdrop's outer texels drawn out to the window's side, as the
 * whole window's. In whole-window coordinates the picture's own 512 pixels span the window, so each
 * side is (1 - 1 / wider) / 2 of them. */
static void draw_sides(gaddr cursor, float wider, int x0, int x1, int y0, int y1) {
    const int band = (int)((float)((x1 - x0) / 2) * (1.0f - 1.0f / wider));
    const int texels = BACKDROP_WIDTH << 4;
    put_marker(cursor, WHOLE_MARKER | 1u);
    draw_side(cursor, x0, x0 + band, 8 + 2 * 16, 8, y0, y1);
    draw_side(cursor, x1, x1 - band, texels - 8 - 2 * 16, texels - 8, y0, y1);
    put_marker(cursor, WHOLE_MARKER);
}

/* The picture as a record in the list whose cursor is at `cursor` (and, in a wider picture, its
 * backdrop's sides), its TEX0 in *tex0_out. False when it is not one the port draws, or the 2D layer
 * is replayed: the caller runs the game's routine. */
static int draw_picture_rgba(
    gaddr cursor, const uint8_t* rgba, int frame, int width, int height, uint64_t* tex0_out
);

static int draw_picture(
    gaddr cursor, int pixels, int frame, int width, int height, uint64_t* tex0_out
) {
    if (width <= 0 || height <= 0 || width > 1024 || height > 1024) {
        return 0;
    }
    return draw_picture_rgba(
        cursor, (const uint8_t*)G((gaddr)pixels & 0x01FFFFFFu), frame, width, height, tex0_out
    );
}

/* The picture from `rgba` (the port's memory or game memory) as the frame's record. */
static int draw_picture_rgba(
    gaddr cursor, const uint8_t* rgba, int frame, int width, int height, uint64_t* tex0_out
) {
    if (width <= 0 || height <= 0 || width > 1024 || height > 1024) {
        return 0;
    }
    const uint64_t tex0 = picture_tex0((uint32_t)frame, width, height);
    const uint32_t ofx = GREF(uint32_t, FRONT_END_REGS + 0x50u) & 0xFFFFu;
    const uint32_t ofy = GREF(uint32_t, FRONT_END_REGS + 0x54u) & 0xFFFFu;
    /* A full-width picture fills the frame's lines: the frame is centred on 2048, so it is twice
     * (2048 - the offset) lines tall. The main menu's background is 416 lines, the NTSC frame's;
     * PAL's frame is 448, and the picture drawn at its own size left a 32-line black band at the
     * bottom (as the PAL disc itself does). Stretched over the frame it has the NTSC disc's shape
     * on both. */
    const int lines = (int)((0x8000u - ofy) >> 3);
    const int shown = width == 512 && height < lines && lines <= 512 ? lines : height;
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof q);
    openrac_ui_rect(
        &q,
        (int)ofx,
        (int)ofy,
        (int)ofx + (width << 4),
        (int)ofy + (shown << 4),
        0,
        0,
        width << 4,
        height << 4
    );
    q.rgba = 0x80808080u;
    q.tex0 = (tex0 & ~0x3FFFull) | PICTURE_BLOCK;
    q.flags = 2u | 16u;       /* a sprite, clamped */
    q.clamp = 1u | (1u << 2); /* CLAMP_1: clamp both ways */
    if (!openrac_ui_send(&q, cursor)) {
        return 0;
    }
    const uint32_t bytes = (uint32_t)width * (uint32_t)height * 4u;
    const uint32_t hash = picture_hash(rgba, bytes);
    const int block = PICTURE_BLOCK;
    if (block != s_block || hash != s_hash) {
        const openrac_game_image image = {block, width >> 6, 0, 0, 0, width, height, 0};
        if (s_block >= 0 && s_block != block) {
            openrac_game_forget_picture(s_block);
        }
        openrac_game_load_picture(&image, rgba);
        s_block = block;
        s_hash = hash;
    }
    s_used = 1;

    const float wider = openrac_game_picture_wider();
    if (wider > 1.001f && width == 512 && height <= BACKDROP_HEIGHT * BACKDROP_SHRINK) {
        if (s_backdrop_rows == 0 || s_backdrop_hash != hash) {
            s_backdrop_rows = make_backdrop(rgba, height);
            s_backdrop_hash = hash;
            const openrac_game_image under =
                {BACKDROP_BLOCK, 1, 0, 0, 0, BACKDROP_WIDTH, s_backdrop_rows, 0};
            openrac_game_load_picture(&under, s_backdrop);
        }
        draw_sides(cursor, wider, (int)q.x[0], (int)q.x[3], (int)q.y[0], (int)q.y[3]);
    }
    *tex0_out = tex0;
    return 1;
}

uint64_t func_003A9BE8(int pixels, int frame, int width, int height) {
    uint64_t tex0;
    if (!draw_picture(LIST_CURSOR, pixels, frame, width, height, &tex0)) {
        return (uint64_t)func_003A9BE8__game(pixels, frame, width, height);
    }
    return tex0;
}

/* The levels' copy (Veldin's 0x004C61A0, frontbin's row func_003A9830: the same instructions but for
 * the list's cursor): a picture a level puts in its frame, the video screens of Holostar's studio
 * (levels 13 and 14) among them, which drew as solid blue boxes from the game's transfers. */
long long func_L01_004C61A0(int pixels, int frame, int width, int height) {
    uint64_t tex0;
    if (!draw_picture(rac3_ui2d_level().cursor, pixels, frame, width, height, &tex0)) {
        return func_L01_004C61A0__game(pixels, frame, width, height);
    }
    return (long long)tex0;
}

/* frontbin's copy (0x003A9830, the catalogue's row of the levels' copy): the main menu's background
 * after a level (Quit Game), on the boot program's list cursor, which frontbin shares. */
long long func_003A9830__frontbin(int pixels, int frame, int width, int height) {
    uint64_t tex0;
    if (!draw_picture(LIST_CURSOR, pixels, frame, width, height, &tex0)) {
        return func_003A9830__frontbin__game(pixels, frame, width, height);
    }
    return (long long)tex0;
}

/* A movie's frame (movie.c): the native player's picture (RGBA rows) as the frame's record in the
 * list at `cursor`, as the main menu's background is. */
int rac3_picture_record(gaddr cursor, const uint8_t* rgba, int width, int height) {
    uint64_t tex0;
    return draw_picture_rgba(cursor, rgba, 0, width, height, &tex0);
}

/* Each frame's display list as it is sent (host/dma.c): a picture no draw of this frame asked for
 * is forgotten, so that the GS memory at its block is what the game puts there again. */
void rac3_picture_frame_sent(void) {
    if (s_block >= 0 && !s_used) {
        openrac_game_forget_picture(s_block);
        s_block = -1;
        if (s_backdrop_rows != 0) {
            openrac_game_forget_picture(BACKDROP_BLOCK);
            s_backdrop_rows = 0;
        }
    }
    s_used = 0;
}

/* The frame under a level's pause menu, as Going Commando's (rac2/host/picture.c, func_002FC8D0 and
 * func_002FC990, whose instructions Veldin's func_L01_004C5F70 and func_L01_004C6030 are). When
 * the pause menu opens, the game draws the world once more and reads the frame buffer back to main
 * memory (`to`), 64 x 64 tiles of PSMCT24 from its pages (`frame`: the buffer's address in the
 * chip's words, `bytes` its size, the lines times 2048); every frame after it writes those tiles
 * back over the buffer as image transfers in the display list (func_L01_00403C88), darkens them
 * (func_L01_004059C8) and draws the menu over them. The window has no chip memory to read or
 * write: it keeps the frame it drew, whole and at its own resolution and shape
 * (openrac_game_store_image, frontend/images.cpp), and a marker in the list
 * (renderer::GifInterpreter::kFrameMarker) draws that frame where the tiles would have landed, over
 * the whole window, so the world under the menu is as sharp and as wide as it was. Nothing of the
 * game's transfers is sent. */
#define FRAME_MARKER (0x4653ull << 48)
/* The statics region is 1 MB in all, so the buffer is the game's own frame at most (448 lines), not
 * a whole megabyte. */
#define FRAME_LINES_MOST 448

void func_L01_004C5F70__game(int to, int frame, int bytes);

void func_L01_004C5F70(int to, int frame, int bytes) {
    static gaddr pixels;
    int lines = bytes >> 11;
    if (lines <= 0 || lines > FRAME_LINES_MOST) {
        func_L01_004C5F70__game(to, frame, bytes);
        return;
    }
    (void)to;
    if (pixels == 0) {
        pixels = openrac_guest_static(512 * FRAME_LINES_MOST * 4);
    }
    /* The frame read back, at the chip's 512 x lines: the window keeps its own whole picture as it
     * answers (the snapshot the marker draws). */
    const openrac_game_image image = {frame >> 8, 512 / 64, 0, 0, 0, 512, lines, pixels};
    openrac_game_store_image(&image);
}

void func_L01_004C6030(int from, int frame, int bytes) {
    (void)from;
    (void)frame;
    (void)bytes;
    rac3_ui2d_marker(rac3_ui2d_level().cursor, FRAME_MARKER);
}
