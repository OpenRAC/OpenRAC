/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/game_particle_allocate.c"
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static unsigned char storage[2048 * 64];
unsigned char* particle_pool = storage;
unsigned char particle_bits[512];
int particle_next, particle_high, particle_count;

static void reset(void) {
    memset(storage, 0xA5, sizeof(storage));
    memset(particle_bits, 0, 256);
    memset(particle_bits + 256, 0xCC, 256);
    particle_next = particle_high = particle_count = 0;
}

static void check_record(unsigned char* p, int type) {
    int j;
    CHECK(p[0] == (unsigned char)type);
    for (j = 1; j < 32; ++j) {
        CHECK(p[j] == 0);
    }
    for (j = 32; j < 64; ++j) {
        CHECK(p[j] == 0xA5);
    }
}

int main(void) {
    int i;
    unsigned char* p;
    reset();
    for (i = 0; i < 2048; ++i) {
        p = func_00218928(i + 256);
        CHECK(p == storage + i * 64);
        check_record(p, i + 256);
        CHECK(particle_next == i + 1 && particle_high == i && particle_count == i + 1);
    }
    CHECK(func_00218928(1) == NULL && func_00218930(2, 1) == NULL);
    CHECK(particle_count == 2048 && particle_next == 2048 && particle_high == 2047);
    for (i = 0; i < 256; ++i) {
        CHECK(particle_bits[i] == 255 && particle_bits[i + 256] == 0xCC);
    }

    /* Sparse reuse below the high-water mark skips occupied bytes, then
     * allocates each hole without lowering that mark or touching protection. */
    reset();
    memset(particle_bits, 255, 256);
    particle_bits[0] &= (unsigned char)~(1u << 3);
    particle_bits[4] &= (unsigned char)~(1u << 2);
    particle_bits[255] &= (unsigned char)~(1u << 7);
    particle_next = 3;
    particle_high = 2047;
    particle_count = 2045;
    p = func_00218930(7, 123);
    CHECK(p == storage + 3 * 64 && particle_next == 34 && particle_high == 2047);
    check_record(p, 7);
    CHECK(func_00218930(8, -1) == storage + 34 * 64 && particle_next == 2047);
    CHECK(func_00218928(9) == storage + 2047 * 64 && particle_next == 2048);
    CHECK(func_00218928(10) == NULL && particle_count == 2048);
    for (i = 256; i < 512; ++i) {
        CHECK(particle_bits[i] == 0xCC);
    }

    /* The byte-aligned scan may find an earlier hole. Preserve that ordering. */
    reset();
    particle_next = 5;
    particle_high = 10;
    particle_bits[0] = 0x1B; /* 0,1,3,4 occupied; bit 2 is free */
    CHECK(func_00218928(4) == storage + 5 * 64 && particle_next == 2);
    CHECK(particle_bits[0] == 0x3B && particle_high == 10 && particle_count == 1);
    puts("particle allocation tests passed");
    return 0;
}
