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
#include "rac3_version.h"

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

/*
 * Texture handles back to images (rac3_texture_image). The getters, per program copy (rac3-decomp
 * src/boot_elf/text/389978.c, 396E18.c; IDENTIFIED.tsv):
 *
 * - HudTextureTex0 (0x00396E18, frontbin's 0x00396A38, the levels' func_L01_00420080): HUD texture n
 *   of the HUD bank (rac3_hud_image). Its handle holds the image entry's and the palette entry's
 *   stored places (the u16 at +4 of each), which the reverse lookup matches.
 * - EffectTex0 (0x00389978, frontbin's 0x00389598, the levels' func_L01_00404040): effect texture n
 *   of the scene's table (0x00226000, Veldin's 0x001E7C80: 16 bytes each, +8 the pixels' and +0xA
 *   the palette's offsets in 64-byte units from the word at 0x001D9C04 (Veldin's 0x001DF984), +0xC
 *   and +0xD the log2 width and height, +0xE the format: 0x13 8-bit indices with a 256-colour
 *   palette in the stored order, 0x14 4-bit with 16 colours), the count at 0x001D9C00. Its negative
 *   numbers are the frame buffer's, no image.
 * - PictureTex0 (0x0039B088, frontbin's 0x0039ACA8, the levels' func_L01_0042D898): a picture by its
 *   header (+8 width, +0xC height, +0x20 the 256-colour palette, +0x420 the 8-bit pixels).
 *
 * A handle is a new allocation each pass (the getters' allocator starts again at 0x00389620), so the
 * last image kept for a handle is the one it names now. Kept by the handle's fields that place and
 * size it (its low 34 bits but for the row width and its palette's place): what the draw routines
 * are given back.
 */
#define HANDLE_KEY_MASK (0x3FFFull | (0x3Full << 20) | (0xFFull << 26) | (0x3FFFull << 37))
#define HANDLE_LOW_MASK (0x3FFFull | (0x3Full << 20) | (0x3Full << 26))
#define KEPT_HANDLES 512

static struct {
    unsigned long long key;
    openrac_game_ui_image image;
} s_handles[KEPT_HANDLES];
static int s_next_handle;

static void keep_handle(unsigned long long handle, const openrac_game_ui_image* image) {
    const unsigned long long key = handle & HANDLE_KEY_MASK;
    if (key == 0) {
        return;
    }
    for (int i = 0; i < KEPT_HANDLES; ++i) {
        if (s_handles[i].key == key) {
            s_handles[i].image = *image;
            return;
        }
    }
    s_handles[s_next_handle].key = key;
    s_handles[s_next_handle].image = *image;
    s_next_handle = (s_next_handle + 1) % KEPT_HANDLES;
}

/* The HUD bank's entry whose stored places are the handle's (the image's at its +4, the palette's
 * at its +4): the bank's header (8 bytes before `tables`) counts its palettes at +0x24 and its images
 * at +0x44. */
static int hud_image_of(unsigned long long handle, gaddr tables, openrac_game_ui_image* out) {
    if (tables == 0) {
        return 0;
    }
    const gaddr header = GREF(gaddr, tables - 8u);
    const gaddr images = GREF(gaddr, tables + 4u);
    const gaddr palettes = GREF(gaddr, tables + 8u);
    if (header == 0 || images == 0 || palettes == 0) {
        return 0;
    }
    const unsigned image_count = GREF(uint16_t, header + 0x44u);
    const unsigned palette_count = GREF(uint16_t, header + 0x24u);
    const unsigned tbp = (unsigned)(handle & 0x3FFFu);
    const unsigned cbp = (unsigned)((handle >> 37) & 0x3FFFu);
    const int cut = (handle >> 32) == 0; /* cut to its low word: no palette's place */
    gaddr image = 0;
    for (unsigned i = 0; i < image_count && i < 4096u; ++i) {
        const gaddr e = images + 8u * i;
        if (GREF(uint16_t, e + 4u) == tbp && tbp != 0) {
            image = e;
            break;
        }
    }
    gaddr palette = 0;
    for (unsigned i = 0; image != 0 && !cut && i < palette_count && i < 4096u; ++i) {
        const gaddr e = palettes + 8u * i;
        if (GREF(uint16_t, e + 4u) == cbp) {
            palette = e;
            break;
        }
    }
    if (image == 0 || palette == 0) {
        return 0;
    }
    const unsigned log2_width = GREF(uint8_t, image + 6u);
    const unsigned log2_height = GREF(uint8_t, image + 7u);
    if (GREF(gaddr, image) == 0 || GREF(gaddr, palette) == 0 || log2_width > 10
        || log2_height > 10) {
        return 0;
    }
    out->pixels = GREF(gaddr, image);
    out->palette = GREF(gaddr, palette);
    out->width = (uint16_t)(1u << log2_width);
    out->height = (uint16_t)(1u << log2_height);
    out->bits = 8;
    out->swizzled = 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

int rac3_texture_image(unsigned long long handle, gaddr hud_tables, openrac_game_ui_image* out) {
    const unsigned long long key = handle & HANDLE_KEY_MASK;
    if (key != 0) {
        for (int i = 0; i < KEPT_HANDLES; ++i) {
            if (s_handles[i].key == key) {
                *out = s_handles[i].image;
                return 1;
            }
        }
        if ((handle >> 32) == 0) {
            /* Cut to its low word: matched by that, when one image alone has it. */
            int found = -1;
            for (int i = 0; i < KEPT_HANDLES; ++i) {
                if (s_handles[i].key != 0
                    && (s_handles[i].key & HANDLE_LOW_MASK) == (key & HANDLE_LOW_MASK)) {
                    if (found >= 0) {
                        found = -2;
                        break;
                    }
                    found = i;
                }
            }
            if (found >= 0) {
                *out = s_handles[found].image;
                return 1;
            }
        }
    }
    return hud_image_of(handle, hud_tables, out);
}

/* Effect texture n of the table at `table`, its count and base at `count` and `count + 4`. */
static int effect_image(gaddr table, gaddr count, int n, openrac_game_ui_image* out) {
    const int32_t entries = GREF(int32_t, count);
    if (n < 0 || (entries > 0 && n >= entries) || n > 4095) {
        return 0;
    }
    const gaddr e = table + 16u * (gaddr)n;
    const gaddr base = GREF(gaddr, count + 4u);
    const unsigned log2_width = GREF(uint8_t, e + 0xCu);
    const unsigned log2_height = GREF(uint8_t, e + 0xDu);
    const int format = GREF(int16_t, e + 0xEu);
    if (base == 0 || log2_width > 10 || log2_height > 10 || (format != 0x13 && format != 0x14)) {
        return 0;
    }
    out->pixels = base + 64u * (gaddr)GREF(uint16_t, e + 8u);
    out->palette = base + 64u * (gaddr)GREF(uint16_t, e + 0xAu);
    out->width = (uint16_t)(1u << log2_width);
    out->height = (uint16_t)(1u << log2_height);
    out->bits = format == 0x14 ? 4 : 8;
    out->swizzled = format == 0x14 ? 0 : 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

/* A picture by its header, its sides powers of two (as the getter's sizes are). */
static int picture_image(gaddr header, openrac_game_ui_image* out) {
    if (header == 0 || header >= 0x02000000u) {
        return 0;
    }
    const uint32_t width = GREF(uint32_t, header + 8u);
    const uint32_t height = GREF(uint32_t, header + 0xCu);
    if (width == 0 || height == 0 || width > 1024 || height > 1024 || (width & (width - 1)) != 0
        || (height & (height - 1)) != 0) {
        return 0;
    }
    out->pixels = header + 0x420u;
    out->palette = header + 0x20u;
    out->width = (uint16_t)width;
    out->height = (uint16_t)height;
    out->bits = 8;
    out->swizzled = 1;
    out->reserved[0] = out->reserved[1] = 0;
    return 1;
}

static void keep_hud(unsigned long long handle, gaddr tables, int n) {
    openrac_game_ui_image image;
    if (rac3_hud_image(tables, n, &image)) {
        keep_handle(handle, &image);
    }
}

static void keep_effect(unsigned long long handle, gaddr table, gaddr count, int n) {
    openrac_game_ui_image image;
    if (effect_image(table, count, n, &image)) {
        keep_handle(handle, &image);
    }
}

static void keep_picture(unsigned long long handle, gaddr header) {
    openrac_game_ui_image image;
    if (picture_image(header, &image)) {
        keep_handle(handle, &image);
    }
}

#define BOOT_HUD_TABLES RAC3_ADDR(0x001D9EA8u)
#define BOOT_EFFECTS RAC3_ADDR(0x00226000u)
#define BOOT_EFFECT_COUNT RAC3_ADDR(0x001D9C00u)
#define LEVEL_HUD_TABLES OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DFD20u))
#define LEVEL_EFFECTS OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E7C80u))
#define LEVEL_EFFECT_COUNT OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DF980u))

unsigned long long func_00396E18__game(int n);
unsigned long long func_00396A38__frontbin__game(int n);
unsigned long long func_L01_00420080__game(int n);
long long func_00389978__game(int n);
long long func_00389598__frontbin__game(int n);
long long func_L01_00404040__game(int n);
unsigned long long func_0039B088__game(int header);
long long func_0039ACA8__frontbin__game(gaddr header);
unsigned long long func_L01_0042D898__game(int header);

unsigned long long func_00396E18(int n) {
    const unsigned long long handle = func_00396E18__game(n);
    keep_hud(handle, BOOT_HUD_TABLES, n);
    return handle;
}

unsigned long long func_00396A38__frontbin(int n) {
    const unsigned long long handle = func_00396A38__frontbin__game(n);
    keep_hud(handle, BOOT_HUD_TABLES, n);
    return handle;
}

unsigned long long func_L01_00420080(int n) {
    const unsigned long long handle = func_L01_00420080__game(n);
    keep_hud(handle, LEVEL_HUD_TABLES, n);
    return handle;
}

long long func_00389978(int n) {
    const long long handle = func_00389978__game(n);
    keep_effect((unsigned long long)handle, BOOT_EFFECTS, BOOT_EFFECT_COUNT, n);
    return handle;
}

long long func_00389598__frontbin(int n) {
    const long long handle = func_00389598__frontbin__game(n);
    keep_effect((unsigned long long)handle, BOOT_EFFECTS, BOOT_EFFECT_COUNT, n);
    return handle;
}

long long func_L01_00404040(int n) {
    const long long handle = func_L01_00404040__game(n);
    keep_effect((unsigned long long)handle, LEVEL_EFFECTS, LEVEL_EFFECT_COUNT, n);
    return handle;
}

unsigned long long func_0039B088(int header) {
    const unsigned long long handle = func_0039B088__game(header);
    keep_picture(handle, (gaddr)header);
    return handle;
}

long long func_0039ACA8__frontbin(gaddr header) {
    const long long handle = func_0039ACA8__frontbin__game(header);
    keep_picture((unsigned long long)handle, header);
    return handle;
}

unsigned long long func_L01_0042D898(int header) {
    const unsigned long long handle = func_L01_0042D898__game(header);
    keep_picture(handle, (gaddr)header);
    return handle;
}
