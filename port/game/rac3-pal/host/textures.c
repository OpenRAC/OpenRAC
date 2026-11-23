/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal's images, identified by the game's own tables rather than by where the console's
 * graphics chip kept them (docs/port/TEXTURES.md).
 *
 * The HUD bank (the boot program's header at *0x001D9EA0, its tables at *0x001D9EA8 entries,
 * *0x001D9EAC images and *0x001D9EB0 palettes; Veldin's at *0x001DFD18..) keeps each image as {data,
 * u16 chip address, u8 log2 width, u8 log2 height}. Group 0 of the bank is sent once, when the bank
 * is loaded (func_00393548, frontbin's func_00393168, the levels' func_L01_0041C648), each image by
 * SendImage (func_00397D60, frontbin's func_00397980, the levels' func_L01_004215C0): (data, chip
 * address, format, log2 width, log2 height, at once). The boot program decompresses that group into
 * a buffer it reuses (0x00227580's +0x10, plus 0x30000), so once it is sent the table's pointers hold
 * whatever comes next (the menus' corner ornaments at 0x005F1D00 read back as one colour).
 *
 * The port keeps what SendImage was given, as it was then (openrac_game_image_keep): an image the
 * 2D layer draws from that address is what the game sent. SendImage itself still runs for the
 * routines that hand the window TEX0 records, until they are native.
 */
#include "textures.h"

#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

int rac3_hud_image(gaddr tables, int n, openrac_game_ui_image* out) {
    if (tables == 0 || n < 0 || n > 0xFFFF) {
        return 0;
    }
    const gaddr entries = GREF(gaddr, tables);
    const gaddr images = GREF(gaddr, tables + 4u);
    const gaddr palettes = GREF(gaddr, tables + 8u);
    if (entries == 0 || images == 0 || palettes == 0) {
        return 0;
    }
    const gaddr entry = entries + 4u * (gaddr)n;
    const gaddr image = images + 8u * (gaddr)GREF(uint16_t, entry + 2u);
    const gaddr palette = palettes + 8u * (gaddr)GREF(uint16_t, entry);
    const unsigned log2_width = GREF(uint8_t, image + 6u);
    const unsigned log2_height = GREF(uint8_t, image + 7u);
    const gaddr pixels = GREF(gaddr, image);
    const gaddr colours = GREF(gaddr, palette);
    if (pixels == 0 || colours == 0 || log2_width > 10 || log2_height > 10) {
        return 0;
    }
    out->pixels = pixels;
    out->palette = colours;
    out->width = (uint16_t)(1u << log2_width);
    out->height = (uint16_t)(1u << log2_height);
    out->bits = 8;
    out->swizzled = 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

void func_00397D60__game(
    int data, int address, int format, int log2_width, int log2_height, int now
);
void func_00397980__frontbin__game(
    int data, int address, int format, int log2_width, int log2_height, int now
);
int func_L01_004215C0__game(
    int data, int address, int format, int log2_width, int log2_height, int now
);

/* Bits a pixel of SendImage's formats: 32-bit, 24-bit, 16-bit colours; 8-bit indices (also the
 * ones kept in the top byte of 32-bit words); 4-bit indices (likewise in bits 24-27, 28-31). */
static uint32_t bits_of(int format) {
    switch (format) {
        case 0x00:
            return 32;
        case 0x01:
            return 24;
        case 0x02:
        case 0x0A:
            return 16;
        case 0x13:
        case 0x1B:
            return 8;
        case 0x14:
        case 0x24:
        case 0x2C:
            return 4;
        default:
            return 0;
    }
}

/* Keeps the image SendImage is about to send: (1 << log2_width) x (1 << log2_height) pixels. */
static void keep_sent_image(int data, int format, int log2_width, int log2_height) {
    const uint32_t bits = bits_of(format);
    if (data == 0 || bits == 0 || log2_width < 0 || log2_width > 10 || log2_height < 0
        || log2_height > 10) {
        return;
    }
    const uint32_t pixels = (1u << log2_width) << log2_height;
    openrac_game_image_keep((gaddr)data, pixels * bits / 8u);
}

void func_00397D60(int data, int address, int format, int log2_width, int log2_height, int now) {
    keep_sent_image(data, format, log2_width, log2_height);
    func_00397D60__game(data, address, format, log2_width, log2_height, now);
}

void func_00397980__frontbin(
    int data, int address, int format, int log2_width, int log2_height, int now
) {
    keep_sent_image(data, format, log2_width, log2_height);
    func_00397980__frontbin__game(data, address, format, log2_width, log2_height, now);
}

int func_L01_004215C0(int data, int address, int format, int log2_width, int log2_height, int now) {
    keep_sent_image(data, format, log2_width, log2_height);
    return func_L01_004215C0__game(data, address, format, log2_width, log2_height, now);
}
