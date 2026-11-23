/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's online rooms past 8 players: the game's per-player tables grown to 24.
 *
 * The game sizes its per-player state for 8 clients: arrays of 8 in its net state block (the
 * client of each slot, each slot's last receive time, ...), the table of remote players (8 x
 * 0xC70), and the arrays of 8 in the room's settings object (OBJECT_GAME_SETTINGS: names, teams,
 * clients, states, account ids). The port's relay and the server hold 24 (protocol.md 7.4).
 *
 * The game's code stays as it is; only where it indexes one of those tables does the port step in.
 * The first 8 entries of every table stay where the game keeps them, so a room of 8 is exactly
 * what it was, and any code that only ever looks at slots 0-7 sees what it saw. Entries 8-23 live
 * in tables of the port's own:
 *
 * - the net block's arrays and the remote players' table in the port's statics (game memory the
 *   game's code reaches like its own);
 * - the settings object's arrays in a second net object, OBJECT_GAME_SETTINGS_WIDE (mp_net.c),
 *   which the host owns and publishes beside the game's object, with slots 8-23 of each array in
 *   the same order (rac3_mp_wide_object), so every console has the same settings for every slot.
 *
 * mp/wide.json (written by mp/wide.py) names every instruction of the game that indexes one of
 * those tables: hostgen translates it with its address through rac3_mp_wide_address, and every
 * bound of a loop or check over the slots through rac3_mp_wide_bound, which gives the room's count
 * (rac3_mp_wide_set_slots: 8, 16 or 24) for the game's 8. In a room of 8 both are the identity.
 *
 * Only the PAL disc's tables are known (NTSC-U keeps 8: rac3_mp_wide_set_slots stays at 8). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mp_wide.h"
#include "openrac/guest.h"
#include "rac3_version.h"

#define WIDE_GAME 8  /* the game's own count */
#define WIDE_MAX 24  /* the most the port gives a room */
#define WIDE_EXTRA (WIDE_MAX - WIDE_GAME)

/* The settings object's data pointer ($gp - 0x6980, set by its on_create callback 0x00199968). */
#define GS_POINTER 0x00241640u

/* Each widened table: in game memory at `base` (or, with `gs` set, at that offset of the settings
 * object's data), `stride` bytes an entry. The order is mp/wide.json's "tables" (wide.py checks). */
/* clang-format off */
#define WIDE_TABLES(X) \
    X(NET_CLIENT_BY_SLOT, 0, 0x001A5E18u, 4)   /* net + 0x148: the DME client of each slot */ \
    X(NET_RECEIVE_TIME, 0, 0x001A635Cu, 4)     /* net + 0x68C: each slot's last message, net time */ \
    X(NET_SLOT_BY_CLIENT, 0, 0x001A637Du, 1)   /* net + 0x6AD: the slot of each client index */ \
    X(NET_STATE_TIME, 0, 0x001A6480u, 4)       /* net + 0x7B0: each slot's last player state */ \
    X(NET_SLOT_BYTES, 0, 0x001A64A5u, 1)       /* net + 0x7D5: a byte per slot, cleared per session */ \
    X(NET_SLOT_NAMES, 0, 0x001A64ADu, 0x10)    /* net + 0x7DD: a name per slot */ \
    X(NET_SLOT_REQUESTS, 0, 0x001A6310u, 4)    /* net + 0x640: the host's slot requests */ \
    X(NET_SLOT_VOICE, 0, 0x001A6330u, 4)       /* net + 0x660: each slot's voice stream state */ \
    X(NET_PLAYERS, 0, 0x0022F3D0u, 0xC70)      /* the remote players (jitter buffer at +0x1F0) */ \
    X(GS_NAME, 1, 0x000u, 16)                  /* OBJECT_GAME_SETTINGS: name of each slot */ \
    X(GS_CLAN, 1, 0x080u, 8)                   /* clan tag */ \
    X(GS_SKIN, 1, 0x0C0u, 2)                   /* skin */ \
    X(GS_TEAM, 1, 0x0D0u, 2)                   /* team */ \
    X(GS_CLIENT, 1, 0x0E0u, 2)                 /* owning DME client, -1 empty */ \
    X(GS_STATE, 1, 0x0F0u, 2)                  /* slot state (joined, ready, loaded, ...) */ \
    X(GS_RESERVED, 1, 0x100u, 2)               /* reservation (the host's) */ \
    X(GS_VALUE_A, 1, 0x110u, 4)                /* per-mode values from the join request */ \
    X(GS_VALUE_B, 1, 0x130u, 4) \
    X(GS_ACCOUNT, 1, 0x150u, 4)                /* account id, -1 empty */
/* clang-format on */

enum {
#define WIDE_ID(name, gs, base, stride) WIDE_##name,
    WIDE_TABLES(WIDE_ID)
#undef WIDE_ID
        WIDE_TABLE_COUNT
};

static const struct {
    const char* name;
    int gs;
    uint32_t base, stride;
} wide_table[WIDE_TABLE_COUNT] = {
#define WIDE_ROW(name, gs, base, stride) {#name, gs, base, stride},
    WIDE_TABLES(WIDE_ROW)
#undef WIDE_ROW
};

static int wide_slots = WIDE_GAME;
static gaddr wide_ext[WIDE_TABLE_COUNT];  /* the port's tables of entries 8-23 (net block, players) */
static gaddr wide_object;                  /* OBJECT_GAME_SETTINGS_WIDE's data on this console */

/* Where each settings array's slots 8-23 sit in OBJECT_GAME_SETTINGS_WIDE: the arrays in their
 * order, each WIDE_EXTRA entries long. */
static uint32_t wide_object_offset(int table) {
    uint32_t off = 0;
    for (int t = 0; t < table; t++) {
        if (wide_table[t].gs) {
            off += wide_table[t].stride * WIDE_EXTRA;
        }
    }
    return off;
}

uint32_t rac3_mp_wide_object_size(void) {
    return wide_object_offset(WIDE_TABLE_COUNT);
}

/* The empty slots 8-23 of a new OBJECT_GAME_SETTINGS_WIDE: no client, no account, as the game
 * leaves its own empty slots (0x001970F8). */
void rac3_mp_wide_object_clear(gaddr data) {
    memset(G(data), 0, rac3_mp_wide_object_size());
    for (int i = 0; i < WIDE_EXTRA; i++) {
        GREF(int16_t, data + wide_object_offset(WIDE_GS_CLIENT) + 2u * (uint32_t)i) = -1;
        GREF(int32_t, data + wide_object_offset(WIDE_GS_ACCOUNT) + 4u * (uint32_t)i) = -1;
    }
}

void rac3_mp_wide_object(gaddr data) {
    wide_object = data;
}

int rac3_mp_wide_available(void) {
#ifdef RAC3_PAL
    return 1;
#else
    return 0;
#endif
}

void rac3_mp_wide_set_slots(int slots) {
    if (!rac3_mp_wide_available() || slots < WIDE_GAME) {
        slots = WIDE_GAME;
    }
    if (slots > WIDE_MAX) {
        slots = WIDE_MAX;
    }
    if (slots != wide_slots) {
        fprintf(stderr, "[info] rac3: online: the room's tables hold %d players\n", slots);
    }
    wide_slots = slots;
}

int rac3_mp_wide_slots(void) {
    return wide_slots;
}

/* A bound of the game's over its slots (8; 7 for "the last slot", ...): the room's. */
int rac3_mp_wide_bound(int game) {
    return game + wide_slots - WIDE_GAME;
}

/* The bytes of a loop over a table's 8 entries (its end pointer): the room's entries. */
int rac3_mp_wide_span(int game) {
    return game / WIDE_GAME * wide_slots;
}

/* An address the game formed into one of the tables, from the game's base: entries 0-7 where the
 * game has them, 8-23 in the port's table. */
uint32_t rac3_mp_wide_address(uint32_t address, int table) {
    if (table < 0 || table >= WIDE_TABLE_COUNT) {
        return address;
    }
    uint32_t base = wide_table[table].base;
    const uint32_t stride = wide_table[table].stride;
    if (wide_table[table].gs) {
        const uint32_t gs = GREF(uint32_t, GS_POINTER);
        if (gs == 0) {
            return address;
        }
        base += gs;
    }
    const uint32_t rel = address - base;
    if (rel < WIDE_GAME * stride || rel >= WIDE_MAX * stride) {
        return address;
    }
    if (wide_table[table].gs) {
        if (wide_object == 0) {
            /* No OBJECT_GAME_SETTINGS_WIDE yet (a console that has the game's object but not the
             * port's): slots 8-23 go to a table of their own, never into the object's next field. */
            static gaddr spare;
            if (spare == 0) {
                spare = openrac_guest_static(rac3_mp_wide_object_size());
                rac3_mp_wide_object_clear(spare);
            }
            return spare + wide_object_offset(table) + rel - WIDE_GAME * stride;
        }
        return wide_object + wide_object_offset(table) + rel - WIDE_GAME * stride;
    }
    if (wide_ext[table] == 0) {
        wide_ext[table] = openrac_guest_static(WIDE_EXTRA * stride);
        memset(G(wide_ext[table]), 0, WIDE_EXTRA * stride);
    }
    return wide_ext[table] + rel - WIDE_GAME * stride;
}

/* A new session: the port's net-block entries 8-23 cleared as the game clears its own. */
void rac3_mp_wide_reset(void) {
    for (int t = 0; t < WIDE_TABLE_COUNT; t++) {
        if (wide_ext[t] != 0) {
            memset(G(wide_ext[t]), 0, WIDE_EXTRA * wide_table[t].stride);
        }
    }
}
