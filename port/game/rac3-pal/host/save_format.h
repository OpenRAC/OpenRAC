/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's save file, read without the game (play.c's save_info and save_check, and a
 * unit test: port/tests/rac3_save_test.c). launcher/docs/RAC3_SAVE_FORMAT.md has the layout. It is
 * Going Commando's container (rac2/host/save_format.h) with other tables: SavePack (func_0039E9C0)
 * writes it, the card code (func_0039C7F0) reads it back, section by section (func_0039ED78):
 *
 *   +0  u32 the global section's size, +4 u32 a level section's size (func_0039EA88 of the global
 *       table 0x00332388 and of the level table 0x00332618), then the global section and 37 level
 *       sections (func_0039E9C0 loops 0x25 times).
 *   a section: u32 its data's size (the section less these 8 bytes), u32 the data's CRC-16
 *       (func_0039EAC0: rac1's and rac2's, 0 for data longer than the global section), then
 *       chunks {s32 id, u32 size, the bytes, padded to 4}, ended by {-1, 0} (func_0039EBA8).
 *
 * The global chunks read here: 0 the level the save resumes on (D_001D545C; -1 in an empty slot:
 * the game writes all four slots so when it makes its folder), 39 the progress block (0x00142660,
 * its first word the bolts), 3 the play time (D_001D5528) in ticks, 60 a second on every disc: the
 * Load page's slot list (func_003B7748) shows hours as ticks / 216000 and minutes as ticks / 3600
 * less 60 an hour. Checked against the files the game writes on a fresh card (a hidden run of the
 * port): 134,992 bytes, both sizes and every checksum as here. */
#ifndef OPENRAC_RAC3_SAVE_FORMAT_H
#define OPENRAC_RAC3_SAVE_FORMAT_H

#include <stdint.h>
#include <string.h>

#define RAC3_SAVE_GLOBAL_SIZE 14216u
#define RAC3_SAVE_LEVEL_SIZE 3264u
#define RAC3_SAVE_LEVELS 37u
#define RAC3_SAVE_SIZE (8u + RAC3_SAVE_GLOBAL_SIZE + RAC3_SAVE_LEVELS * RAC3_SAVE_LEVEL_SIZE)
#define RAC3_SAVE_TICKS_A_SECOND 60
/* A save's level must be one the game numbers (1..55; 0 is none). */
#define RAC3_SAVE_LEVEL_LIMIT 56
/* The slots, save0.bin to save3.bin (the card code's listing takes "save" and one digit under 4). */
#define RAC3_SAVE_SLOTS 4

#define RAC3_SAVE_CHUNK_LEVEL 0
#define RAC3_SAVE_CHUNK_TIME 3
#define RAC3_SAVE_CHUNK_PROGRESS 39

static inline uint32_t rac3_save_u32(const uint8_t* at) {
    uint32_t v;
    memcpy(&v, at, sizeof v);
    return v;
}

/* func_0039EAC0: the register seeded with 0xEDB88320, polynomial 0x1F45, the high bit first, the low
 * 16 bits kept; 0 (a checksum that always fails) for data longer than the global section. */
static inline uint32_t rac3_save_crc16(const uint8_t* data, uint32_t size) {
    if (size > RAC3_SAVE_GLOBAL_SIZE) {
        return 0;
    }
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
 * size and checksum are the game's (a stored 0 always fails, as func_0039EB68 has it). */
static inline int rac3_save_section_ok(const uint8_t* section, uint32_t size) {
    const uint32_t data = rac3_save_u32(section);
    const uint32_t crc = rac3_save_u32(section + 4);
    if (data + 8u != size || crc == 0 || rac3_save_crc16(section + 8, data) != crc) {
        return 0;
    }
    uint32_t at = 8;
    while (at + 8u <= size) {
        const int32_t id = (int32_t)rac3_save_u32(section + at);
        const uint32_t bytes = rac3_save_u32(section + at + 4);
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
 * rac3_save_section_ok took. */
static inline const uint8_t* rac3_save_chunk(
    const uint8_t* section, uint32_t size, int32_t id, uint32_t* bytes_out
) {
    uint32_t at = 8;
    while (at + 8u <= size) {
        const int32_t cid = (int32_t)rac3_save_u32(section + at);
        const uint32_t bytes = rac3_save_u32(section + at + 4);
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

static inline int rac3_save_head_ok(const uint8_t* save, uint32_t size) {
    return save != NULL && size == RAC3_SAVE_SIZE && rac3_save_u32(save) == RAC3_SAVE_GLOBAL_SIZE
           && rac3_save_u32(save + 4) == RAC3_SAVE_LEVEL_SIZE
           && rac3_save_section_ok(save + 8, RAC3_SAVE_GLOBAL_SIZE);
}

/* Whether `save` is a whole save the game would load: the two sizes the tables give and every
 * section's chunks and checksum. */
static inline int rac3_save_whole(const uint8_t* save, uint32_t size) {
    if (!rac3_save_head_ok(save, size)) {
        return 0;
    }
    for (uint32_t i = 0; i < RAC3_SAVE_LEVELS; ++i) {
        const uint8_t* section = save + 8u + RAC3_SAVE_GLOBAL_SIZE + i * RAC3_SAVE_LEVEL_SIZE;
        if (!rac3_save_section_ok(section, RAC3_SAVE_LEVEL_SIZE)) {
            return 0;
        }
    }
    return 1;
}

/* What a save holds: 1 and the fields for a save of a level (an empty slot's level is -1), 0
 * otherwise. Reads the sizes and the global section (its checksum too). */
static inline int rac3_save_info(
    const uint8_t* save, uint32_t size, int32_t* level, int32_t* bolts, int32_t* seconds
) {
    if (!rac3_save_head_ok(save, size)) {
        return 0;
    }
    const uint8_t* global = save + 8;
    uint32_t bytes = 0;
    const uint8_t* at =
        rac3_save_chunk(global, RAC3_SAVE_GLOBAL_SIZE, RAC3_SAVE_CHUNK_LEVEL, &bytes);
    if (at == NULL || bytes != 4) {
        return 0;
    }
    *level = (int32_t)rac3_save_u32(at);
    at = rac3_save_chunk(global, RAC3_SAVE_GLOBAL_SIZE, RAC3_SAVE_CHUNK_PROGRESS, &bytes);
    if (at == NULL || bytes < 4) {
        return 0;
    }
    *bolts = (int32_t)rac3_save_u32(at);
    at = rac3_save_chunk(global, RAC3_SAVE_GLOBAL_SIZE, RAC3_SAVE_CHUNK_TIME, &bytes);
    if (at == NULL || bytes != 4) {
        return 0;
    }
    *seconds = (int32_t)(rac3_save_u32(at) / RAC3_SAVE_TICKS_A_SECOND);
    return *level > 0 && *level < RAC3_SAVE_LEVEL_LIMIT;
}

#endif /* OPENRAC_RAC3_SAVE_FORMAT_H */
