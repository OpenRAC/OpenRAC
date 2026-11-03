/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
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

#include "game/rac1/host/water_bomb.h"
#include "game_protos.h"
#include "openrac/game_host.h"
void game_FUN_L01_002b7fe0(gaddr, int);
void game_FUN_L00_002a3ec8(gaddr, int, int, float, float, float, float);
void game_FUN_L01_00262618(gaddr);
uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE; /* the console's 32 MiB arena */
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

static unsigned explosion_events, original_explosions;
static gaddr explosion_owner, explosion_position;
static float explosion_point[3], explosion_radius, explosion_strength;
void game_FUN_L00_0025e450(
    gaddr,
    gaddr,
    gaddr,
    float,
    float,
    int,
    int,
    int,
    float,
    float,
    float,
    float,
    int,
    float,
    int,
    int,
    int,
    unsigned char
);

void openrac_game_water_explosion(const float point[3], float radius, float strength) {
    ++explosion_events;
    memcpy(explosion_point, point, sizeof(explosion_point));
    explosion_radius = radius;
    explosion_strength = strength;
}

void game_FUN_L00_0025e450__game(
    gaddr owner,
    gaddr unused,
    gaddr position,
    float a,
    float b,
    int burst,
    int sparks,
    int shards,
    float c,
    float d,
    float e,
    float scale,
    int sound,
    float flash,
    int shake,
    int spray,
    int reduction,
    unsigned char flag
) {
    ++original_explosions;
    CHECK(owner == explosion_owner && position == explosion_position && unused == 0x987u);
    CHECK(a == 2 && b == 3 && burst == 4 && sparks == 5 && shards == 6);
    CHECK(c == 7 && d == 8 && e == 9 && scale == 1.25f && sound == 11 && flash == 12);
    CHECK(shake == 13 && spray == 14 && reduction == -1 && flag == 1);
}

static void check_explosion_capture(void) {
    explosion_owner = 0x1000;
    explosion_position = 0x2000;
    for (unsigned axis = 0; axis < 3; ++axis) {
        GREF(float, explosion_position + axis * 4) = (float)axis + 10.f;
        GREF(float, explosion_owner + 0x10u + axis * 4) = (float)axis + 20.f;
    }
    game_FUN_L00_0025e450(
        explosion_owner,
        0x987u,
        explosion_position,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        1.25f,
        11,
        12,
        13,
        14,
        -1,
        1
    );
    CHECK(original_explosions == 1 && explosion_events == 1);
    CHECK(explosion_point[0] == 10 && explosion_point[2] == 12);
    CHECK(explosion_radius == 2 && explosion_strength == 1);
    explosion_position = 0;
    game_FUN_L00_0025e450(
        explosion_owner, 0x987u, 0, 2, 3, 4, 5, 6, 7, 8, 9, 1.25f, 11, 12, 13, 14, -1, 1
    );
    CHECK(original_explosions == 2 && explosion_events == 2 && explosion_point[0] == 20);
    explosion_owner = 0;
    game_FUN_L00_0025e450(0, 0x987u, 0, 2, 3, 4, 5, 6, 7, 8, 9, 1.25f, 11, 12, 13, 14, -1, 1);
    CHECK(
        original_explosions == 3 && explosion_events == 2
    );  // Original still runs with no position.
}

/* Real class/state/byte transitions from Bomb Glove update, including the
 * immediate underwater deletion and the later expanding-ring ticks. */
static void check_bomb_glove_capture(void) {
    const gaddr moby = 0x3000;
    const unsigned before_events = explosion_events;
    GREF(int16_t, moby + 0xA6u) = 121;
    GREF(uint8_t, moby + 0x20u) = 0;
    GREF(uint8_t, moby + 0xBCu) = 0;
    CHECK(!rac1_water_bomb_pending(moby));  // Held, not thrown.
    GREF(uint8_t, moby + 0x20u) = 1;
    int pending = rac1_water_bomb_pending(moby);
    CHECK(pending);
    rac1_water_bomb_after(moby, pending);
    CHECK(explosion_events == before_events);  // Ordinary flight never emits a blast.
    for (unsigned axis = 0; axis < 3; ++axis) {
        GREF(float, moby + 0x10u + 4u * axis) = 30.f + (float)axis;
    }
    GREF(uint8_t, moby + 0x20u) = 2;
    GREF(uint8_t, moby + 0xBCu) = 2;
    unsigned char unchanged[0x100];
    memcpy(unchanged, G(moby), sizeof unchanged);
    rac1_water_bomb_after(moby, pending);
    CHECK(memcmp(unchanged, G(moby), sizeof unchanged) == 0);  // Read-only observation.
    CHECK(
        explosion_events == before_events + 1 && explosion_point[0] == 30
        && explosion_point[2] == 32
    );
    CHECK(explosion_radius == 3 && explosion_strength == 1.6f);
    for (unsigned tick = 0; tick < 15; ++tick) {
        pending = rac1_water_bomb_pending(moby);
        GREF(uint8_t, moby + 0xBCu)++;
        rac1_water_bomb_after(moby, pending);
    }
    CHECK(explosion_events == before_events + 1);  // No blast per expanding-ring frame.
    GREF(uint8_t, moby + 0x20u) = 1;
    GREF(uint8_t, moby + 0xBCu) = 0;
    pending = rac1_water_bomb_pending(moby);
    GREF(uint8_t, moby + 0x20u) = 0xFE;
    GREF(uint8_t, moby + 0xBCu) = 1;
    rac1_water_bomb_after(moby, pending);
    CHECK(explosion_events == before_events + 2);  // Underwater: deleted inside the update.
    GREF(uint8_t, moby + 0x20u) = 1;
    GREF(uint8_t, moby + 0xBCu) = 0;
    pending = rac1_water_bomb_pending(moby);
    GREF(uint8_t, moby + 0x20u) = 0xFE;
    rac1_water_bomb_after(moby, pending);
    CHECK(explosion_events == before_events + 2);  // An ordinary deletion is not an explosion.
    GREF(int16_t, moby + 0xA6u) = 0;
    GREF(uint8_t, moby + 0x20u) = 1;
    CHECK(!rac1_water_bomb_pending(moby));
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
    check_explosion_capture();
    check_bomb_glove_capture();
    free(openrac_guest_base);
    return 0;
}
