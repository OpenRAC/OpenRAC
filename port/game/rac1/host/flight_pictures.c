/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * An added planet's own picture and caption on the flight there. The flight's lump (the transition
 * WAD, which initialize_level_runtime decompresses and uploads) holds one planet picture per level
 * and one caption per language and level, found through its texture table. On the flight to an
 * added destination, the template level's entries point at the destination's own images instead,
 * built after the lump's end in the lump's own format; the game reads them as it reads its own. */
#include <stdio.h>
#include <string.h>

#include "rac1.h"

#define PICTURE_SIZE 128
#define CAPTION_WIDTH 256
#define CAPTION_HEIGHT 32
#define PALETTE_BYTES 0x400
#define FIRST_CAPTION 0x14 /* the table's first caption: after its count and 19 pictures */
#define LEVELS 19
#define PSM_8BIT 0x13

/* A texture of the lump (upload_mip_texture's MipTexture): 8 bytes of tag, width, height, pixel
 * format, palette format, a word, the level count, then the palette and the pixels. */
#define TEXTURE_HEAD 0x20

static int is_texture(gaddr at, int width, int height) {
    return GREF(int, at + 8) == width && GREF(int, at + 12) == height
           && GREF(int, at + 16) == PSM_8BIT && GREF(int, at + 20) == 0 && GREF(int, at + 28) == 1;
}

/* The CLUT's place for colour index i: the GS reads an 8-bit texture's 256 colours with entries
 * 8 to 15 and 16 to 23 of each 32 swapped. */
static int clut_place(int i) {
    return (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
}

/* PNG at PATH as a texture of WIDTH x HEIGHT at AT, in the lump's format (TAG its first 8 bytes):
 * at most 256 colours, fewer bits per channel until they fit. Returns its size, 0 when the
 * file is missing or another size. */
static int build(const char* path, int width, int height, gaddr tag, gaddr at) {
    int w = 0, h = 0;
    unsigned char* rgba = openrac_game_load_png(path, &w, &h);
    if (rgba == NULL) {
        return 0;
    }
    if (w != width || h != height) {
        fprintf(
            stderr,
            "[warning] %s is %dx%d, not %dx%d: the flight keeps the template's\n",
            path,
            w,
            h,
            width,
            height
        );
        openrac_game_free_png(rgba);
        return 0;
    }
    const int count = width * height;
    uint32_t palette[256];
    int colours = 0;
    for (int shift = 0; shift < 8; ++shift) {
        const unsigned mask = (0xFFu << shift) & 0xFFu;
        colours = 0;
        int i = 0;
        for (; i < count; ++i) {
            const unsigned char* p = rgba + i * 4;
            const uint32_t c = (p[0] & mask) | (p[1] & mask) << 8 | (p[2] & mask) << 16
                               | (uint32_t)((p[3] * 128 + 127) / 255) << 24;
            int k = 0;
            while (k < colours && palette[k] != c) {
                ++k;
            }
            if (k == colours) {
                if (colours == 256) {
                    break;
                }
                palette[colours++] = c;
            }
            GREF(unsigned char, at + TEXTURE_HEAD + PALETTE_BYTES + i) = (unsigned char)k;
        }
        if (i == count) {
            break;
        }
    }
    openrac_game_free_png(rgba);
    memcpy(G(at), G(tag), 8);
    GREF(int, at + 8) = width;
    GREF(int, at + 12) = height;
    GREF(int, at + 16) = PSM_8BIT;
    GREF(int, at + 20) = 0;
    GREF(int, at + 24) = 0;
    GREF(int, at + 28) = 1;
    for (int i = 0; i < 256; ++i) {
        GREF(uint32_t, at + TEXTURE_HEAD + clut_place(i) * 4) = i < colours ? palette[i] : 0;
    }
    return (TEXTURE_HEAD + PALETTE_BYTES + count + 15) & ~15;
}

int rac1_flight_pictures(gaddr to, uint32_t made) {
    const int d = openrac_game_active_destination;
    if (d < 0 || (unsigned)d >= openrac_game_destination_count || openrac_game_mods_dir == NULL) {
        return 0;
    }
    const int level = openrac_game_destinations[d].level;
    if (level < 0 || level >= LEVELS || made < 0x80) {
        return 0;
    }
    /* The flight's lump, and no other WAD: its texture table (header +0x4C, from the data at
     * header +4) counts its entries first, and the template's picture and caption are the lump's
     * 128x128 and 256x32 textures. */
    const uint32_t data = GREF(uint32_t, to + 4);
    const uint32_t table_at = data + GREF(uint32_t, to + 0x4C);
    if (data >= made || table_at >= made - 4) {
        return 0;
    }
    const gaddr table = to + table_at;
    const int entries = GREF(int, table);
    if (entries < FIRST_CAPTION + level || entries > 1000 || table_at + 4u * (entries + 1) > made) {
        return 0;
    }
    const gaddr picture = table + GREF(int, table + (level + 1) * 4);
    const gaddr caption = table + GREF(int, table + (FIRST_CAPTION + level) * 4);
    if (picture <= table || picture >= to + made || caption <= table || caption >= to + made
        || !is_texture(picture, PICTURE_SIZE, PICTURE_SIZE)
        || !is_texture(caption, CAPTION_WIDTH, CAPTION_HEIGHT)) {
        return 0;
    }
    const gaddr start = (to + made + 15) & ~15u;
    const uint32_t room = PICTURE_SIZE * PICTURE_SIZE + CAPTION_WIDTH * CAPTION_HEIGHT
                          + 2 * (TEXTURE_HEAD + PALETTE_BYTES) + 32;
    if (start + room > openrac_guest_ram_size) {
        return 0;
    }
    char path[1024];
    gaddr at = start;
    snprintf(
        path,
        sizeof(path),
        "%s/level_%02d/textures/flight_picture.png",
        openrac_game_mods_dir,
        level
    );
    int size = build(path, PICTURE_SIZE, PICTURE_SIZE, picture, at);
    if (size != 0) {
        GREF(int, table + (level + 1) * 4) = (int)(at - table);
        at += size;
    }
    snprintf(
        path,
        sizeof(path),
        "%s/level_%02d/textures/flight_caption.png",
        openrac_game_mods_dir,
        level
    );
    size = build(path, CAPTION_WIDTH, CAPTION_HEIGHT, caption, at);
    if (size != 0) {
        /* One caption for every language the lump has. */
        for (int index = FIRST_CAPTION + level; index <= entries; index += LEVELS) {
            GREF(int, table + index * 4) = (int)(at - table);
        }
        at += size;
    }
    return (int)(at - (to + made));
}
