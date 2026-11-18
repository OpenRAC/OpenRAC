/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's save file, read without the game (play.c's save_info and save_check, and a unit
 * test on a real save: port/tests/rac2_save_test.c). launcher/docs/RAC2_SAVE_FORMAT.md has the
 * layout; SavePack (0x0029ADA0) writes it, SaveUnpack (0x00298D28) reads it:
 *
 *   +0  u32 the global section's size, +4 u32 a level section's size (the save tables' sizes:
 *       SaveUnpack takes nothing else), then the global section and 28 level sections.
 *   a section: u32 its data's size (the section less these 8 bytes), u32 the data's CRC-16
 *       (SaveChecksum, 0x0029AEB0), then chunks {s32 id, u32 size, the bytes, padded to 4}, ended
 *       by {-1, 0}.
 *
 * The global chunks read here: 0 the level the save resumes on (-1 in an empty slot), 39 the
 * progress block (its first word the bolts), 3 the play time in ticks (60 a second: the level loop
 * adds one a tick, ten more every 50 on a 50 Hz disc). Per level, chunk 3003 holds the four
 * platinum bolts' found bytes (g_platinumBoltFlags). The three discs' tables are the same (the
 * launcher's save editor reads all three with one layout). SaveUnpack checks only the two sizes,
 * so a save from elsewhere is checked here whole before the game is handed it. */
#ifndef OPENRAC_RAC2_SAVE_FORMAT_H
#define OPENRAC_RAC2_SAVE_FORMAT_H

#include <stdint.h>
#include <string.h>

#define RAC2_SAVE_GLOBAL_SIZE 7416u
#define RAC2_SAVE_LEVEL_SIZE 3280u
#define RAC2_SAVE_LEVELS 28u
#define RAC2_SAVE_SIZE (8u + RAC2_SAVE_GLOBAL_SIZE + RAC2_SAVE_LEVELS * RAC2_SAVE_LEVEL_SIZE)
#define RAC2_SAVE_TICKS_A_SECOND 60
/* The levels the game numbers (0..30), as a save's level must be one. */
#define RAC2_SAVE_LEVEL_LIMIT 31

#define RAC2_SAVE_CHUNK_LEVEL 0
#define RAC2_SAVE_CHUNK_TIME 3
#define RAC2_SAVE_CHUNK_PROGRESS 39
#define RAC2_SAVE_CHUNK_PLATINUM 3003

static inline uint32_t rac2_save_u32(const uint8_t* at) {
    uint32_t v;
    memcpy(&v, at, sizeof v);
    return v;
}

/* SaveChecksum: the register seeded with 0xEDB88320, polynomial 0x1F45, the high bit first, the low
 * 16 bits kept. */
static inline uint32_t rac2_save_crc16(const uint8_t* data, uint32_t size) {
    uint32_t crc = 0xEDB88320u;
    for (uint32_t i = 0; i < size; ++i) {
        crc ^= (uint32_t)data[i] << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1F45u : crc << 1;
        }
    }
    return crc & 0xFFFFu;
}

/* The section of `size` bytes at `section`: its chunks walk to their end inside it, and its data's
 * size and checksum are the game's (a stored 0 always fails, as func_0029AF58 has it). */
static inline int rac2_save_section_ok(const uint8_t* section, uint32_t size) {
    const uint32_t data = rac2_save_u32(section);
    const uint32_t crc = rac2_save_u32(section + 4);
    if (data + 8u != size || crc == 0 || rac2_save_crc16(section + 8, data) != crc) {
        return 0;
    }
    uint32_t at = 8;
    while (at + 8u <= size) {
        const int32_t id = (int32_t)rac2_save_u32(section + at);
        const uint32_t bytes = rac2_save_u32(section + at + 4);
        if (id == -1) {
            return bytes == 0 && at + 8u == size;
        }
        if (bytes > size - at - 8u) {
            return 0;
        }
        at += 8u + ((bytes + 3u) & ~3u);
    }
    return 0;
}

/* Chunk `id` of the section: its bytes and size, NULL for none. The section is one
 * rac2_save_section_ok took. */
static inline const uint8_t* rac2_save_chunk(
    const uint8_t* section, uint32_t size, int32_t id, uint32_t* bytes_out
) {
    uint32_t at = 8;
    while (at + 8u <= size) {
        const int32_t cid = (int32_t)rac2_save_u32(section + at);
        const uint32_t bytes = rac2_save_u32(section + at + 4);
        if (cid == -1 || bytes > size - at - 8u) {
            return NULL;
        }
        if (cid == id) {
            *bytes_out = bytes;
            return section + at + 8;
        }
        at += 8u + ((bytes + 3u) & ~3u);
    }
    return NULL;
}

/* Whether `save` is a whole save the game would load: the two sizes the tables give, every
 * section's chunks and checksum, a level the game has. */
static inline int rac2_save_whole(const uint8_t* save, uint32_t size) {
    if (save == NULL || size != RAC2_SAVE_SIZE || rac2_save_u32(save) != RAC2_SAVE_GLOBAL_SIZE
        || rac2_save_u32(save + 4) != RAC2_SAVE_LEVEL_SIZE
        || !rac2_save_section_ok(save + 8, RAC2_SAVE_GLOBAL_SIZE)) {
        return 0;
    }
    for (uint32_t i = 0; i < RAC2_SAVE_LEVELS; ++i) {
        const uint8_t* section = save + 8u + RAC2_SAVE_GLOBAL_SIZE + i * RAC2_SAVE_LEVEL_SIZE;
        if (!rac2_save_section_ok(section, RAC2_SAVE_LEVEL_SIZE)) {
            return 0;
        }
    }
    return 1;
}

/* What a save holds: 1 and the fields for a save of a level (an empty slot's level is -1), 0
 * otherwise. Reads the sizes and the global section (its checksum too); gold_bolts counts the
 * platinum bolts found, from the level sections that are whole. */
static inline int rac2_save_info(
    const uint8_t* save,
    uint32_t size,
    int32_t* level,
    int32_t* bolts,
    int32_t* gold_bolts,
    int32_t* seconds
) {
    if (save == NULL || size != RAC2_SAVE_SIZE || rac2_save_u32(save) != RAC2_SAVE_GLOBAL_SIZE
        || rac2_save_u32(save + 4) != RAC2_SAVE_LEVEL_SIZE
        || !rac2_save_section_ok(save + 8, RAC2_SAVE_GLOBAL_SIZE)) {
        return 0;
    }
    const uint8_t* global = save + 8;
    uint32_t bytes = 0;
    const uint8_t* at =
        rac2_save_chunk(global, RAC2_SAVE_GLOBAL_SIZE, RAC2_SAVE_CHUNK_LEVEL, &bytes);
    if (at == NULL || bytes != 4) {
        return 0;
    }
    *level = (int32_t)rac2_save_u32(at);
    at = rac2_save_chunk(global, RAC2_SAVE_GLOBAL_SIZE, RAC2_SAVE_CHUNK_PROGRESS, &bytes);
    if (at == NULL || bytes < 4) {
        return 0;
    }
    *bolts = (int32_t)rac2_save_u32(at);
    at = rac2_save_chunk(global, RAC2_SAVE_GLOBAL_SIZE, RAC2_SAVE_CHUNK_TIME, &bytes);
    if (at == NULL || bytes != 4) {
        return 0;
    }
    *seconds = (int32_t)(rac2_save_u32(at) / RAC2_SAVE_TICKS_A_SECOND);
    int32_t platinum = 0;
    for (uint32_t i = 0; i < RAC2_SAVE_LEVELS; ++i) {
        const uint8_t* section = save + 8u + RAC2_SAVE_GLOBAL_SIZE + i * RAC2_SAVE_LEVEL_SIZE;
        if (!rac2_save_section_ok(section, RAC2_SAVE_LEVEL_SIZE)) {
            continue;
        }
        at = rac2_save_chunk(section, RAC2_SAVE_LEVEL_SIZE, RAC2_SAVE_CHUNK_PLATINUM, &bytes);
        for (uint32_t b = 0; at != NULL && b < bytes; ++b) {
            platinum += at[b] != 0;
        }
    }
    *gold_bolts = platinum;
    return *level >= 0 && *level < RAC2_SAVE_LEVEL_LIMIT;
}

#endif /* OPENRAC_RAC2_SAVE_FORMAT_H */
