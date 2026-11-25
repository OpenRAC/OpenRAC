/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's online rooms past 8 players: the game's per-player tables grown to 24.
 *
 * The game sizes its per-player state for 8 clients: arrays of 8 in its net state block (the
 * client of each slot, each slot's last receive time, ...), the table of remote players (8 x
 * 0xC70), the arrays of 8 in the room's settings object (OBJECT_GAME_SETTINGS: names, teams,
 * clients, states, account ids), and in a match each program's players array, the arrays of its
 * game-mode struct (scores, the mode's columns, per-weapon counts) and the results block the map
 * leaves for level 39's results screen. The port's relay and the server hold 24 (protocol.md 7.4).
 *
 * The game's code stays as it is; only where it indexes one of those tables does the port step in.
 * The first 8 entries of every table stay where the game keeps them, so a room of 8 is exactly
 * what it was, and any code that only ever looks at slots 0-7 sees what it saw. Entries 8-23 live
 * in tables of the port's own:
 *
 * - the net block's arrays, the remote players' table, the players array, the game-mode arrays and
 *   the results block's arrays in the port's statics (game memory the game's code reaches like its
 *   own);
 * - the settings object's arrays in a second net object, OBJECT_GAME_SETTINGS_WIDE (mp_net.c),
 *   which the host owns and publishes beside the game's object, with slots 8-23 of each array in
 *   the same order (rac3_mp_wide_object), so every console has the same settings for every slot.
 *
 * mp/wide.json (written by mp/wide.py) names every instruction of the game that indexes one of
 * those tables: hostgen translates it with its address through rac3_mp_wide_address, and every
 * bound of a loop or check over the slots through rac3_mp_wide_bound, which gives the room's count
 * (rac3_mp_wide_set_slots: 8 to 24) for the game's 8. In a room of 8 both are the identity.
 *
 * The players array and the game-mode struct are each program's own (level 39 and every map has
 * its copy): their tables take the base of the program whose code runs. */
#include "mp_wide.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

#define WIDE_GAME 8 /* the game's own count */
#define WIDE_MAX 24 /* the most the port gives a room */
#define WIDE_EXTRA (WIDE_MAX - WIDE_GAME)

/* The settings object's data pointer ($gp - 0x6980, set by its on_create callback 0x00199968). */
#define GS_POINTER RAC3_MADDR(0x00241640u)
/* The end-of-match results the map leaves for level 39 (0x82C bytes in the core, the same address
 * in every program; filled by the map's end routine, L39_00670D90's copy). */
#define RESULTS RAC3_MADDR(0x001FD7E8u)

/* Where a table's entries are: at a fixed address, at an offset of the settings object's data, of
 * the program's game-mode struct, or of its players array. */
enum {
    AT,
    IN_GS,
    IN_MODE,
    IN_PLAYERS
};

/* Each widened table: `base` (an address, or an offset into what `where` names), `stride` bytes an
 * entry. The order is mp/wide.json's "tables" (wide.py checks). */
/* clang-format off */
#define WIDE_TABLES(X) \
    X(NET_CLIENT_BY_SLOT, AT, RAC3_MADDR(0x001A5E18u), 4) /* net + 0x148: the DME client of each slot */ \
    X(NET_RECEIVE_TIME, AT, RAC3_MADDR(0x001A635Cu), 4)   /* net + 0x68C: each slot's last message */ \
    X(NET_SLOT_BY_CLIENT, AT, RAC3_MADDR(0x001A637Du), 1) /* net + 0x6AD: the slot of each client */ \
    X(NET_STATE_TIME, AT, RAC3_MADDR(0x001A6480u), 4)     /* net + 0x7B0: each slot's last state */ \
    X(NET_SLOT_BYTES, AT, RAC3_MADDR(0x001A64A5u), 1)     /* net + 0x7D5: a byte per slot */ \
    X(NET_SLOT_NAMES, AT, RAC3_MADDR(0x001A64ADu), 0x10)  /* net + 0x7DD: a name per slot */ \
    X(NET_SLOT_REQUESTS, AT, RAC3_MADDR(0x001A6310u), 4)  /* net + 0x640: the host's slot requests */ \
    X(NET_SLOT_VOICE, AT, RAC3_MADDR(0x001A6330u), 4)     /* net + 0x660: each slot's voice stream */ \
    X(NET_PLAYERS, AT, RAC3_MADDR(0x0022F3D0u), 0xC70)    /* the remote players (jitter buffer +0x1F0) */ \
    X(GS_NAME, IN_GS, 0x000u, 16)                         /* OBJECT_GAME_SETTINGS: each slot's name */ \
    X(GS_CLAN, IN_GS, 0x080u, 8)                          /* clan tag */ \
    X(GS_SKIN, IN_GS, 0x0C0u, 2)                          /* skin */ \
    X(GS_TEAM, IN_GS, 0x0D0u, 2)                          /* team */ \
    X(GS_CLIENT, IN_GS, 0x0E0u, 2)                        /* owning DME client, -1 empty */ \
    X(GS_STATE, IN_GS, 0x0F0u, 2)                         /* slot state (joined, ready, loaded, ...) */ \
    X(GS_RESERVED, IN_GS, 0x100u, 2)                      /* reservation (the host's) */ \
    X(GS_VALUE_A, IN_GS, 0x110u, 4)                       /* per-mode values from the join request */ \
    X(GS_VALUE_B, IN_GS, 0x130u, 4) \
    X(GS_ACCOUNT, IN_GS, 0x150u, 4)                       /* account id, -1 empty */ \
    X(MAP_PLAYERS, IN_PLAYERS, 0x000u, 4)                 /* the program's players, by slot */ \
    X(MODE_SCORE, IN_MODE, 0x018u, 4)                     /* game mode: kills and deaths, s16 each */ \
    X(MODE_COLUMN_A, IN_MODE, 0x038u, 1)                  /* the modes' columns (siege, CTF) */ \
    X(MODE_COLUMN_B, IN_MODE, 0x040u, 1) \
    X(MODE_COLUMN_C, IN_MODE, 0x048u, 1) \
    X(MODE_COLUMN_D, IN_MODE, 0x050u, 1) \
    X(MODE_SHARE, IN_MODE, 0x058u, 4)                     /* a float a slot (siege's share) */ \
    X(MODE_WEAPONS, IN_MODE, 0x078u, 0x24)                /* kills and deaths per weapon */ \
    X(MODE_SUICIDES, IN_MODE, 0x198u, 2) \
    X(RESULT_NAME, AT, RESULTS + 0x000u, 0x20)            /* the results: each slot's name */ \
    X(RESULT_FLAG, AT, RESULTS + 0x100u, 4) \
    X(RESULT_TEAM, AT, RESULTS + 0x120u, 4) \
    X(RESULT_SCORE, AT, RESULTS + 0x140u, 4)              /* kills and deaths, s16 each */ \
    X(RESULT_COLUMN_A, AT, RESULTS + 0x160u, 1) \
    X(RESULT_COLUMN_B, AT, RESULTS + 0x168u, 1) \
    X(RESULT_COLUMN_C, AT, RESULTS + 0x170u, 1) \
    X(RESULT_COLUMN_D, AT, RESULTS + 0x178u, 1) \
    X(RESULT_SHARE, AT, RESULTS + 0x180u, 4) \
    X(RESULT_SUICIDES, AT, RESULTS + 0x1B8u, 2) \
    X(RESULT_WEAPONS, AT, RESULTS + 0x1C8u, 0x24)
/* clang-format on */

enum {
#define WIDE_ID(name, where, base, stride) WIDE_##name,
    WIDE_TABLES(WIDE_ID)
#undef WIDE_ID
    WIDE_TABLE_COUNT
};

static const struct {
    const char* name;
    int where;
    uint32_t base, stride;
} wide_table[WIDE_TABLE_COUNT] = {

#define WIDE_ROW(name, where, base, stride) {#name, where, base, stride},
    WIDE_TABLES(WIDE_ROW)
#undef WIDE_ROW
};

/* Each program's players array (8 pointers, by slot) and game-mode struct (ui-scoreboard.md 2.3):
 * level 39 and the maps 40-53 (the split-screen 54 and 55 are never online). */
#define PROGRAM_FIRST 39
#define PROGRAM_LAST 53

static const struct {
    uint32_t players, mode;
} wide_program[PROGRAM_LAST - PROGRAM_FIRST + 1] = {
    {RAC3_LADDR(39, 0x0024CE20u), RAC3_LADDR(39, 0x00352940u)},
    {RAC3_LADDR(40, 0x00249330u), RAC3_LADDR(40, 0x00357260u)},
    {RAC3_LADDR(41, 0x00249530u), RAC3_LADDR(41, 0x003573A0u)},
    {RAC3_LADDR(42, 0x00249420u), RAC3_LADDR(42, 0x0034F2A0u)},
    {RAC3_LADDR(43, 0x002492A0u), RAC3_LADDR(43, 0x0034F120u)},
    {RAC3_LADDR(44, 0x00249320u), RAC3_LADDR(44, 0x0034F160u)},
    {RAC3_LADDR(45, 0x002492A0u), RAC3_LADDR(45, 0x0034F120u)},
    {RAC3_LADDR(46, 0x00248EA0u), RAC3_LADDR(46, 0x0035FCE0u)},
    {RAC3_LADDR(47, 0x00248FA0u), RAC3_LADDR(47, 0x0035FE20u)},
    {RAC3_LADDR(48, 0x00248FA0u), RAC3_LADDR(48, 0x0035FE60u)},
    {RAC3_LADDR(49, 0x00248FA0u), RAC3_LADDR(49, 0x0035FDE0u)},
    {RAC3_LADDR(50, 0x00249330u), RAC3_LADDR(50, 0x00368260u)},
    {RAC3_LADDR(51, 0x00249530u), RAC3_LADDR(51, 0x003683A0u)},
    {RAC3_LADDR(52, 0x00249420u), RAC3_LADDR(52, 0x003602A0u)},
    {RAC3_LADDR(53, 0x002492A0u), RAC3_LADDR(53, 0x00360120u)},
};

static int wide_slots = WIDE_GAME;
static gaddr
    wide_ext[WIDE_TABLE_COUNT]; /* the port's tables of entries 8-23 (all but the settings') */
static gaddr wide_object;       /* OBJECT_GAME_SETTINGS_WIDE's data on this console */
static int wide_overlay = -1;   /* the program the per-program tables were last used in */

/* Where each settings array's slots 8-23 sit in OBJECT_GAME_SETTINGS_WIDE: the arrays in their
 * order, each WIDE_EXTRA entries long. */
static uint32_t wide_object_offset(int table) {
    uint32_t off = 0;
    for (int t = 0; t < table; t++) {
        if (wide_table[t].where == IN_GS) {
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
    /* NTSC-U's sites and addresses are not carried yet (wide.json, release.json): its rooms stay at 8. */
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

static gaddr wide_extension(int table) {
    if (wide_ext[table] == 0) {
        wide_ext[table] = openrac_guest_static(WIDE_EXTRA * wide_table[table].stride);
        memset(G(wide_ext[table]), 0, WIDE_EXTRA * wide_table[table].stride);
    }
    return wide_ext[table];
}

/* A new program (a map loaded, level 39 again): the players array's and the game-mode struct's
 * entries 8-23 are the new program's, empty as the game's own are when it starts; at a map's start
 * the results' too (the map's end routine fills them, after its memset of the game's block). */
static void wide_program_changed(int overlay) {
    for (int t = 0; t < WIDE_TABLE_COUNT; t++) {
        const int program = wide_table[t].where == IN_PLAYERS || wide_table[t].where == IN_MODE;
        const int results = t >= WIDE_RESULT_NAME && overlay > PROGRAM_FIRST;
        if ((program || results) && wide_ext[t] != 0) {
            memset(G(wide_ext[t]), 0, WIDE_EXTRA * wide_table[t].stride);
        }
    }
    wide_overlay = overlay;
}

/* The game's base of a table: an address, or 0 where the table has none now (no settings object,
 * a program without the table). */
static uint32_t wide_base(int table) {
    const uint32_t base = wide_table[table].base;
    switch (wide_table[table].where) {
        case IN_GS:
            return GREF(uint32_t, GS_POINTER) != 0 ? GREF(uint32_t, GS_POINTER) + base : 0;
        case IN_MODE:
        case IN_PLAYERS: {
            const int overlay = openrac_game_loaded_overlay();
            if (overlay < PROGRAM_FIRST || overlay > PROGRAM_LAST) {
                return 0;
            }
            if (overlay != wide_overlay) {
                wide_program_changed(overlay);
            }
            const uint32_t at = wide_table[table].where == IN_MODE
                                    ? wide_program[overlay - PROGRAM_FIRST].mode
                                    : wide_program[overlay - PROGRAM_FIRST].players;
            return at != 0 ? at + base : 0;
        }
        default:
            return base;
    }
}

/* An address the game formed into one of the tables, from the game's base: entries 0-7 where the
 * game has them, 8-23 in the port's table. */
uint32_t rac3_mp_wide_address(uint32_t address, int table) {
    if (table < 0 || table >= WIDE_TABLE_COUNT) {
        return address;
    }
    const uint32_t base = wide_base(table);
    const uint32_t stride = wide_table[table].stride;
    if (base == 0) {
        return address;
    }
    const uint32_t rel = address - base;
    if (rel < WIDE_GAME * stride || rel >= WIDE_MAX * stride) {
        return address;
    }
    if (wide_table[table].where == IN_GS) {
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
    return wide_extension(table) + rel - WIDE_GAME * stride;
}

/* A new session: the port's net-block entries 8-23 cleared as the game clears its own. */
void rac3_mp_wide_reset(void) {
    for (int t = 0; t < WIDE_TABLE_COUNT; t++) {
        if (wide_ext[t] != 0 && wide_table[t].where == AT && t < WIDE_RESULT_NAME) {
            memset(G(wide_ext[t]), 0, WIDE_EXTRA * wide_table[t].stride);
        }
    }
}

/* Slot `slot`'s entry of the settings array at `offset` of the settings object's data (+0x000
 * names, +0x0E0 clients, ...), wherever the port keeps it; 0 without a settings object. */
gaddr rac3_mp_wide_settings(uint32_t offset, int slot) {
    for (int t = 0; t < WIDE_TABLE_COUNT; t++) {
        if (wide_table[t].where == IN_GS && wide_table[t].base == offset) {
            const uint32_t base = wide_base(t);
            return base == 0
                       ? 0
                       : rac3_mp_wide_address(base + wide_table[t].stride * (uint32_t)slot, t);
        }
    }
    return 0;
}

/* Slot `slot`'s entry of the results' array at `offset` of the results block (+0x000 names, +0x120
 * teams, +0x140 kills and deaths, +0x1B8 suicides, ...), wherever the port keeps it. */
gaddr rac3_mp_wide_results(uint32_t offset, int slot) {
    for (int t = WIDE_RESULT_NAME; t < WIDE_TABLE_COUNT; t++) {
        if (wide_table[t].base == RESULTS + offset) {
            return rac3_mp_wide_address(
                RESULTS + offset + wide_table[t].stride * (uint32_t)slot, t
            );
        }
    }
    return RESULTS + offset;
}

/* The game-mode struct's array at `offset` (+0x18 kills and deaths, +0x38/+0x48/+0x50 the modes'
 * columns, +0x198 suicides), slots `first` to `first + count - 1`, copied out (`out`) or in. The
 * channel 5 end-of-match message carries slots 0-7 of these; the port's companion the others. */
int rac3_mp_wide_mode_copy(uint32_t offset, int first, int count, uint8_t* bytes, int out) {
    for (int t = 0; t < WIDE_TABLE_COUNT; t++) {
        if (wide_table[t].where == IN_MODE && wide_table[t].base == offset) {
            const uint32_t base = wide_base(t);
            const uint32_t stride = wide_table[t].stride;
            if (base == 0) {
                return 0;
            }
            for (int i = 0; i < count; i++) {
                const gaddr at = rac3_mp_wide_address(base + stride * (uint32_t)(first + i), t);
                if (out) {
                    memcpy(bytes + stride * (uint32_t)i, G(at), stride);
                } else {
                    memcpy(G(at), bytes + stride * (uint32_t)i, stride);
                }
            }
            return (int)stride * count;
        }
    }
    return 0;
}

/* ---- The end of a match (L39_00670D90's copy in each map) ---- */

/* The end routine fills the results block for the game's eight slots only: names from the
 * settings, the player's flag and team, and from the game-mode struct the scores and the mode's
 * columns, and decides the winner (+0x1A8). The wrap runs after it and does the same for slots 8 and
 * up, from the port's tables, then decides the winner again over every slot. */
#define RES_FLAG 0x100u
#define RES_TEAM 0x120u
#define RES_SCORE 0x140u
#define RES_SUICIDES 0x1B8u
#define RES_WINNER 0x1A8u
#define RES_TEAMS 0x2E8u /* eight team records of 0xA4 bytes: idx[8], a[8], b[8], ..., count at +0x8C */
#define RES_TEAM_BYTES 0xA4u
#define RES_TEAM_COUNT 0x8Cu
#define RES_GAME_TYPE 0x80Cu
#define PLAYER_LOCAL 0x1A14u   /* a byte: this console's player */
#define PLAYER_PROFILE 0x2534u /* the word the local player's profile reset takes */
#define PLAYER_TEAM 0x2548u

/* The game-mode struct's arrays the end copies into the results, and the results' array of each
 * (offset in the mode, offset in the results, bytes an entry). The first five are also what the
 * channel 5 end message carries for slots 0-7; the port's relay message carries the others'. */
static const struct {
    uint32_t mode, result, bytes;
    int sent;
} wide_end_array[] = {
    {0x018u, 0x140u, 4, 1},
    {0x198u, 0x1B8u, 2, 1},
    {0x038u, 0x160u, 1, 1},
    {0x048u, 0x170u, 1, 1},
    {0x050u, 0x178u, 1, 1},
    {0x040u, 0x168u, 1, 0},
    {0x058u, 0x180u, 4, 0},
    {0x078u, 0x1C8u, 0x24, 0},
};
#define WIDE_END_ARRAYS (sizeof wide_end_array / sizeof wide_end_array[0])

/* The players' slot `slot` struct (0: none). */
static gaddr wide_player(int slot) {
    const uint32_t base = wide_base(WIDE_MAP_PLAYERS);
    return base == 0 ? 0
                     : GREF(gaddr, rac3_mp_wide_address(base + 4u * (uint32_t)slot, WIDE_MAP_PLAYERS));
}

/* The bytes the end message carries for slots 8 to slots-1 (at most 24 - 8 slots of each array
 * the game's own message has: scores 4, suicides 2, three columns of 1). */
int rac3_mp_wide_end_pack(uint8_t* out, int capacity) {
    if (wide_slots <= WIDE_GAME) {
        return 0;
    }
    const int count = wide_slots - WIDE_GAME;
    int n = 0;
    for (size_t a = 0; a < WIDE_END_ARRAYS; a++) {
        if (!wide_end_array[a].sent) {
            continue;
        }
        if (n + (int)wide_end_array[a].bytes * count > capacity
            || rac3_mp_wide_mode_copy(wide_end_array[a].mode, WIDE_GAME, count, out + n, 1) == 0) {
            return 0;
        }
        n += (int)wide_end_array[a].bytes * count;
    }
    return n;
}

/* The same, received: the room's width first (a console with another width ignores it). */
void rac3_mp_wide_end_unpack(const uint8_t* in, int length) {
    if (wide_slots <= WIDE_GAME) {
        return;
    }
    const int count = wide_slots - WIDE_GAME;
    int n = 0;
    for (size_t a = 0; a < WIDE_END_ARRAYS; a++) {
        if (!wide_end_array[a].sent) {
            continue;
        }
        if (n + (int)wide_end_array[a].bytes * count > length) {
            return;
        }
        uint8_t copy[WIDE_EXTRA * 4];
        memcpy(copy, in + n, (size_t)wide_end_array[a].bytes * (size_t)count);
        rac3_mp_wide_mode_copy(wide_end_array[a].mode, WIDE_GAME, count, copy, 0);
        n += (int)wide_end_array[a].bytes * count;
    }
}

/* The winner of a deathmatch, over every slot (L39_00670D90's choice, which sees eight). Teams:
 * the team with the most kills less suicides, fewer deaths on a tie, a tie of both none (-1);
 * else the same for the players, the winner being the best one's team. */
static int wide_winner(int teams) {
    const gaddr gs = GREF(gaddr, GS_POINTER);
    int best_index = -1, best1 = -1, best2 = -1, tie = 0;
    for (int t = 0; t < (teams ? 8 : wide_slots); t++) {
        int sum1 = 0, sum2 = 0, any = 0;
        for (int j = teams ? 0 : t; j < (teams ? wide_slots : t + 1); j++) {
            const gaddr client = rac3_mp_wide_settings(0x0E0u, j);
            if (client == 0 || GREF(int16_t, client) == -1) {
                continue;
            }
            if (teams && GREF(int16_t, rac3_mp_wide_settings(0x0D0u, j)) != t) {
                continue;
            }
            const gaddr score = rac3_mp_wide_results(RES_SCORE, j);
            sum1 += GREF(int16_t, score) - GREF(int16_t, rac3_mp_wide_results(RES_SUICIDES, j));
            sum2 += GREF(int16_t, score + 2u);
            any = 1;
        }
        if (!teams && !any) {
            continue;
        }
        if (best1 < sum1 || (sum1 == best1 && sum2 < best2)) {
            best1 = sum1;
            best2 = sum2;
            best_index = t;
            tie = 0;
        } else if (sum1 == best1 && sum2 == best2) {
            tie = 1;
        }
    }
    (void)gs;
    if (tie || best_index < 0) {
        return -1;
    }
    return teams ? best_index : GREF(int32_t, rac3_mp_wide_results(RES_TEAM, best_index));
}

void rac3_mp_wide_end_results(int reason, int (*profile_reset)(int)) {
    const gaddr gs = GREF(gaddr, GS_POINTER);
    if (wide_slots <= WIDE_GAME || gs == 0) {
        return;
    }
    for (int i = WIDE_GAME; i < wide_slots; i++) {
        const gaddr name = rac3_mp_wide_results(0x000u, i);
        const gaddr from = rac3_mp_wide_settings(0x000u, i);
        memset(G(name), 0, 0x20);
        if (GREF(uint8_t, from) != 0) {
            memcpy(G(name), G(from), 15); /* the game's strcpy of a 16-byte name */
        }
        GREF(int32_t, rac3_mp_wide_results(RES_FLAG, i)) = 0;
        const gaddr player = wide_player(i);
        if (player != 0) {
            const int32_t team = GREF(int32_t, player + PLAYER_TEAM);
            if (GREF(uint8_t, player + PLAYER_LOCAL) != 0) {
                GREF(int32_t, rac3_mp_wide_results(RES_FLAG, i)) = 1;
                profile_reset(GREF(int32_t, player + PLAYER_PROFILE));
            }
            GREF(int32_t, rac3_mp_wide_results(RES_TEAM, i)) = team;
            if (team >= 0 && team < 8) {
                const gaddr record = RESULTS + RES_TEAMS + RES_TEAM_BYTES * (gaddr)team;
                const int32_t n = GREF(int32_t, record + RES_TEAM_COUNT);
                /* The record lists eight members; a team of more lists the first eight. */
                if (n >= 0 && n < 8) {
                    GREF(float, record + 0x20u + 4u * (gaddr)n) =
                        GREF(float, rac3_mp_wide_settings(0x110u, i));
                    GREF(float, record + 0x40u + 4u * (gaddr)n) =
                        GREF(float, rac3_mp_wide_settings(0x130u, i));
                    GREF(int32_t, record + 4u * (gaddr)n) = i;
                    GREF(int32_t, record + RES_TEAM_COUNT) = n + 1;
                }
            }
        }
        for (size_t a = 0; a < WIDE_END_ARRAYS; a++) {
            uint8_t bytes[0x24];
            rac3_mp_wide_mode_copy(wide_end_array[a].mode, i, 1, bytes, 1);
            memcpy(
                G(rac3_mp_wide_results(wide_end_array[a].result, i)), bytes, wide_end_array[a].bytes
            );
        }
    }
    /* Deathmatch (type 2) ends 1 and 5 name the winner from the scores: over every slot now. */
    if ((reason == 1 || reason == 5) && GREF(int32_t, RESULTS + RES_GAME_TYPE) == 2) {
        GREF(int32_t, RESULTS + RES_WINNER) = wide_winner(GREF(int8_t, gs + 0x19Du) != 0);
    }
}
