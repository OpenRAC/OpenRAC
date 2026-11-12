/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Render the real host wrapper into a draw recorder, without a game or assets. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_protos.h"
#include "openrac/game_host.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

static uint32_t memory[0x200000 / 4];
static unsigned char enabled;
static int original_calls, original_cap, count;
static const gaddr element = 0x1000;
struct Draw { int frame, x, y, w, h, alpha, flipped, u, v, uw, vh; };
static struct Draw draws[9];

void* test_memory(gaddr address) {
    CHECK(address < sizeof(memory));
    return (unsigned char*)memory + address;
}
gaddr test_level_data(int level, gaddr address) {
    CHECK(level == 0);
    return address + 0x10000; /* Deliberately not the source level's address. */
}
const unsigned char* openrac_game_fix(const char* key) {
    CHECK(strcmp(key, "fix_nanotech_hud_panel") == 0);
    return &enabled;
}
int game_FUN_001ff960(int group, int frame) {
    CHECK(group == 0x7580);
    return frame;
}
unsigned long long game_FUN_001ffa10(int frame) {
    CHECK(frame == 0 || frame == 1);
    return (5ull << 26) | (5ull << 30) | (unsigned)frame;
}
void game_FUN_001f5450(int x, int y, int w, int h, int u, int v, int uw, int vh,
                     long long colour, long long texture) {
    CHECK(count < 9);
    draws[count++] = (struct Draw){(int)(texture & 1), x, y, w, h,
        (int)((colour >> 24) & 255), uw < 0, u, v, uw, vh};
}
int game_FUN_L00_00239810__game(gaddr at) {
    CHECK(at == element);
    ++original_calls;
    original_cap = GREF(int, OPENRAC_LDATA(0, 0x15F81C));
    return 73;
}
static void setup(int capacity) {
    memset(memory, 0, sizeof(memory));
    count = original_calls = 0;
    GREF(int, element + 0x50) = 256;
    GREF(unsigned char, element + 0x70) = 8;
    GREF(int, 0x15EDA0) = capacity;
    GREF(int, 0x1415F8) = 3; /* Missing health must not shrink the capacity's panel. */
    GREF(int, OPENRAC_LDATA(0, 0x15F7F0)) = 8;
    GREF(int, OPENRAC_LDATA(0, 0x15F808)) = 14;
    GREF(int, OPENRAC_LDATA(0, 0x15F810)) = -16;
    GREF(int, OPENRAC_LDATA(0, 0x15F81C)) = 32;
    GREF(int, OPENRAC_LDATA(0, 0x15F820)) = 20;
}
static void preserved(void) {
    CHECK(original_calls == 1);
    CHECK(GREF(int, OPENRAC_LDATA(0, 0x15F81C)) == 32);
    CHECK(GREF(int, 0x1415F8) == 3);
}
int main(void) {
    for (int capacity = 4; capacity <= 8; ++capacity) {
        if (capacity != 4 && capacity != 5 && capacity != 8) continue;
        setup(capacity); enabled = 0;
        CHECK(game_FUN_L00_00239810(element) == 73);
        CHECK(count == 0 && original_cap == 32); preserved();
        setup(capacity); enabled = 1;
        CHECK(game_FUN_L00_00239810(element) == 73);
        CHECK(count == 9 && original_cap == 0); preserved();
        const int half = capacity == 5 ? 32 : 48;
        const int height = (capacity == 4 ? 32 : 64) + 8;
        CHECK(draws[0].frame == 0 && draws[0].x == 256 - half);
        CHECK(draws[0].w == 2 * half && draws[0].h == 16);
        CHECK(draws[0].y == 12 && draws[0].v == 0 && draws[0].vh == 16);
        CHECK(draws[1].y == 28 && draws[1].h == height - 32);
        CHECK(draws[1].v == 16 && draws[1].vh == 1);
        CHECK(draws[2].y == 12 + height - 16 && draws[2].h == 16);
        CHECK(draws[2].v == 16 && draws[2].vh == 16);
        int area = 0;
        for (int i = 0; i < 3; ++i) {
            CHECK(draws[i].frame == 0 && draws[i].x == 256 - half && draws[i].w == 2 * half);
            area += draws[i].w * draws[i].h;
        }
        for (int right = 0; right < 2; ++right) {
            const int at = 3 + 3 * right;
            const int edge = right ? 256 + half : 256 - half - 32;
            CHECK(draws[at].y == 12 && draws[at].h == 16);
            CHECK(draws[at].v == 0 && draws[at].vh == 16);
            CHECK(draws[at+1].y == 28 && draws[at+1].h == height - 32);
            CHECK(draws[at+1].v == 16 && draws[at+1].vh == 1);
            CHECK(draws[at+2].y == 12 + height - 16 && draws[at+2].h == 16);
            CHECK(draws[at+2].v == 16 && draws[at+2].vh == 16);
            for (int i = at; i < at + 3; ++i) {
                CHECK(draws[i].frame == 1 && draws[i].x == edge && draws[i].w == 32);
                CHECK(draws[i].flipped == right && draws[i].alpha == 20);
                CHECK(draws[i].u == (right ? 32 : 0));
                CHECK(draws[i].uw == (right ? -32 : 32));
                area += draws[i].w * draws[i].h;
            }
        }
        /* The nine non-overlapping pieces fill one padded rectangle. */
        CHECK(area == (2 * half + 64) * height);

    }
    setup(5); enabled = 1; GREF(unsigned char, element + 0x70) = 4;
    CHECK(game_FUN_L00_00239810(element) == 73);
    CHECK(count == 9 && draws[0].w == 32 && draws[0].alpha == 10); preserved();
    setup(5); GREF(unsigned char, element + 0x70) = 0;
    CHECK(game_FUN_L00_00239810(element) == 73 && count == 0 && original_cap == 32);
    setup(7); CHECK(game_FUN_L00_00239810(element) == 73 && count == 0);
    setup(8); GREF(unsigned char, 0x13F350 + 0x20A4) = 2;
    CHECK(game_FUN_L00_00239810(element) == 73 && count == 0);
    setup(5); enabled = 0;
    CHECK(game_FUN_L00_00239810(element) == 73 && count == 0 && original_cap == 32);
    return 0;
}
