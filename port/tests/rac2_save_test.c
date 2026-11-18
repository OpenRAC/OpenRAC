/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's save read without the game (rac2/host/save_format.h, the play calls' save_info
 * and save_check), on a save the game wrote: the launcher's fixture (an Oozla save given 123,456
 * bolts, launcher/docs/RAC2_SAVE_FORMAT.md, e2), and the same save broken the ways a save from
 * elsewhere can be. */
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

static uint8_t save[RAC2_SAVE_SIZE + 16];

static void put_u32(uint8_t* at, uint32_t v) {
    memcpy(at, &v, sizeof v);
}

int main(void) {
    FILE* f = fopen(RAC2_SAVE_FIXTURE, "rb");
    CHECK(f != NULL);
    const size_t size = fread(save, 1, sizeof save, f);
    fclose(f);
    CHECK(size == RAC2_SAVE_SIZE);

    int32_t level = -2, bolts = -1, gold = -1, seconds = -1;
    CHECK(rac2_save_info(save, (uint32_t)size, &level, &bolts, &gold, &seconds));
    CHECK(level == 1);      /* Oozla */
    CHECK(bolts == 123456); /* OPENRAC_GIVE=bolts:123456 */
    CHECK(seconds == 28685 / RAC2_SAVE_TICKS_A_SECOND);
    CHECK(gold == 0);
    CHECK(rac2_save_whole(save, (uint32_t)size));
    CHECK(rac2_save_crc16(save + 16, 7408) == rac2_save_u32(save + 12));

    /* Not the whole file, or more of it. */
    CHECK(!rac2_save_whole(save, (uint32_t)size - 4));
    CHECK(!rac2_save_info(save, (uint32_t)size + 4, &level, &bolts, &gold, &seconds));

    /* A byte of a level section changed: its checksum fails, so the save is refused whole, though
     * its head still reads. */
    uint8_t* last_level = save + 8 + RAC2_SAVE_GLOBAL_SIZE + 27 * RAC2_SAVE_LEVEL_SIZE;
    last_level[20] ^= 0x55;
    CHECK(!rac2_save_whole(save, (uint32_t)size));
    CHECK(rac2_save_info(save, (uint32_t)size, &level, &bolts, &gold, &seconds));
    last_level[20] ^= 0x55;
    CHECK(rac2_save_whole(save, (uint32_t)size));

    /* A checksum of 0 always fails, as the game's check has it. */
    const uint32_t crc = rac2_save_u32(save + 12);
    put_u32(save + 12, 0);
    CHECK(!rac2_save_whole(save, (uint32_t)size));
    CHECK(!rac2_save_info(save, (uint32_t)size, &level, &bolts, &gold, &seconds));
    put_u32(save + 12, crc);

    /* Sizes other than the tables' (SaveUnpack refuses them). */
    put_u32(save, RAC2_SAVE_GLOBAL_SIZE + 4);
    CHECK(!rac2_save_whole(save, (uint32_t)size));
    put_u32(save, RAC2_SAVE_GLOBAL_SIZE);

    /* An empty slot's level, -1: a section the game would read, but no save to load. Its level
     * chunk is the global section's first; the checksum is made again. */
    uint32_t bytes = 0;
    uint8_t* chunk = (uint8_t*)rac2_save_chunk(save + 8, RAC2_SAVE_GLOBAL_SIZE, 0, &bytes);
    CHECK(chunk != NULL && bytes == 4);
    put_u32(chunk, 0xFFFFFFFFu);
    put_u32(save + 12, rac2_save_crc16(save + 16, rac2_save_u32(save + 8)));
    CHECK(rac2_save_whole(save, (uint32_t)size));
    CHECK(!rac2_save_info(save, (uint32_t)size, &level, &bolts, &gold, &seconds));
    CHECK(level == -1);

    /* A platinum bolt found on Oozla is counted. */
    put_u32(chunk, 1);
    put_u32(save + 12, rac2_save_crc16(save + 16, rac2_save_u32(save + 8)));
    uint8_t* oozla = save + 8 + RAC2_SAVE_GLOBAL_SIZE + 1 * RAC2_SAVE_LEVEL_SIZE;
    uint8_t* found = (uint8_t*)rac2_save_chunk(oozla, RAC2_SAVE_LEVEL_SIZE, 3003, &bytes);
    CHECK(found != NULL && bytes == 4);
    found[0] = 1;
    found[2] = 1;
    put_u32(oozla + 4, rac2_save_crc16(oozla + 8, rac2_save_u32(oozla)));
    CHECK(rac2_save_whole(save, (uint32_t)size));
    CHECK(rac2_save_info(save, (uint32_t)size, &level, &bolts, &gold, &seconds));
    CHECK(gold == 2);

    puts("rac2_save: ok");
    return 0;
}
