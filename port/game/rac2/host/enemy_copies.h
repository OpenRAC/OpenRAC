/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: copies of a level's placed enemies for a mod (openrac_api moby_copy_check and
 * moby_copy, interface 1.25; enemy_copies.c says how each part is found in the game). */
#ifndef OPENRAC_RAC2_ENEMY_COPIES_H
#define OPENRAC_RAC2_ENEMY_COPIES_H

#include <stdint.h>

#include "openrac/guest.h"

/* Why a moby may not be copied: OPENRAC_COPY_* of openrac_mod.h, the same numbers. */
enum {
    RAC2_COPY_OK = 0,
    /* Not of the kind the bestiary counts (kind 5). */
    RAC2_COPY_NOT_ENEMY = 1,
    /* Made by the game, or with no record or reward in the level's file. */
    RAC2_COPY_NOT_PLACED = 2,
    /* A group's, named by the level's data, or naming another moby. */
    RAC2_COPY_SCRIPTED = 3,
    /* The port's table: bosses, turrets, spawners, the crystals' creatures. */
    RAC2_COPY_LISTED = 4,
    /* Its pvars larger than a copy may have. */
    RAC2_COPY_NO_ROOM = 5,
    /* No level, or the level's file is no longer in memory. */
    RAC2_COPY_UNKNOWN = -1
};

/* Why a scripted moby is (RAC2_COPY_SCRIPTED): its record's script slot, a group's member, named by
 * another's data, naming another. */
enum {
    RAC2_COPY_WHY_SCRIPT = 1,
    RAC2_COPY_WHY_GROUP = 2,
    RAC2_COPY_WHY_NAMED = 4,
    RAC2_COPY_WHY_NAMES = 8
};

/* A class the port never copies, and why (RAC2_COPY_LISTED). */
typedef struct rac2_copy_class {
    int16_t oclass;
    const char* what;
} rac2_copy_class;

extern const rac2_copy_class rac2_copy_never[];
extern const unsigned rac2_copy_never_count;

/* The level's mobys, as play.c reads them: the first, the first the game made (past the placed
 * ones) and the end. */
typedef struct rac2_moby_table {
    gaddr base;
    gaddr dynamic;
    gaddr end;
} rac2_moby_table;

/* Whether the placed moby at `moby` may be copied (RAC2_COPY_*). */
int rac2_copy_check(const rac2_moby_table* table, gaddr moby);

/* Why a moby rac2_copy_check calls scripted is (RAC2_COPY_WHY_* bits), 0 for another answer. */
int rac2_copy_detail(const rac2_moby_table* table, gaddr moby);

/* A copy of the placed moby at `moby`, its `index`-th (0 for its first), at a clear spot beside
 * it: its address, or 0 (not one that may be copied, no clear spot, no room in the pool). */
gaddr rac2_copy_make(const rac2_moby_table* table, gaddr moby, int index);

/* Whether the moby at `moby` is a live copy (rac2_copy_make's) of this load. */
int rac2_copy_is(gaddr moby);

/* The copies made since the level's last load. */
int rac2_copy_count(void);

#endif /* OPENRAC_RAC2_ENEMY_COPIES_H */
