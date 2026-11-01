/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* A level's sim digest (moby_pool.c): a number for what must agree between two games that share a
 * level, read from its gameplay file's moby records, on plain bytes so a test can run it without
 * the game.
 *
 * A party's world names a placed moby by its record's place in the file (world.c), so two games
 * share a level only if their records say the same: how many, in which order, and in each what the
 * loader decides by (its size, mission, flags, spawn id, bolts), its class, scale, position and
 * rotation. Nothing else goes in: each game runs its own copy of a moby's update distance, and on
 * Hoven the discs' records differ in just that (six class 805 records, 255 against 64). */
#ifndef RAC1_SIM_DIGEST_H
#define RAC1_SIM_DIGEST_H

#include <stdint.h>

enum {
    SIM_RECORD_MIN = 0x48, /* a record up to the end of what is read */
    SIM_RECORD_MAX = 0x400,
    SIM_RECORD_KEYS = 0,      /* size, mission, flags, spawn id, two bolts, class, scale */
    SIM_RECORD_KEYS_SIZE = 0x20,
    SIM_RECORD_PLACE = 0x30,  /* position and rotation, three floats each */
    SIM_RECORD_PLACE_SIZE = 0x18,
    SIM_MOBYS_MAX = 0x8000
};

#define SIM_DIGEST_START 2166136261u

static inline uint32_t sim_digest_mix(uint32_t hash, const uint8_t* bytes, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static inline uint32_t sim_digest_word(const uint8_t* bytes) {
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16
           | (uint32_t)bytes[3] << 24;
}

/* The digest of `count` records from `records`, `size` bytes of them there. 0 stands for "not
 * known" and is never a digest: a table that does not read as records gives 0, a digest of 0 gives
 * 1. */
static inline uint32_t sim_digest_records(const uint8_t* records, int32_t count, uint32_t size) {
    if (count < 0 || count > SIM_MOBYS_MAX) {
        return 0;
    }
    const uint8_t count_bytes[4] = {
        (uint8_t)count, (uint8_t)(count >> 8), (uint8_t)(count >> 16), (uint8_t)(count >> 24)
    };
    uint32_t hash = sim_digest_mix(SIM_DIGEST_START, count_bytes, 4);
    uint32_t at = 0;
    for (int32_t i = 0; i < count; ++i) {
        if (size - at < SIM_RECORD_MIN) {
            return 0;
        }
        const uint32_t record = sim_digest_word(records + at);
        if (record < SIM_RECORD_MIN || record > SIM_RECORD_MAX || (record & 3u) != 0
            || record > size - at) {
            return 0;
        }
        hash = sim_digest_mix(hash, records + at + SIM_RECORD_KEYS, SIM_RECORD_KEYS_SIZE);
        hash = sim_digest_mix(hash, records + at + SIM_RECORD_PLACE, SIM_RECORD_PLACE_SIZE);
        at += record;
    }
    return hash != 0 ? hash : 1;
}

#endif /* RAC1_SIM_DIGEST_H */
