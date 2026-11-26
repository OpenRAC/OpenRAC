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

/* An arena whose waves the game draws from a reserve of placed enemies: its controller's group
 * lists them, each waits in `dormant` until the controller puts it in a pod, which carries it in.
 * A member of that group of one of `members` may be copied (not RAC2_COPY_SCRIPTED); its copy goes
 * in with its original (rac2_copy_arenas_tick). The offsets are of the classes' pvars. */
#define RAC2_COPY_ARENA_MEMBERS 4

typedef struct rac2_copy_arena {
    const char* what;
    int16_t controller; /* the controller's class */
    uint16_t group;     /* the s32 of its pvars naming the reserve's group */
    int16_t pod;        /* the class of the pods */
    uint16_t pod_held;  /* the pod's members, a word each, 0 for an empty one */
    uint8_t pod_held_count;
    uint8_t pod_states; /* the pod's states (bits) while it holds them, before it lets go */
    uint8_t dormant;    /* a member's state in the reserve */
    uint8_t held;       /* a member's state in a pod */
    uint8_t health_at;  /* the float a member's health starts from, set to 1 as it goes in */
    int16_t members[RAC2_COPY_ARENA_MEMBERS];
} rac2_copy_arena;

extern const rac2_copy_arena rac2_copy_arenas[];
extern const unsigned rac2_copy_arenas_count;

/* Once a frame (play.c's rac2_play_frame): a copy of an arena's member waiting in the reserve, its
 * original out of it, goes into a pod as the controller puts a member in one (its state the held
 * one, its owner the controller, its health full): the original's pod, or when that one is full
 * another of the controller's drop with room. The wave's counts are left as they are, so the wave
 * ends only once the copy is beaten too. */
void rac2_copy_arenas_tick(void);

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
 * it, with copies of its parts: its address, or 0 (not one that may be copied, no clear spot, no
 * room in the pool, a spawned moby it names that others share). */
gaddr rac2_copy_make(const rac2_moby_table* table, gaddr moby, int index);

/* The `i`-th copy of this load (0 past the last), alive or not, and its original in *orig. */
gaddr rac2_copy_at(int i, gaddr* orig);

/* Set by OPENRAC_PLAY_TRACE's copywatch: each SetDeathBits is printed (PTRACE death lines). */
extern int rac2_copy_trace;

/* Whether the moby at `moby` is a live copy (rac2_copy_make's) of this load. */
int rac2_copy_is(gaddr moby);

/* CreateMoby gave out the slot `moby`: a dead copy's index is taken off the groups' lists. */
void rac2_copy_slot_taken(gaddr moby);

/* The moby at `moby` has just left the live states (+0x20 at 0x80 or over: its own death, or a
 * mod's kill): a copy's index is taken off its group's list at once, so no squad counts or writes
 * to it. */
void rac2_copy_gone(gaddr moby);

/* Once a tick, before the moby loop: every entry of the lists moved to the port's room past the
 * placed mobys that is no longer its live copy (dead, or its slot given out to a moby of another
 * class by a path rac2_copy_slot_taken did not see) is taken off. A squad's controller writes its
 * alerts at +0x244 and +0x2D0 of each member's pvars: on a slot given out again, past the 0x80
 * bytes CreateMoby gives it, into the next slots' pvars; and its counts of live members never
 * reached zero. */
void rac2_copy_groups_tidy(void);

/* The pvars the port made for the live copy at `moby`, as rac2_copy_make laid them out (in its
 * slot's 0x80 bytes or the port's room), and their size in *bytes; 0 for any other moby, or a copy
 * whose pvars word no longer names them. For the port's own checks (weapon_culling.c): a copy's
 * pvars in the port's memory are its own, not a stray word. */
gaddr rac2_copy_pvars(gaddr moby, uint32_t* bytes);

/* The copies made since the level's last load. */
int rac2_copy_count(void);

#endif /* OPENRAC_RAC2_ENEMY_COPIES_H */
