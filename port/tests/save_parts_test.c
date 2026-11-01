/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The SDK's rac1_save.h: what reads a save from another player's game. A save built here as the game
 * builds them passes; each way of damaging one is refused. Given files (real saves), each must pass
 * too: a save the game wrote is never refused. The mod's play_save_test goes through it as well. */
#include <stdio.h>
#include <stdlib.h>

#include "openrac/games/rac1_save.h"

static int failures;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c);                                \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static uint8_t save[OPENRAC_RAC1_SAVE_BYTES];

static void put(uint8_t* at, int32_t value) {
    memcpy(at, &value, sizeof value);
}

/* A part as the game writes it: its blocks in order, the end mark, then the checksum. */
static void make_part(
    uint8_t* part, uint32_t size, const openrac_rac1_save_block* blocks, uint32_t count
) {
    memset(part, 0, size);
    uint32_t at = 8;
    for (uint32_t i = 0; i < count; i++) {
        put(part + at, blocks[i].id);
        put(part + at + 4, blocks[i].size);
        at += 8 + (((uint32_t)blocks[i].size + 3u) & ~3u);
    }
    put(part + at, -1);
    put(part, (int32_t)(size - 8));
    put(part + 4, (int32_t)openrac_rac1_save_checksum(part + 8, size - 8));
}

static void make_save(void) {
    put(save, (int32_t)OPENRAC_RAC1_SAVE_GAME_BYTES);
    put(save + 4, (int32_t)OPENRAC_RAC1_SAVE_LEVEL_BYTES);
    make_part(
        save + 8, OPENRAC_RAC1_SAVE_GAME_BYTES, openrac_rac1_save_game_blocks,
        OPENRAC_RAC1_SAVE_GAME_BLOCKS
    );
    for (uint32_t i = 0; i < OPENRAC_RAC1_SAVE_LEVELS; i++) {
        make_part(
            save + 8 + OPENRAC_RAC1_SAVE_GAME_BYTES + i * OPENRAC_RAC1_SAVE_LEVEL_BYTES,
            OPENRAC_RAC1_SAVE_LEVEL_BYTES, openrac_rac1_save_level_blocks,
            OPENRAC_RAC1_SAVE_LEVEL_BLOCKS
        );
    }
}

static int fault(void) {
    return openrac_rac1_save_check(save, sizeof save);
}

/* The part at `part` signed again after a change, as a save made on purpose would be. */
static void resign(uint8_t* part) {
    int32_t size;
    memcpy(&size, part, sizeof size);
    put(part + 4, (int32_t)openrac_rac1_save_checksum(part + 8, (uint32_t)size));
}

static void built_saves(void) {
    make_save();
    CHECK(fault() == OPENRAC_RAC1_SAVE_OK);

    /* A byte changed in the last level part, or a part's length past its end. */
    save[sizeof save - 40] ^= 1;
    CHECK(fault() == OPENRAC_RAC1_SAVE_CHECKSUM);
    make_save();
    put(save + 8, (int32_t)OPENRAC_RAC1_SAVE_GAME_BYTES);
    CHECK(fault() == OPENRAC_RAC1_SAVE_BLOCKS);
    make_save();
    put(save + 8 + 4, 0);
    CHECK(fault() == OPENRAC_RAC1_SAVE_CHECKSUM);

    /* Signed, and still not one: the walks that would take the game past the end, or stop it
     * never: a size past the end, a negative size, and the end mark gone. */
    make_save();
    put(save + 16 + 4, 0x7FFFFFF0);
    resign(save + 8);
    CHECK(fault() == OPENRAC_RAC1_SAVE_BLOCKS);
    make_save();
    put(save + 16 + 4, -8);
    resign(save + 8);
    CHECK(fault() == OPENRAC_RAC1_SAVE_BLOCKS);
    make_save();
    put(save + 8 + OPENRAC_RAC1_SAVE_GAME_BYTES - 8, 7);
    resign(save + 8);
    CHECK(fault() == OPENRAC_RAC1_SAVE_BLOCKS);

    /* Signed, a value the game indexes with out of its range: the level. */
    make_save();
    put(save + 16 + 8, 19);
    resign(save + 8);
    CHECK(fault() == OPENRAC_RAC1_SAVE_RANGE);

    /* The sizes in front not adding up, or the file cut short. */
    make_save();
    put(save, (int32_t)OPENRAC_RAC1_SAVE_GAME_BYTES + 4);
    CHECK(fault() == OPENRAC_RAC1_SAVE_SIZE);
    make_save();
    CHECK(openrac_rac1_save_check(save, sizeof save - 1) == OPENRAC_RAC1_SAVE_SIZE);
    CHECK(openrac_rac1_save_check(save, 4) == OPENRAC_RAC1_SAVE_SIZE);
    CHECK(openrac_rac1_save_check(NULL, sizeof save) == OPENRAC_RAC1_SAVE_SIZE);
}

/* A part whose checksum truly comes to 0 is still refused: the game takes 0 for no checksum
 * (memcard_TestChecksum). The last level part's last block is free to change, so it finds one. */
static void a_checksum_of_zero(void) {
    make_save();
    uint8_t* part = save + sizeof save - OPENRAC_RAC1_SAVE_LEVEL_BYTES;
    const uint32_t size = OPENRAC_RAC1_SAVE_LEVEL_BYTES - 8u;
    uint8_t* value = part + 8 + size - 8 - 4; /* block 4002's value, before the end mark */
    int found = 0;
    for (uint32_t v = 0; v < 0x10000u && !found; v++) {
        memcpy(value, &v, sizeof v);
        found = openrac_rac1_save_checksum(part + 8, size) == 0;
    }
    CHECK(found);
    put(part + 4, 0);
    CHECK(fault() == OPENRAC_RAC1_SAVE_CHECKSUM);
}

static void real_save(const char* path) {
    FILE* f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f == NULL) {
        return;
    }
    static uint8_t bytes[0x10000];
    const size_t n = fread(bytes, 1, sizeof bytes, f);
    fclose(f);
    /* A slot never saved to is whole, with no level in it. */
    int32_t level;
    memcpy(&level, bytes + 24, sizeof level);
    const int wanted = level == -1 ? OPENRAC_RAC1_SAVE_RANGE : OPENRAC_RAC1_SAVE_OK;
    if (openrac_rac1_save_check(bytes, n) != wanted) {
        fprintf(stderr, "%s: answers %d\n", path, openrac_rac1_save_check(bytes, n));
        failures++;
    }
}

int main(int argc, char** argv) {
    built_saves();
    a_checksum_of_zero();
    for (int i = 1; i < argc; i++) {
        real_save(argv[i]);
    }
    return failures != 0;
}
