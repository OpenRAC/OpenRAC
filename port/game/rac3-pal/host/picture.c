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
 * routine runs. */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

long long func_003A9BE8__game(int pixels, int frame, int width, int height);

/* The display list's cursor, which the routine writes its transfers at. */
#define LIST_CURSOR 0x001DA050u
/* The front end's register packet (A+D): XYOFFSET_1 at +0x50, the offset the menu's 2D is drawn
 * with (0x7000, 0x7200 in 12.4). */
#define FRONT_END_REGS 0x00142120u

/* The block the window keeps the picture at: one no game texture can start at in that shape (a
 * 512-wide picture from 0x3F00 would run past the GS's 4 MB), so it is never an upload of the
 * game's; the frame's own block (0x1E00) is where the game draws the frame. */
#define PICTURE_BLOCK 0x3F00

/* The picture the window holds: its block, a hash of its pixels, and whether a frame used it. */
static int s_block = -1;
static uint32_t s_hash;
static int s_used;

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

uint64_t func_003A9BE8(int pixels, int frame, int width, int height) {
    if (width <= 0 || height <= 0 || width > 1024 || height > 1024) {
        return (uint64_t)func_003A9BE8__game(pixels, frame, width, height);
    }
    const uint64_t tex0 = picture_tex0((uint32_t)frame, width, height);
    const uint32_t ofx = GREF(uint32_t, FRONT_END_REGS + 0x50u) & 0xFFFFu;
    const uint32_t ofy = GREF(uint32_t, FRONT_END_REGS + 0x54u) & 0xFFFFu;
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof q);
    openrac_ui_rect(
        &q,
        (int)ofx,
        (int)ofy,
        (int)ofx + (width << 4),
        (int)ofy + (height << 4),
        0,
        0,
        width << 4,
        height << 4
    );
    q.rgba = 0x80808080u;
    q.tex0 = (tex0 & ~0x3FFFull) | PICTURE_BLOCK;
    q.flags = 2u | 16u;       /* a sprite, clamped */
    q.clamp = 1u | (1u << 2); /* CLAMP_1: clamp both ways */
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        return (uint64_t)func_003A9BE8__game(pixels, frame, width, height);
    }
    const uint32_t bytes = (uint32_t)width * (uint32_t)height * 4u;
    const uint8_t* rgba = (const uint8_t*)G((gaddr)pixels & 0x01FFFFFFu);
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
    return tex0;
}

/* Each frame's display list as it is sent (host/dma.c): a picture no draw of this frame asked for
 * is forgotten, so that the GS memory at its block is what the game puts there again. */
void rac3_picture_frame_sent(void) {
    if (s_block >= 0 && !s_used) {
        openrac_game_forget_picture(s_block);
        s_block = -1;
    }
    s_used = 0;
}
