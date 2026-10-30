/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Novalis enhanced water on the PAL disc: rac1-ntsc/host/water.c's hooks at PAL's functions and
 * data. The functions pair by games/rac1's catalogue (config/overlays/functions.tsv): the same
 * fingerprint, or the same size and places where the NTSC decomp's differs. The data are
 * games/rac1's names (include/names.h) of the NTSC decomp's same records. */
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"

/* l01_packet_r (D_L01_00161240; NTSC 0x001611C0), the level's VIF chain cursor. */
#define PACKET OPENRAC_LDATA(1, 0x00161240u)
/* pool_tiles (NTSC 0x001E34C0): 21 tiles of 0x1190 bytes. */
#define POOL_TILES OPENRAC_LDATA(1, 0x001E3780u)
/* novalis_scroll_layers, novalis_bounded_scroll_layers, novalis_four_scroll_layers and
 * novalis_pulse_scroll_layers (NTSC 0x001E2FC0, 0x001E3380, 0x001FBC80, 0x00202D80). */
#define SCROLL_LAYERS OPENRAC_LDATA(1, 0x001E32A0u)
#define BOUNDED_LAYERS OPENRAC_LDATA(1, 0x001E3660u)
#define FOUR_LAYERS OPENRAC_LDATA(1, 0x001FBF40u)
#define CAVE_LAYERS OPENRAC_LDATA(1, 0x00203040u)

/* Novalis draw callbacks append to the level VIF chain, not the resident HUD chain.
 * The harmless GS NOP travels with that chain, including the one-tick display delay. */
static void water_marker(unsigned surface) {
    const gaddr cursor = PACKET;
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

/* Snapshot shader UV parameters into harmless NOPs beside this layer's draw packets.
 * The original layer update still owns scroll wrapping and the animation counter. */
static void water_uv_motion(gaddr layer) {
    const gaddr cursor = PACKET;
    const gaddr at = GREF(gaddr, cursor);
    const uint32_t fields[] =
        {GREF(uint32_t, layer + 0x18),
         GREF(uint32_t, layer + 0x50),
         GREF(uint32_t, layer + 0x40),
         GREF(uint32_t, layer + 0x44),
         GREF(uint32_t, layer + 0x48),
         GREF(uint32_t, layer + 0x4C)};
    GREF(uint32_t, at) = 0x10000007u;
    GREF(uint32_t, at + 4) = GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000007u;
    GREF(uint64_t, at + 16) = 6ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xE;
    for (unsigned i = 0; i < 6; ++i) {
        GREF(uint64_t, at + 32 + i * 16) = (0x5755ull << 48) | ((uint64_t)i << 32) | fields[i];
        GREF(uint64_t, at + 40 + i * 16) = 0x7F;
    }
    GREF(gaddr, cursor) = at + 128;
}

/* Moby751Draw (NTSC FUN_L01_002fd0c0): the pond. */
void func_L01_002FE498(void) {
    water_marker(1);
    func_L01_002FE498__game();
    water_marker(0);
}

/* update_and_draw_level01_scrolling_layers (NTSC FUN_L01_002f60f8): the flowing strips. */
void func_L01_002F74D0(gaddr moby) {
    water_marker(2);
    func_L01_002F74D0__game(moby);
    water_marker(0);
}

/* draw_two_scroll_layers (NTSC FUN_L01_002f6150). */
void func_L01_002F7528(gaddr moby) {
    water_marker(3);
    func_L01_002F7528__game(moby);
    water_marker(0);
}

/* draw_four_scroll_layers (NTSC FUN_L01_002feb28). */
void func_L01_002FFF00(gaddr moby) {
    water_marker(4);
    func_L01_002FFF00__game(moby);
    water_marker(0);
}

/* Restrict shared helpers to the Novalis pool's tiles, including subset calls. */
static int enhanced_pool(gaddr tiles, int count) {
    const gaddr first = POOL_TILES;
    return count > 0 && count <= 21 && tiles >= first && tiles < first + 21u * 0x1190u
           && (tiles - first) % 0x1190u == 0 && (tiles - first) / 0x1190u + (unsigned)count <= 21
           && openrac_game_enhanced_water();
}

/* RippleSimClock (NTSC FUN_L01_002b7fe0). */
void func_L01_002B9198(gaddr tiles, int count) {
    if (!enhanced_pool(tiles, count)) {
        func_L01_002B9198__game(tiles, count);
        return;
    }
    /* All three buffers must be flat, including pre-existing waves when toggled on.
     * Keep the tile metadata, UV scrolling and draw callback active. */
    for (int i = 0; i < count; ++i) {
        memset(&GREF(unsigned char, tiles + (unsigned)i * 0x1190u + 0x50u), 0, 3u * 0x5C0u);
    }
}

/* RippleDisturb (NTSC FUN_L00_002a3ec8). */
void func_L00_002A5158(
    gaddr tiles, int count, int mode, float x, float y, float radius, float amount
) {
    if (!enhanced_pool(tiles, count)) {
        func_L00_002A5158__game(tiles, count, mode, x, y, radius, amount);
    }
}

/* draw_level01_scroll_layers (NTSC FUN_L01_00309bf8): three overlapping cave pools at different
 * heights. */
void func_L01_0030AFD0(gaddr moby) {
    func_L01_0030AFD0__game(moby);
    water_marker(0);
}

static int water_layer(gaddr layer) {
    const gaddr cave = CAVE_LAYERS;
    if (layer >= cave && layer < cave + 3u * 0x60u && (layer - cave) % 0x60u == 0) {
        return 5 + (int)((layer - cave) / 0x60u);
    }
    const gaddr bases[] = {SCROLL_LAYERS, BOUNDED_LAYERS, FOUR_LAYERS};
    const unsigned counts[] = {8, 2, 4};
    for (unsigned i = 0; i < 3; ++i) {
        if (layer >= bases[i] && layer < bases[i] + counts[i] * 0x60u
            && (layer - bases[i]) % 0x60u == 0) {
            return 1;
        }
    }
    return 0;
}

/* build_overlay_layer_wave_vertices (NTSC FUN_L01_00262618). */
void func_L01_002635C8(gaddr layer) {
    const int surface = water_layer(layer);
    if (surface >= 5) {
        water_marker((unsigned)surface);
    }
    if (!surface || !openrac_game_enhanced_water()) {
        func_L01_002635C8__game(layer);
        return;
    }
    const gaddr vertices = GREF(gaddr, layer);
    const unsigned count = GREF(unsigned, layer + 0xCu);
    if (count < 3 || count > 4096) {
        func_L01_002635C8__game(layer);
        return;
    }
    /* Packet building runs synchronously on the game thread; no recursive call uses this scratch. */
    static float saved[4096 * 2];
    const float amplitude = GREF(float, layer + 0x18);
    float offsets[4];
    memcpy(offsets, G(layer + 0x40), sizeof(offsets));
    water_uv_motion(layer);
    GREF(float, layer + 0x18) = 0;
    memset(G(layer + 0x40), 0, sizeof(offsets));
    for (unsigned i = 0; i < count; ++i) {
        const gaddr at = vertices + i * 16u;
        saved[i * 2] = GREF(float, at + 8);
        saved[i * 2 + 1] = GREF(float, at + 12);
        const float rest = (saved[i * 2] + saved[i * 2 + 1]) * .5f;
        GREF(float, at + 8) = GREF(float, at + 12) = rest;
    }
    /* Capture base UVs and static heights. The GPU applies the snapshotted UV motion and
     * enhanced displacement; the original packet layout and colour remain unchanged. */
    func_L01_002635C8__game(layer);
    GREF(float, layer + 0x18) = amplitude;
    memcpy(G(layer + 0x40), offsets, sizeof(offsets));
    for (unsigned i = 0; i < count; ++i) {
        const gaddr at = vertices + i * 16u;
        GREF(float, at + 8) = saved[i * 2];
        GREF(float, at + 12) = saved[i * 2 + 1];
    }
}
