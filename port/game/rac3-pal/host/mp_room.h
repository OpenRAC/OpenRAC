/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's online room, owned by the port (mp_room.c): who is in the room (up to 24),
 * each player's name, clan, team, skin and READY, the host's START and its countdown, and the
 * match's results. mp_net.c drives it with the server's party packets and the port's relay
 * messages; mp_ui.c draws it; at START it writes what the map and the match read into the game's
 * settings object (docs/handoff/notes/rac3-mp-wide-rooms.md, section 9). */
#ifndef OPENRAC_RAC3_MP_ROOM_H
#define OPENRAC_RAC3_MP_ROOM_H

#include <stdint.h>

#include "openrac/guest.h"

#define RAC3_ROOM_MAX 24  /* the most players a room holds (the server's and the port's tables) */
#define RAC3_ROOM_NAME 16 /* a name as the game keeps it: 15 characters and the end */
#define RAC3_ROOM_CLAN 8  /* a clan tag: the game prints 4 */

/* One player of the room, by DME client index (the server's stable slot). */
typedef struct {
    int present; /* in the party */
    int32_t session;
    char name[RAC3_ROOM_NAME];
    char clan[RAC3_ROOM_CLAN];
    int16_t team, skin;
    uint8_t ready;
} rac3_room_player;

/* The room's phase, as every console sees it. */
enum {
    RAC3_ROOM_NONE,     /* not in a room */
    RAC3_ROOM_STAGING,  /* the staging screen: joins, READY, team and skin */
    RAC3_ROOM_STARTING, /* START pressed: the game's countdown to the map */
    RAC3_ROOM_MATCH,    /* the map runs */
    RAC3_ROOM_RESULTS   /* back at level 39's results screen */
};

/* One row of the match's results, for the scoreboard. */
typedef struct {
    char name[33];
    int team, kills, deaths, suicides, score;
} rac3_room_result;

/* ---- Driven by mp_net.c ---- */

/* Whether the native room runs: a party on a disc whose tables can grow (PAL). */
int rac3_room_active(void);
/* The party's members changed (PARTY_INFO, JOINED, LEFT, SLOTS, HOST_CHANGED, END). */
void rac3_room_members_changed(void);
/* A relay message of the room's (RELAY_ROOM_STATE, RELAY_ROOM_REQUEST) from session `from`. */
void rac3_room_receive(int kind, int32_t from, const uint8_t* bytes, int length);
/* Once a DME update, before the objects' changes go out: the room's writes into the settings. */
void rac3_room_frame(void);

/* ---- The player's own choices (mp_ui.c) ---- */

/* READY of this console's player toggled. Returns 1 when the room took the press. */
int rac3_room_toggle_ready(void);
/* This console's player's team and skin. */
void rac3_room_set_team(int team);
void rac3_room_set_skin(int skin);
/* The host's START: 1 when every joiner is ready (the settings are written for the match, the
 * game's own START runs next), 0 when it is refused. */
int rac3_room_start(void);

/* ---- What the screens show (mp_ui.c) ---- */

int rac3_room_phase(void);
/* The room's size, how many are in it, and how many of the joiners are ready (the host counted). */
int rac3_room_width(void);
int rac3_room_count(void);
int rac3_room_ready_count(void);
int rac3_room_is_host(void);
int rac3_room_team_play(void);
/* The seconds left of the START countdown, -1 when none runs. */
int rac3_room_countdown(void);
/* The DME client in slot `slot` (-1 empty), and the player of client `client` (NULL none). */
int rac3_room_slot_client(int slot);
const rac3_room_player* rac3_room_player_of(int client);
/* This console's DME client, -1 outside a room. */
int rac3_room_my_client(void);
/* The results the map left, captured as the results screen opens: count, then each row. */
void rac3_room_capture_results(void);
int rac3_room_result_count(void);
const rac3_room_result* rac3_room_result_at(int n);

/* ---- The game's staging helpers, answered from the room (mp/libraries.tsv) ---- */

/* The row (among the filled slots, in slot order) of this console's `n`-th slot, -1 none. */
int rac3_room_my_row(int n);
/* The slot of row `row`, -1 none. */
int rac3_room_row_slot(int row);

#endif /* OPENRAC_RAC3_MP_ROOM_H */
