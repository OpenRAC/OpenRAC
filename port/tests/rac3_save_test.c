/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Up Your Arsenal's save read without the game (rac3-pal/host/save_format.h), on a save the game
 * wrote: the launcher's fixture, slot 0 as the PAL game writes it when it makes its folder on a
 * fresh card (an empty slot, level -1; launcher/docs/RAC3_SAVE_FORMAT.md), then made a save of
 * Veldin, and broken the ways a save from elsewhere can be. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "save_format.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static uint8_t save[RAC3_SAVE_SIZE + 16];

static void put_u32(uint8_t* at, uint32_t v) {
    memcpy(at, &v, sizeof v);
}

/* The global section's checksum made again, as SavePack does. */
static void sum_global(void) {
    put_u32(save + 12, rac3_save_crc16(save + 16, rac3_save_u32(save + 8)));
}

int main(void) {
    FILE* f = fopen(RAC3_SAVE_FIXTURE, "rb");
    CHECK(f != NULL);
    const size_t size = fread(save, 1, sizeof save, f);
    fclose(f);
    CHECK(size == RAC3_SAVE_SIZE);
    CHECK(size == 134992);

    /* The game's own file: whole, every checksum as the game makes it, but an empty slot. */
    CHECK(rac3_save_whole(save, (uint32_t)size));
    CHECK(rac3_save_crc16(save + 16, 14208) == rac3_save_u32(save + 12));
    int32_t level = 0, bolts = -1, seconds = -1;
    CHECK(!rac3_save_info(save, (uint32_t)size, &level, &bolts, &seconds));
    CHECK(level == -1);

    /* The item rows (chunk 30): each item's own row in a new game. */
    uint32_t bytes = 0;
    const uint8_t* rows = rac3_save_chunk(save + 8, RAC3_SAVE_GLOBAL_SIZE, 30, &bytes);
    CHECK(rows != NULL && bytes == 160);
    CHECK(rows[0] == 0 && rows[9] == 9 && rows[155] == 155);

    /* A save of Veldin with 12,345 bolts and an hour played. */
    uint8_t* chunk = (uint8_t*)rac3_save_chunk(save + 8, RAC3_SAVE_GLOBAL_SIZE, 0, &bytes);
    CHECK(chunk != NULL && bytes == 4);
    put_u32(chunk, 1);
    chunk = (uint8_t*)rac3_save_chunk(save + 8, RAC3_SAVE_GLOBAL_SIZE, 39, &bytes);
    CHECK(chunk != NULL && bytes == 128);
    put_u32(chunk, 12345);
    chunk = (uint8_t*)rac3_save_chunk(save + 8, RAC3_SAVE_GLOBAL_SIZE, 3, &bytes);
    CHECK(chunk != NULL && bytes == 4);
    put_u32(chunk, 216000);
    CHECK(!rac3_save_whole(save, (uint32_t)size)); /* its checksum is the old one */
    sum_global();
    CHECK(rac3_save_whole(save, (uint32_t)size));
    CHECK(rac3_save_info(save, (uint32_t)size, &level, &bolts, &seconds));
    CHECK(level == 1 && bolts == 12345 && seconds == 3600);

    /* Not the whole file, or more of it. */
    CHECK(!rac3_save_whole(save, (uint32_t)size - 4));
    CHECK(!rac3_save_info(save, (uint32_t)size + 4, &level, &bolts, &seconds));

    /* A byte of the last level section changed: its checksum fails, so the save is refused whole,
     * though its head still reads. */
    uint8_t* last_level = save + 8 + RAC3_SAVE_GLOBAL_SIZE + 36 * RAC3_SAVE_LEVEL_SIZE;
    last_level[20] ^= 0x55;
    CHECK(!rac3_save_whole(save, (uint32_t)size));
    CHECK(rac3_save_info(save, (uint32_t)size, &level, &bolts, &seconds));
    last_level[20] ^= 0x55;
    CHECK(rac3_save_whole(save, (uint32_t)size));

    /* A checksum of 0 always fails, as the game's check has it. */
    const uint32_t crc = rac3_save_u32(save + 12);
    put_u32(save + 12, 0);
    CHECK(!rac3_save_whole(save, (uint32_t)size));
    put_u32(save + 12, crc);

    /* Sizes other than the tables' (Going Commando's, for one). */
    put_u32(save, 7416);
    CHECK(!rac3_save_whole(save, (uint32_t)size));
    put_u32(save, RAC3_SAVE_GLOBAL_SIZE);
    CHECK(rac3_save_whole(save, (uint32_t)size));

    /* Data longer than the global section has no checksum (func_0039EAC0 returns 0). */
    CHECK(rac3_save_crc16(save, RAC3_SAVE_GLOBAL_SIZE + 1) == 0);

    puts("rac3_save: ok");
    return 0;
}
