/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#include <stdlib.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
void game_FUN_L01_002b7fe0(gaddr, int);
void game_FUN_L00_002a3ec8(gaddr, int, int, float, float, float, float);
void game_FUN_L01_00262618(gaddr);
uint8_t* openrac_guest_base;
static int enabled, steps, impulses, builds;
static float captured_z, captured_w, captured_amplitude, captured_scroll;

gaddr openrac_relocate_level_data(int from, gaddr at) {
    (void)from;
    return at;
}

int openrac_game_enhanced_water(void) {
    return enabled;
}

void game_FUN_L01_002fd0c0__game(void) {}

void game_FUN_L01_002f60f8__game(void) {}

void game_FUN_L01_002f6150__game(void) {}

void game_FUN_L01_002feb28__game(void) {}

void game_FUN_L01_00309bf8__game(void) {}

void game_FUN_L01_002b7fe0__game(gaddr at, int n) {
    (void)at;
    (void)n;
    ++steps;
}

void game_FUN_L00_002a3ec8__game(
    gaddr at, int n, int mode, float x, float y, float r, float amount
) {
    (void)at;
    (void)n;
    (void)mode;
    (void)x;
    (void)y;
    (void)r;
    (void)amount;
    ++impulses;
}

void game_FUN_L01_00262618__game(gaddr layer) {
    ++builds;
    gaddr vertices = GREF(gaddr, layer);
    captured_z = GREF(float, vertices + 8);
    captured_w = GREF(float, vertices + 12);
    captured_amplitude = GREF(float, layer + 0x18);
    captured_scroll = GREF(float, layer + 0x40);
}

int main(void) {
    openrac_guest_base = calloc(1, GUEST_RAM_SIZE);
    CHECK(openrac_guest_base);
    const gaddr pool = 0x001E34C0u, layer = 0x001FBC80u, vertices = 0x300000u;
    const gaddr cursor = 0x001611C0u, packet = 0x00500000u;
    GREF(gaddr, cursor) = packet;
    memset(G(pool), 0x5A, 0x1190);
    game_FUN_L01_002b7fe0(pool, 1);
    CHECK(steps == 1);
    game_FUN_L00_002a3ec8(pool, 1, 1, 0, 0, 1, .2f);
    CHECK(impulses == 1);
    enabled = 1;
    game_FUN_L01_002b7fe0(pool, 1);
    CHECK(steps == 1);
    for (unsigned i = 0; i < 0x50; ++i) {
        CHECK(GREF(unsigned char, pool + i) == 0x5A);
    }
    for (unsigned i = 0x50; i < 0x1190; ++i) {
        CHECK(GREF(unsigned char, pool + i) == 0);
    }
    game_FUN_L00_002a3ec8(pool, 1, 1, 0, 0, 1, .2f);
    CHECK(impulses == 1);
    game_FUN_L00_002a3ec8(0x400000, 1, 1, 0, 0, 1, .2f);
    CHECK(impulses == 2);  // Other pools untouched.
    GREF(gaddr, layer) = vertices;
    GREF(unsigned, layer + 0xC) = 3;
    for (unsigned i = 0; i < 3; ++i) {
        GREF(float, vertices + i * 16 + 8) = 2;
        GREF(float, vertices + i * 16 + 12) = 4;
    }
    GREF(float, layer + 0x18) = .25f;
    GREF(float, layer + 0x40) = .125f;
    GREF(unsigned, layer + 0x50) = 77;
    game_FUN_L01_00262618(layer);
    CHECK(captured_amplitude == 0 && captured_scroll == 0);
    CHECK(GREF(float, layer + 0x18) == .25f);
    CHECK((GREF(uint64_t, packet + 32) >> 48) == 0x5755);
    CHECK(GREF(unsigned, packet + 48) == 77);
    CHECK(builds == 1 && captured_z == 3 && captured_w == 3);
    CHECK(GREF(float, layer + 0x40) == .125f);
    CHECK(
        GREF(float, vertices + 8) == 2 && GREF(float, vertices + 12) == 4
    );  // Restore original endpoints.
    enabled = 0;
    game_FUN_L01_00262618(layer);
    CHECK(captured_z == 2 && captured_w == 4);
    game_FUN_L01_002b7fe0(pool, 1);
    CHECK(steps == 2);
    game_FUN_L00_002a3ec8(pool, 1, 1, 0, 0, 1, .2f);
    CHECK(impulses == 3);
    /* Cave layers get distinct tags, retain UV scrolling, and suppress only
     * their legacy height endpoints when enhanced mode is active. */
    enabled = 1;
    GREF(gaddr, cursor) = packet;
    for (unsigned i = 0; i < 3; ++i) {
        const gaddr cave = 0x00202D80u + i * 0x60u;
        GREF(gaddr, cave) = vertices;
        GREF(unsigned, cave + 0xCu) = 3;
        game_FUN_L01_00262618(cave);
        CHECK(captured_z == 3 && captured_w == 3);
        CHECK(GREF(float, vertices + 8) == 2 && GREF(float, vertices + 12) == 4);
        CHECK((GREF(uint64_t, packet + i * 176u + 32u) & 0xffffu) == 5u + i);
        CHECK(GREF(float, cave + 0x40u) == 0.f);
    }
    game_FUN_L01_00309bf8();
    CHECK((GREF(uint64_t, packet + 3u * 176u + 32u) & 0xffffu) == 0);
    enabled = 0;
    game_FUN_L01_00262618(0x00202D80u);
    CHECK(captured_z == 2 && captured_w == 4);
    free(openrac_guest_base);
    return 0;
}
