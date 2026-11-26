/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal's adapter to the shared 2D layer (port/ui, openrac/ui.h): where its 2D screen,
 * its fonts and their glyph pages are in the game's memory. Read from the game's own data, nothing
 * of the console's graphics chip:
 *
 * - the 2D screen: the game adds an offset to every 2D position (12.4; the boot program's at
 *   0x001D4BD0, the multiplayer core's at 0x00240310) that centres its screen on 2048, so the screen
 *   is 2 x (2048 - offset) pixels each way (512 x 448 on PAL). The multiplayer executable's screen
 *   is its frame block's size (0x00228580: +0x150 width, +0x152 height), which its SetScissor
 *   (0x00299218) keeps the scissor within and its screen set-ups (0x0027D190, 0x0027DA40) centre
 *   the offset by. Its split-screen maps' view layout (level 39's 0x00588A08, each map's copy)
 *   moves the offset to each view's top left and cuts the scissor to the view, so its 2D is
 *   placed from the whole screen's offset, not the view's (seen in game code);
 * - the fonts: two font headers (the boot program's table at 0x001D55D0, which every
 *   single-player program's copy of FontUpload and FontSelect reads; the multiplayer core's at
 *   0x00241458, which its level 39 and maps read too; the font selected at +0x18 of either). A
 *   header holds, for each of its glyph pages p, the indices of its image and
 *   palette (+0x10 + 2p, +0x11 + 2p), the images' formats (+0x40[j]: 0x14 4-bit, else 8-bit), log2
 *   sizes (+0x50[j]: two halfwords) and offsets from the header (+0xD0[j]), and the palettes'
 *   formats (+0x30[k]: 0x14 16 colours, else 256 in the stored order) and offsets (+0x90[k]); the
 *   font's print looks a glyph's page up so (func_0038EBC8, before func_0038A0E0).
 */
#include "ui_adapter.h"

#include <stdint.h>

#include "exec.h"
#include "openrac/guest.h"
#include "openrac/ui.h"
#include "rac3_version.h"

/* The frame block's halfwords: the screen's width and height in pixels. */
#define FRAME_WIDTH 0x150u
#define FRAME_HEIGHT 0x152u
/* The middle of the chip's 4096 x 4096 drawing window, where the 2D offset centres the screen. */
#define CHIP_MIDDLE 0x800
/* The sizes a 2D screen can have; outside them the game has not set its screen up yet. */
#define SCREEN_MIN 64
#define SCREEN_MAX 1024

gaddr rac3_ui_offset(void) {
    /* The boot program's, which every single-player program shares, or the multiplayer core's,
     * which its level 39 and maps share (seen in game code). */
    return rac3_exec_program() == RAC3_EXEC_MULTIPLAYER ? RAC3_MADDR(0x00240310u)
                                                        : RAC3_ADDR(0x001D4BD0u);
}

gaddr rac3_ui_frame(void) {
    /* The boot program's (its SetScissor func_003A98B8) or the multiplayer core's (0x00299218),
     * which level 39 and the maps read too (seen in game code). */
    return rac3_exec_program() == RAC3_EXEC_MULTIPLAYER ? RAC3_MADDR(0x00228580u)
                                                        : RAC3_ADDR(0x001CFEC0u);
}

/* The multiplayer executable's whole screen, its frame block's size: 0 before the game sets it. */
static int mp_screen(int* width, int* height) {
    /* The single-player programs' offset is their whole screen's: no views to undo. */
    if (rac3_exec_program() != RAC3_EXEC_MULTIPLAYER) {
        return 0;
    }
    *width = GREF(int16_t, rac3_ui_frame() + FRAME_WIDTH);
    *height = GREF(int16_t, rac3_ui_frame() + FRAME_HEIGHT);
    return *width >= SCREEN_MIN && *width <= SCREEN_MAX && *height >= SCREEN_MIN
           && *height <= SCREEN_MAX;
}

void rac3_ui_origin(int* x, int* y) {
    int width, height;
    /* The whole screen's offset as the core's screen set-ups write it (seen in game code:
     * 0x0027D190, 0x0027DA40): 2048 less half the screen, in sixteenths. */
    if (mp_screen(&width, &height)) {
        *x = (CHIP_MIDDLE - (width >> 1)) << 4;
        *y = (CHIP_MIDDLE - (height >> 1)) << 4;
        return;
    }
    *x = GREF(int32_t, rac3_ui_offset());
    *y = GREF(int32_t, rac3_ui_offset() + 4u);
}

void rac3_ui_view_corner(float* x, float* y) {
    int ox, oy;
    rac3_ui_origin(&ox, &oy);
    /* From 12.4 to pixels. */
    *x = (float)(GREF(int32_t, rac3_ui_offset()) - ox) / 16.0f;
    *y = (float)(GREF(int32_t, rac3_ui_offset() + 4u) - oy) / 16.0f;
}

/* The running program's font header table (two fonts): the boot program's, which every
 * single-player program reads, or the multiplayer core's, which level 39 and the maps read. */
static gaddr font_headers(void) {
    return rac3_exec_program() == RAC3_EXEC_MULTIPLAYER ? RAC3_MADDR(0x00241458u)
                                                        : RAC3_ADDR(0x001D55D0u);
}

int rac3_ui_font_index(void) {
    return GREF(
        int32_t,
        rac3_exec_program() == RAC3_EXEC_MULTIPLAYER ? RAC3_MADDR(0x00241470u)
                                                     : RAC3_ADDR(0x001D55E8u)
    );
}

static void screen(float* width, float* height) {
    int w, h;
    /* The multiplayer executable's views each draw part of the whole screen its 2D spans. */
    if (mp_screen(&w, &h)) {
        *width = (float)w;
        *height = (float)h;
        return;
    }
    /* The screen's left and top as the game offsets its 2D (to_screen_x(0) is that less 8). */
    const float ofx = (float)GREF(int32_t, rac3_ui_offset()) / 16.0f;
    const float ofy = (float)GREF(int32_t, rac3_ui_offset() + 4u) / 16.0f;
    *width = 2.0f * (2048.0f - ofx);
    *height = 2.0f * (2048.0f - ofy);
    if (*width < 64.0f || *width > 1024.0f || *height < 64.0f || *height > 1024.0f) {
        *width = 512.0f;
        *height = 448.0f;
    }
}

int rac3_ui_font_page(int font, int page, openrac_game_ui_image* out) {
    if (font < 0 || font > 1 || page < 0 || page > 15) {
        return 0;
    }
    const gaddr table = font_headers();
    const gaddr header = table != 0 ? GREF(gaddr, table + 4u * (gaddr)font) : 0;
    if (header == 0) {
        return 0;
    }
    const unsigned images = GREF(uint8_t, header + 1u);
    const unsigned palettes = GREF(uint8_t, header + 0u);
    const unsigned j = GREF(uint8_t, header + 0x10u + 2u * (gaddr)page);
    const unsigned k = GREF(uint8_t, header + 0x11u + 2u * (gaddr)page);
    if (j >= images || k >= palettes || j > 15 || k > 15) {
        return 0;
    }
    const int log2w = GREF(int16_t, header + 0x50u + 4u * j);
    const int log2h = GREF(int16_t, header + 0x52u + 4u * j);
    if (log2w < 0 || log2w > 10 || log2h < 0 || log2h > 10) {
        return 0;
    }
    out->pixels = header + (gaddr)GREF(int32_t, header + 0xD0u + 4u * j);
    out->palette = header + (gaddr)GREF(int32_t, header + 0x90u + 4u * k);
    out->width = (uint16_t)(1u << log2w);
    out->height = (uint16_t)(1u << log2h);
    out->bits = GREF(uint8_t, header + 0x40u + j) == 0x14 ? 4 : 8;
    out->swizzled = GREF(uint8_t, header + 0x30u + k) == 0x14 ? 0 : 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

int rac3_ui_font_page_of(
    int font,
    uint32_t image_handle,
    uint32_t palette_handle,
    uint32_t mask,
    gaddr images,
    gaddr palettes
) {
    if (font < 0 || font > 1) {
        return -1;
    }
    const gaddr table = font_headers();
    const gaddr header = table != 0 ? GREF(gaddr, table + 4u * (gaddr)font) : 0;
    if (header == 0) {
        return -1;
    }
    const unsigned image_count = GREF(uint8_t, header + 1u);
    const unsigned palette_count = GREF(uint8_t, header + 0u);
    for (int page = 0; page < 16; ++page) {
        const unsigned j = GREF(uint8_t, header + 0x10u + 2u * (gaddr)page);
        const unsigned k = GREF(uint8_t, header + 0x11u + 2u * (gaddr)page);
        if (j >= image_count || k >= palette_count || j > 15 || k > 15) {
            continue;
        }
        if ((GREF(uint32_t, images + 4u * j) & mask) == image_handle
            && (GREF(uint32_t, palettes + 4u * k) & mask) == palette_handle) {
            return page;
        }
    }
    return -1;
}

static int font_page(int font, int page, openrac_game_ui_image* out) {
    return rac3_ui_font_page(font, page, out);
}

static const openrac_ui_adapter kAdapter = {screen, font_page, NULL};

void rac3_ui_adapter_install(void) {
    static int installed = 0;
    if (!installed) {
        installed = 1;
        openrac_ui_set_adapter(&kAdapter);
    }
}
