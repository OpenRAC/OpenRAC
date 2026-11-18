/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* A level's sim digest (rac1/host/sim_digest.h). Two discs' records that differ as Hoven's do, in
 * the update distance of class 805's records alone, have one digest; a record more, another order,
 * another class, place, spawn id, flag or bolts give another, as two discs that could not share
 * this level's keys would. The numbers are the same as the disc check's
 * (port/tools/versionmap/sim_digest.py), which measures every level of both discs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1/host/sim_digest.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

enum {
    RECORD = 0x78,
    COUNT = 6,
    UPDATE_DISTANCE = 0x24,
    DRAW_DISTANCE = 0x20,
    PVAR_INDEX = 0x58,
    MISSION = 0x4,
    FLAGS = 0x8,
    SPAWN_ID = 0xC,
    BOLTS = 0x10,
    CLASS = 0x18,
    X = 0x30,
    RZ = 0x44
};

static void put(uint8_t* table, int record, int offset, uint32_t word) {
    memcpy(table + record * RECORD + offset, &word, 4);
}

/* Six spawn points in the manner of Hoven's 561 to 566. */
static void hoven(uint8_t* table, uint32_t update_distance) {
    memset(table, 0, COUNT * RECORD);
    for (int i = 0; i < COUNT; ++i) {
        put(table, i, 0, RECORD);
        put(table, i, SPAWN_ID, 561u + (uint32_t)i);
        put(table, i, CLASS, 805);
        put(table, i, UPDATE_DISTANCE, update_distance);
        put(table, i, X, 0x42C80000u + (uint32_t)i);
    }
}

static uint32_t digest(const uint8_t* table) {
    return sim_digest_records(table, COUNT, COUNT * RECORD);
}

/* Each of `offset`'s changes in one record changes the digest. */
static void changes(uint32_t base, int offset) {
    uint8_t table[COUNT * RECORD];
    hoven(table, 255);
    table[3 * RECORD + offset] ^= 1;
    CHECK(digest(table) != base);
}

int main(void) {
    uint8_t pal[COUNT * RECORD], ntsc[COUNT * RECORD];
    hoven(pal, 255);
    hoven(ntsc, 64);
    CHECK(memcmp(pal, ntsc, sizeof pal) != 0);
    const uint32_t base = digest(pal);
    CHECK(base != 0);
    /* The measured difference is allowed. */
    CHECK(digest(ntsc) == base);
    /* The same value as the disc check's, so the two read a record alike. */
    CHECK(base == 0xC43115BDu);

    /* What each game keeps to itself does not count. */
    uint8_t own[COUNT * RECORD];
    hoven(own, 255);
    put(own, 2, DRAW_DISTANCE, 99);
    put(own, 2, PVAR_INDEX, 7);
    CHECK(digest(own) == base);

    /* What the keys rest on does. */
    changes(base, MISSION);
    changes(base, FLAGS);
    changes(base, SPAWN_ID);
    changes(base, BOLTS);
    changes(base, CLASS);
    changes(base, X);
    changes(base, RZ + 3);

    /* A record more or fewer, and two the other way round. */
    CHECK(sim_digest_records(pal, COUNT - 1, COUNT * RECORD) != base);
    uint8_t swapped[COUNT * RECORD];
    hoven(swapped, 255);
    memcpy(swapped, pal + RECORD, RECORD);
    memcpy(swapped + RECORD, pal, RECORD);
    CHECK(digest(swapped) != base);

    /* A table that does not read as records is "not known". */
    CHECK(sim_digest_records(pal, -1, COUNT * RECORD) == 0);
    CHECK(sim_digest_records(pal, COUNT + 1, COUNT * RECORD) == 0);
    CHECK(sim_digest_records(pal, COUNT, COUNT * RECORD - 1) == 0);
    uint8_t broken[COUNT * RECORD];
    hoven(broken, 255);
    put(broken, 1, 0, 3);
    CHECK(digest(broken) == 0);
    puts("sim_digest: all passed");
    return 0;
}
