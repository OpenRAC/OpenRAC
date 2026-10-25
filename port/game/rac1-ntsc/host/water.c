/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"

/* Novalis draw callbacks append to the level VIF chain, not the resident HUD chain.
 * The harmless GS NOP travels with that chain, including the one-tick display delay. */
static void water_marker(unsigned surface) {
    const gaddr cursor = OPENRAC_LDATA(1, 0x001611C0u);
    const gaddr at = GREF(gaddr, cursor);
    GREF(uint32_t, at) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xE;
    GREF(uint64_t, at + 32) = (0x5754ull << 48) | surface;
    GREF(uint64_t, at + 40) = 0x7F;
    GREF(gaddr, cursor) = at + 48;
}

void game_FUN_L01_002fd0c0(void) {
    water_marker(1);
    game_FUN_L01_002fd0c0__game();
    water_marker(0);
}

void game_FUN_L01_002f60f8(void) {
    water_marker(2);
    game_FUN_L01_002f60f8__game();
    water_marker(0);
}

void game_FUN_L01_002f6150(void) {
    water_marker(3);
    game_FUN_L01_002f6150__game();
    water_marker(0);
}

void game_FUN_L01_002feb28(void) {
    water_marker(4);
    game_FUN_L01_002feb28__game();
    water_marker(0);
}

/* Restrict shared helpers to the Novalis pool's tiles, including subset calls. */
static int enhanced_pool(gaddr tiles, int count) {
    const gaddr first = OPENRAC_LDATA(1, 0x001E34C0u);
    return count > 0 && count <= 21 && tiles >= first && tiles < first + 21u * 0x1190u
           && (tiles - first) % 0x1190u == 0 && (tiles - first) / 0x1190u + (unsigned)count <= 21
           && openrac_game_enhanced_water();
}

void game_FUN_L01_002b7fe0(gaddr tiles, int count) {
    if (!enhanced_pool(tiles, count)) {
        game_FUN_L01_002b7fe0__game(tiles, count);
        return;
    }
    /* All three buffers must be flat, including pre-existing waves when toggled on.
     * Keep the tile metadata, UV scrolling and draw callback active. */
    for (int i = 0; i < count; ++i) {
        memset(&GREF(unsigned char, tiles + (unsigned)i * 0x1190u + 0x50u), 0, 3u * 0x5C0u);
    }
}

void game_FUN_L00_002a3ec8(
    gaddr tiles, int count, int mode, float x, float y, float radius, float amount
) {
    if (!enhanced_pool(tiles, count)) {
        game_FUN_L00_002a3ec8__game(tiles, count, mode, x, y, radius, amount);
    }
}

/* This callback draws three overlapping cave pools at different heights. */
void game_FUN_L01_00309bf8(void) {
    game_FUN_L01_00309bf8__game();
    water_marker(0);
}

static int water_layer(gaddr layer) {
    const gaddr cave = OPENRAC_LDATA(1, 0x00202D80u);
    if (layer >= cave && layer < cave + 3u * 0x60u && (layer - cave) % 0x60u == 0) {
        return 5 + (int)((layer - cave) / 0x60u);
    }
    const gaddr bases[] = {
        OPENRAC_LDATA(1, 0x001E2FC0u), OPENRAC_LDATA(1, 0x001E3380u), OPENRAC_LDATA(1, 0x001FBC80u)
    };
    const unsigned counts[] = {8, 2, 4};
    for (unsigned i = 0; i < 3; ++i) {
        if (layer >= bases[i] && layer < bases[i] + counts[i] * 0x60u
            && (layer - bases[i]) % 0x60u == 0) {
            return 1;
        }
    }
    return 0;
}

void game_FUN_L01_00262618(gaddr layer) {
    const int surface = water_layer(layer);
    if (surface >= 5) {
        water_marker((unsigned)surface);
    }
    if (!surface || !openrac_game_enhanced_water()) {
        game_FUN_L01_00262618__game(layer);
        return;
    }
    const gaddr vertices = GREF(gaddr, layer);
    const unsigned count = GREF(unsigned, layer + 0xCu);
    if (count < 3 || count > 4096) {
        game_FUN_L01_00262618__game(layer);
        return;
    }
    /* Packet building runs synchronously on the game thread; no recursive call uses this scratch. */
    static float saved[4096 * 2];
    for (unsigned i = 0; i < count; ++i) {
        const gaddr at = vertices + i * 16u;
        saved[i * 2] = GREF(float, at + 8);
        saved[i * 2 + 1] = GREF(float, at + 12);
        const float rest = (saved[i * 2] + saved[i * 2 + 1]) * .5f;
        GREF(float, at + 8) = GREF(float, at + 12) = rest;
    }
    /* The original packet builder still supplies UVs/colour, but its two height endpoints
     * are identical. The enhanced renderer owns mesh displacement. */
    game_FUN_L01_00262618__game(layer);
    for (unsigned i = 0; i < count; ++i) {
        const gaddr at = vertices + i * 16u;
        GREF(float, at + 8) = saved[i * 2];
        GREF(float, at + 12) = saved[i * 2 + 1];
    }
}
