/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Whether a Ratchet & Clank save can be handed to the game: the one check, which the port runs
 * before play_start and coop_save_write and a mod can run before it sends or accepts a save. The
 * game trusts its card: memcard_RestoreData walks the blocks by the sizes in them with no end but a
 * mark, and the game indexes tables with what it reads, so bytes from another player's game are
 * read here first. Both discs write one layout, so one check holds for both: the size, the two
 * sizes at the front, each part's checksum, every block's id and size in the game's own order, and
 * the values the game uses as an index or a count.
 *
 * The file: the sizes of the game part and of a level part (8 bytes), the game part, 20 level parts.
 * A part: its length after this header, its checksum, then the blocks, each {id, size, bytes}
 * padded to 4, and {-1, 0} at the end.
 *
 * An empty slot is not a save: its level is -1, and it answers OPENRAC_RAC1_SAVE_RANGE.
 *
 * C and C++, header only, no game needed.
 */
#ifndef OPENRAC_GAMES_RAC1_SAVE_H
#define OPENRAC_GAMES_RAC1_SAVE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPENRAC_RAC1_SAVE_GAME_BYTES 5424u
#define OPENRAC_RAC1_SAVE_LEVEL_BYTES 2724u
#define OPENRAC_RAC1_SAVE_LEVELS 20u
#define OPENRAC_RAC1_SAVE_BYTES                                                                    \
    (8u + OPENRAC_RAC1_SAVE_GAME_BYTES + OPENRAC_RAC1_SAVE_LEVELS * OPENRAC_RAC1_SAVE_LEVEL_BYTES)

/* The largest part the game's checksum takes (memcard_Checksum). */
#define OPENRAC_RAC1_SAVE_PART_MOST 0x1800u

/* What is wrong with a save. */
#define OPENRAC_RAC1_SAVE_OK 0
#define OPENRAC_RAC1_SAVE_SIZE 1     /* not a save's size, or the sizes at its front are not the game's */
#define OPENRAC_RAC1_SAVE_CHECKSUM 2 /* a part's checksum is wrong */
#define OPENRAC_RAC1_SAVE_BLOCKS 3   /* a part's blocks are not the game's, in its order */
#define OPENRAC_RAC1_SAVE_RANGE 4    /* a value the game indexes with is out of its range */

/* One block of a save. */
typedef struct openrac_rac1_save_block {
    int32_t id;
    int32_t size;
} openrac_rac1_save_block;

/* The two discs' tables of what a save holds (mc_gamedata, mc_leveldata): the same ids and sizes in
 * the same order, entry for entry, read from both executables on 2026-10-31. */
#define OPENRAC_RAC1_SAVE_GAME_BLOCKS 47u
#define OPENRAC_RAC1_SAVE_LEVEL_BLOCKS 11u

static const openrac_rac1_save_block openrac_rac1_save_game_blocks[OPENRAC_RAC1_SAVE_GAME_BLOCKS] = {
    {0, 4},      {1, 4},      {2, 4},      {3, 4},      {4, 8},      {5, 0x80},   {7, 0xC},
    {8, 0x20},   {9, 0x94},   {10, 0x25},  {11, 0x25},  {12, 0xC},   {13, 0x20},  {19, 4},
    {14, 0x14},  {20, 0x50},  {15, 0x790}, {16, 0x4A0}, {17, 0x120}, {18, 0x128}, {21, 4},
    {22, 4},     {23, 4},     {24, 4},     {25, 4},     {26, 4},     {27, 4},     {28, 1},
    {29, 1},     {30, 0x28},  {31, 4},     {32, 0x1C},  {33, 1},     {34, 4},     {35, 4},
    {36, 4},     {37, 0xC},   {1000, 0x94}, {1001, 0x94}, {1002, 0x94}, {1003, 4},  {1004, 4},
    {1005, 4},   {1008, 4},   {1009, 4},   {1010, 0x96}, {1011, 4},
};

static const openrac_rac1_save_block openrac_rac1_save_level_blocks[OPENRAC_RAC1_SAVE_LEVEL_BLOCKS] = {
    {3001, 1}, {3002, 0x800}, {3003, 4}, {3004, 0x10}, {3005, 0x100}, {3006, 0x100},
    {3007, 8}, {3008, 0x10},  {4000, 4}, {4001, 4},    {4002, 4},
};

static inline int32_t openrac_rac1_save_word_(const uint8_t* at) {
    int32_t v;
    memcpy(&v, at, sizeof v);
    return v;
}

/* The game's checksum of a part (memcard_Checksum), 0 past the most it takes. */
static inline uint32_t openrac_rac1_save_checksum(const uint8_t* data, size_t size) {
    uint32_t crc = 0xEDB88320u;
    size_t i;
    int bit;

    if (size > OPENRAC_RAC1_SAVE_PART_MOST) {
        return 0;
    }

    for (i = 0; i < size; ++i) {
        crc ^= (uint32_t)data[i] << 8;

        for (bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000u) != 0 ? (crc << 1) ^ 0x1F45u : crc << 1;
        }
    }

    return crc & 0xFFFFu;
}

/* Every word of a block in [low, high). */
static inline int openrac_rac1_save_words_in_(const uint8_t* at, size_t size, int32_t low, int32_t high) {
    size_t i;

    for (i = 0; i + 4 <= size; i += 4) {
        const int32_t v = openrac_rac1_save_word_(at + i);

        if (v < low || v >= high) {
            return 0;
        }
    }

    return 1;
}

/* Whether the values of a block the game indexes with, or counts by, are in their range. */
static inline int openrac_rac1_save_in_range_(const openrac_rac1_save_block* block, const uint8_t* at) {
    const size_t size = (size_t)block->size;

    switch (block->id) {
        case 0: /* the level */
            return openrac_rac1_save_words_in_(at, size, 0, 19);
        case 1: /* the bolts */
        case 3: /* the clock */
            return openrac_rac1_save_word_(at) >= 0;
        case 2: /* the gold bolts */
            return openrac_rac1_save_words_in_(at, size, 0, 80 + 1);
        case 13: /* the quick select */
        case 21: /* the quick swap */
        case 32: /* the last weapons */
            return openrac_rac1_save_words_in_(at, size, -1, 40);
        case 20: /* the planet list */
            return openrac_rac1_save_words_in_(at, size, 0, 20);
        case 1011: /* the help messages */
            return openrac_rac1_save_words_in_(at, size, 0, 0x96 + 1);
        default:
            return 1;
    }
}

static inline int openrac_rac1_save_check_part_(
    const uint8_t* part, size_t size, const openrac_rac1_save_block* blocks, size_t count, int values
) {
    const int32_t length = openrac_rac1_save_word_(part);
    const int32_t sum = openrac_rac1_save_word_(part + 4);
    size_t at = 8;
    size_t i;

    if (length != (int32_t)size - 8) {
        return OPENRAC_RAC1_SAVE_BLOCKS;
    }

    if (sum == 0 || (uint32_t)sum != openrac_rac1_save_checksum(part + 8, (size_t)length)) {
        return OPENRAC_RAC1_SAVE_CHECKSUM;
    }

    /* The walk memcard_RestoreData makes, kept inside the part: each block must be the next of the
     * game's, so no size in the file is ever followed. */
    for (i = 0; i < count; ++i) {
        const openrac_rac1_save_block* block = &blocks[i];
        const size_t room = 8 + (((size_t)block->size + 3) & ~(size_t)3);

        if (at + room + 8 > size || openrac_rac1_save_word_(part + at) != block->id
            || openrac_rac1_save_word_(part + at + 4) != block->size) {
            return OPENRAC_RAC1_SAVE_BLOCKS;
        }

        if (values && !openrac_rac1_save_in_range_(block, part + at + 8)) {
            return OPENRAC_RAC1_SAVE_RANGE;
        }

        at += room;
    }

    if (at + 8 != size || openrac_rac1_save_word_(part + at) != -1
        || openrac_rac1_save_word_(part + at + 4) != 0) {
        return OPENRAC_RAC1_SAVE_BLOCKS;
    }

    return OPENRAC_RAC1_SAVE_OK;
}

/* What is wrong with the `size` bytes at `save`: OPENRAC_RAC1_SAVE_OK for a save the game can load. */
static inline int openrac_rac1_save_check(const uint8_t* save, size_t size) {
    int fault;
    size_t i;

    if (save == NULL || size != OPENRAC_RAC1_SAVE_BYTES
        || openrac_rac1_save_word_(save) != (int32_t)OPENRAC_RAC1_SAVE_GAME_BYTES
        || openrac_rac1_save_word_(save + 4) != (int32_t)OPENRAC_RAC1_SAVE_LEVEL_BYTES) {
        return OPENRAC_RAC1_SAVE_SIZE;
    }

    fault = openrac_rac1_save_check_part_(
        save + 8, OPENRAC_RAC1_SAVE_GAME_BYTES, openrac_rac1_save_game_blocks,
        OPENRAC_RAC1_SAVE_GAME_BLOCKS, 1
    );

    for (i = 0; fault == OPENRAC_RAC1_SAVE_OK && i < OPENRAC_RAC1_SAVE_LEVELS; ++i) {
        fault = openrac_rac1_save_check_part_(
            save + 8 + OPENRAC_RAC1_SAVE_GAME_BYTES + i * OPENRAC_RAC1_SAVE_LEVEL_BYTES,
            OPENRAC_RAC1_SAVE_LEVEL_BYTES, openrac_rac1_save_level_blocks,
            OPENRAC_RAC1_SAVE_LEVEL_BLOCKS, 0
        );
    }

    return fault;
}

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAMES_RAC1_SAVE_H */
