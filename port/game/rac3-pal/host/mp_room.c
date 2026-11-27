/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's online room, owned by the port: up to 24 players, their names, clans, teams,
 * skins and READY, the host's START and its countdown, and the match's results. The design is in
 * docs/handoff/notes/rac3-mp-wide-rooms.md, section 9.
 *
 * Who is in the room is the server's (its party: PARTY_INFO, JOINED, LEFT, SLOTS, HOST_CHANGED);
 * each member's DME client index is its stable slot there. The slot of the settings object a client
 * plays in is the one DME's admission gave it (the core's session-add handler, 0x0019A6F8, which also
 * fills the net block's per-slot tables the match reads): the room reads it from the settings'
 * client words each frame and keys everything else by the client.
 *
 * Everything else is the room's, and the host's room decides it:
 *
 * - a player's name is its account's, its clan its own, its team and skin its choices (a new
 *   player starts with the ones the admission gave it);
 * - READY: a Circle on the staging screen (mp_ui.c), on any console; a joiner asks the host's room
 *   (RELAY_ROOM_REQUEST), whose answer every console gets in the room's state (RELAY_ROOM_STATE);
 *   a new player in the room clears every joiner's READY, as the game's own rule does;
 * - START: the host's, when every joiner is ready; the room then writes the settings object for
 *   the match and the game's own START runs (its countdown, +0x174, is the room's countdown);
 * - the results: the map leaves them in the results block (and the port's companions past 8), the
 *   room keeps them for the scoreboard.
 *
 * The settings object (OBJECT_GAME_SETTINGS and its companion past 8 slots) is what the map and the
 * match read: the host's room writes each slot's name, clan, team, skin and state there as they
 * change, and all of them again at START; the host publishes the object as ever (mp_net.c), so
 * every console's map reads the same. The game's own staging code no longer moves a slot: its
 * slot update (L39_006D4ED8) and its slot searches (L39_006C4788, L39_006C46F0, L39_006C45C8) are
 * answered here from the room (mp/libraries.tsv), for every slot of a room of up to 24. */
#include "mp_room.h"

#include <stdio.h>
#include <string.h>

#include "mp/game_protos.h"
#include "mp_protocol.h"
#include "mp_wide.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

/* mp_net.c: the party and the relay. */
int32_t rac3_mp_net_session(void);
int32_t rac3_mp_net_host(void);
int rac3_mp_net_client_of(int32_t session);
int rac3_mp_net_member(int n, int32_t* session, int* client, char* name, int size);
void rac3_mp_net_room_send(int32_t target, int kind, const uint8_t* bytes, int length);
void rac3_mp_net_slot_update(int local, int team, int skin, int state, gaddr name, gaddr clan);

/* The settings object's data ($gp - 0x6980), its arrays (slot entries through mp_wide.c, which
 * keeps slots 8-23), the player count, the match's start time and the team-play flag; the net
 * block's room clock (dme_get_time, copied each frame, milliseconds). */
#define GS_POINTER RAC3_MADDR(0x00241640u)
#define GS_NAME 0x000u
#define GS_CLAN 0x080u
#define GS_SKIN 0x0C0u
#define GS_TEAM 0x0D0u
#define GS_CLIENT 0x0E0u
#define GS_STATE 0x0F0u
#define GS_PLAYERS 0x178u
#define GS_MATCH_START 0x174u
#define GS_TEAM_PLAY 0x19Du
#define NET_CLOCK (RAC3_MADDR(0x001A5CD0u) + 0x128u)

/* Slot states (protocol.md 4.5.1): joined, ready, START pressed (slot 0). */
#define STATE_JOINED 0
#define STATE_READY 6
#define STATE_STARTED 7

#define MENU_PROGRAM 39 /* level 39: the lobby; 40-53 are the maps */

/* RELAY_ROOM_STATE: version, width, count, then per player its client, ready, team, skin, clan,
 * name. RELAY_ROOM_REQUEST: an op, the sender's local slot, then the value(s). */
#define STATE_VERSION 1
#define STATE_PLAYER_BYTES (1 + 1 + 2 + 2 + RAC3_ROOM_CLAN + RAC3_ROOM_NAME)
#define STATE_BYTES (3 + RAC3_ROOM_MAX * STATE_PLAYER_BYTES)
#define RESEND_FRAMES 150 /* the host's state again after this many DME updates, unchanged */

enum {
    REQUEST_SLOT = 1 /* local, team, skin, state: a slot's change, as L39_006D4ED8 makes it */
};

typedef struct {
    rac3_room_player p;
    int admitted; /* has a slot in the settings (the host's room has taken its first values) */
} player;

static struct {
    int width;
    player players[RAC3_ROOM_MAX];
    int slot_client[RAC3_ROOM_MAX];
    int admitted_count;
    uint8_t sent[STATE_BYTES];
    int sent_length;
    int members_changed;
    int since_sent;
    rac3_room_result results[RAC3_ROOM_MAX];
    int result_count;
    int started_logged;
} room;

static int net_log(void) {
    static int on = -1;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_NET_LOG");
        on = v != NULL && v[0] == '1';
    }
    return on;
}

int rac3_room_active(void) {
    return rac3_mp_wide_available() && rac3_mp_net_session() != 0 && rac3_mp_net_host() != 0
           && rac3_mp_net_client_of(rac3_mp_net_session()) >= 0;
}

int rac3_room_is_host(void) {
    return rac3_room_active() && rac3_mp_net_host() == rac3_mp_net_session();
}

int rac3_room_my_client(void) {
    return rac3_room_active() ? rac3_mp_net_client_of(rac3_mp_net_session()) : -1;
}

static gaddr gs(void) {
    return GREF(gaddr, GS_POINTER);
}

/* Slot `slot`'s entry of the settings' array at `offset` (0: no settings object). */
static gaddr at(uint32_t offset, int slot) {
    return gs() != 0 ? rac3_mp_wide_settings(offset, slot) : 0;
}

static void put_name(gaddr to, const char* text, size_t size) {
    memset(G(to), 0, size);
    snprintf((char*)G(to), size, "%s", text);
}

/* ---- The members and their slots ---- */

/* The server's members: present or not, their session and name. */
static void read_members(void) {
    int here[RAC3_ROOM_MAX] = {0};
    int32_t session;
    int client;
    char name[33];
    for (int n = 0; rac3_mp_net_member(n, &session, &client, name, (int)sizeof name); n++) {
        if (client < 0 || client >= RAC3_ROOM_MAX) {
            continue;
        }
        player* p = &room.players[client];
        if (!p->p.present || p->p.session != session) {
            memset(p, 0, sizeof *p);
            p->p.present = 1;
            p->p.session = session;
            p->p.team = (int16_t)client;
        }
        /* A name of the game's own length; "(null)" was a joiner with no name in its net block. */
        snprintf(p->p.name, sizeof p->p.name, "%s", name);
        here[client] = 1;
    }
    for (int c = 0; c < RAC3_ROOM_MAX; c++) {
        if (!here[c] && room.players[c].p.present) {
            memset(&room.players[c], 0, sizeof room.players[c]);
        }
    }
}

/* Which client each slot holds: the settings' client words (DME's admission). */
static void read_slots(void) {
    for (int s = 0; s < RAC3_ROOM_MAX; s++) {
        const gaddr client = s < room.width ? at(GS_CLIENT, s) : 0;
        const int c = client != 0 ? GREF(int16_t, client) : -1;
        room.slot_client[s] = c >= 0 && c < RAC3_ROOM_MAX ? c : -1;
    }
}

static int slot_of(int client, int local) {
    for (int s = 0; s < room.width; s++) {
        if (room.slot_client[s] == client && local-- == 0) {
            return s;
        }
    }
    return -1;
}

/* ---- The host's room: its writes into the settings ---- */

/* Slot `slot` of the settings as the room has its player. `with_state` also writes READY (a
 * slot's state below START: ready, or joined when the room has it not ready). */
static void write_slot(int slot, const player* p, int with_state) {
    put_name(at(GS_NAME, slot), p->p.name, RAC3_ROOM_NAME);
    put_name(at(GS_CLAN, slot), p->p.clan, RAC3_ROOM_CLAN);
    GREF(int16_t, at(GS_TEAM, slot)) = p->p.team;
    GREF(int16_t, at(GS_SKIN, slot)) = p->p.skin;
    if (with_state) {
        const gaddr state = at(GS_STATE, slot);
        const int16_t now = GREF(int16_t, state);
        /* START and the map's states are the game's; a leave or a kick (1-5) too. */
        if (now == STATE_JOINED || now == STATE_READY) {
            GREF(int16_t, state) = p->p.ready ? STATE_READY : STATE_JOINED;
        }
    }
}

/* A slot newly given to a client: the room takes the admission's team and skin as the player's
 * first choices, and every joiner's READY is cleared (a new player sees a fresh vote). */
static void admit(int slot, int client) {
    player* p = &room.players[client];
    p->admitted = 1;
    p->p.team = GREF(int16_t, at(GS_TEAM, slot));
    p->p.skin = GREF(int16_t, at(GS_SKIN, slot));
    memcpy(p->p.clan, G(at(GS_CLAN, slot)), RAC3_ROOM_CLAN - 1);
    p->p.clan[RAC3_ROOM_CLAN - 1] = 0;
    p->p.ready = 0;
    /* In a team mode the room balances: a new player joins the smaller of the two teams (the
     * admission gives everyone the joiner's own pick, the first team). */
    if (rac3_room_team_play()) {
        int members[2] = {0, 0};
        for (int c = 0; c < RAC3_ROOM_MAX; c++) {
            if (c != client && room.players[c].admitted && room.players[c].p.team >= 0
                && room.players[c].p.team < 2) {
                members[room.players[c].p.team]++;
            }
        }
        p->p.team = (int16_t)(members[1] < members[0] ? 1 : 0);
    }
    for (int c = 0; c < RAC3_ROOM_MAX; c++) {
        if (c != rac3_room_my_client()) {
            room.players[c].p.ready = 0;
        }
    }
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: room: %s (client %d) in slot %d, team %d, skin %d\n",
            p->p.name,
            client,
            slot,
            p->p.team,
            p->p.skin
        );
    }
}

static int staging(void) {
    return gs() != 0 && openrac_game_loaded_overlay() == MENU_PROGRAM
           && GREF(int16_t, at(GS_STATE, 0)) < STATE_STARTED;
}

static void host_frame(void) {
    int admitted[RAC3_ROOM_MAX] = {0};
    for (int s = 0; s < room.width; s++) {
        const int c = room.slot_client[s];
        if (c < 0 || !room.players[c].p.present) {
            continue;
        }
        admitted[c] = 1;
        if (!room.players[c].admitted) {
            admit(s, c);
        }
    }
    for (int c = 0; c < RAC3_ROOM_MAX; c++) {
        room.players[c].admitted = admitted[c];
    }
    /* In the staging room every slot is as the room has it (a write that changes nothing sends
     * nothing: mp_net.c publishes the settings when their bytes change). */
    if (staging()) {
        for (int s = 0; s < room.width; s++) {
            const int c = room.slot_client[s];
            if (c >= 0 && room.players[c].admitted) {
                write_slot(s, &room.players[c], 1);
            }
        }
    }
}

/* The room's state as the host sends it. */
static int pack(uint8_t* out) {
    int n = 3;
    int count = 0;
    out[0] = STATE_VERSION;
    out[1] = (uint8_t)room.width;
    for (int c = 0; c < RAC3_ROOM_MAX; c++) {
        const player* p = &room.players[c];
        if (!p->p.present) {
            continue;
        }
        out[n++] = (uint8_t)c;
        out[n++] = p->p.ready;
        memcpy(out + n, &p->p.team, 2);
        memcpy(out + n + 2, &p->p.skin, 2);
        memcpy(out + n + 4, p->p.clan, RAC3_ROOM_CLAN);
        memcpy(out + n + 4 + RAC3_ROOM_CLAN, p->p.name, RAC3_ROOM_NAME);
        n += STATE_PLAYER_BYTES - 2;
        count++;
    }
    out[2] = (uint8_t)count;
    return n;
}

static void unpack(const uint8_t* in, int length) {
    if (length < 3 || in[0] != STATE_VERSION) {
        return;
    }
    const int count = in[2];
    int n = 3;
    for (int k = 0; k < count && n + STATE_PLAYER_BYTES <= length; k++) {
        const int c = in[n];
        if (c < RAC3_ROOM_MAX && room.players[c].p.present) {
            rac3_room_player* p = &room.players[c].p;
            p->ready = in[n + 1];
            memcpy(&p->team, in + n + 2, 2);
            memcpy(&p->skin, in + n + 4, 2);
            memcpy(p->clan, in + n + 6, RAC3_ROOM_CLAN);
            memcpy(p->name, in + n + 6 + RAC3_ROOM_CLAN, RAC3_ROOM_NAME);
            p->clan[RAC3_ROOM_CLAN - 1] = 0;
            p->name[RAC3_ROOM_NAME - 1] = 0;
        }
        n += STATE_PLAYER_BYTES;
    }
}

/* The host's state to everyone when it changed, or when the members did (a newcomer has none). */
static void host_send(void) {
    uint8_t bytes[STATE_BYTES];
    const int n = pack(bytes);
    /* Again every few seconds: a console that came in late, or missed one, catches up. */
    if (++room.since_sent < RESEND_FRAMES && !room.members_changed && n == room.sent_length
        && memcmp(bytes, room.sent, (size_t)n) == 0) {
        return;
    }
    room.since_sent = 0;
    room.members_changed = 0;
    rac3_mp_net_room_send(0, RELAY_ROOM_STATE, bytes, n);
    memcpy(room.sent, bytes, (size_t)n);
    room.sent_length = n;
}

void rac3_room_members_changed(void) {
    room.members_changed = 1;
}

void rac3_room_frame(void) {
    if (!rac3_room_active()) {
        if (room.width != 0) {
            memset(&room, 0, sizeof room);
        }
        return;
    }
    room.width = rac3_mp_wide_slots();
    read_members();
    read_slots();
    if (rac3_room_is_host()) {
        host_frame();
        host_send();
    }
    if (!room.started_logged && gs() != 0 && GREF(int16_t, at(GS_STATE, 0)) >= STATE_STARTED) {
        room.started_logged = 1;
        fprintf(stderr, "[info] rac3: room: the match starts with %d players\n", rac3_room_count());
        /* The next round in this room starts with a fresh vote. */
        for (int c = 0; c < RAC3_ROOM_MAX; c++) {
            room.players[c].p.ready = 0;
        }
    } else if (room.started_logged && staging()) {
        room.started_logged = 0;
    }
}

/* ---- Requests ---- */

/* The host applies a slot change: team, skin and READY to the room, then to the settings. A state
 * the room does not hold (a leave, a kick) goes to the settings as the game's host would write it. */
static void apply_slot(int client, int local, int team, int skin, int state) {
    if (client < 0 || client >= RAC3_ROOM_MAX || !room.players[client].p.present) {
        return;
    }
    const int slot = slot_of(client, local);
    if (slot < 0) {
        return;
    }
    player* p = &room.players[client];
    p->p.team = (int16_t)team;
    p->p.skin = (int16_t)skin;
    if (state == STATE_READY || state == STATE_JOINED) {
        p->p.ready = state == STATE_READY;
        if (net_log()) {
            fprintf(
                stderr,
                "[info] rac3: room: %s %s (slot %d)\n",
                p->p.name,
                p->p.ready ? "ready" : "not ready",
                slot
            );
        }
    } else {
        GREF(int16_t, at(GS_STATE, slot)) = (int16_t)state;
    }
    write_slot(slot, p, 1);
}

void rac3_room_receive(int kind, int32_t from, const uint8_t* bytes, int length) {
    if (!rac3_room_active()) {
        return;
    }
    if (kind == RELAY_ROOM_STATE) {
        /* Only the host's state counts; the host's own room is the one it sends. */
        if (from == rac3_mp_net_host() && !rac3_room_is_host()) {
            read_members();
            unpack(bytes, length);
        }
        return;
    }
    if (kind == RELAY_ROOM_REQUEST && rac3_room_is_host() && length >= 8
        && bytes[0] == REQUEST_SLOT) {
        int16_t team, skin, state;
        memcpy(&team, bytes + 2, 2);
        memcpy(&skin, bytes + 4, 2);
        memcpy(&state, bytes + 6, 2);
        read_members();
        read_slots();
        /* A console changes its own slots only. */
        apply_slot(rac3_mp_net_client_of(from), bytes[1], team, skin, state);
    }
}

/* This console's slot `local` changed (the game's slot update, or the native screens'). */
static void request_slot(int local, int team, int skin, int state) {
    if (rac3_room_is_host()) {
        apply_slot(rac3_room_my_client(), local, team, skin, state);
        return;
    }
    uint8_t bytes[8];
    const int16_t values[3] = {(int16_t)team, (int16_t)skin, (int16_t)state};
    bytes[0] = REQUEST_SLOT;
    bytes[1] = (uint8_t)local;
    memcpy(bytes + 2, values, sizeof values);
    rac3_mp_net_room_send(rac3_mp_net_host(), RELAY_ROOM_REQUEST, bytes, (int)sizeof bytes);
}

static const rac3_room_player* me(void) {
    const int c = rac3_room_my_client();
    return c >= 0 && room.players[c].p.present ? &room.players[c].p : NULL;
}

int rac3_room_toggle_ready(void) {
    const rac3_room_player* p = me();
    if (p == NULL || !staging() || slot_of(rac3_room_my_client(), 0) < 0) {
        return 0;
    }
    const int ready = !p->ready;
    fprintf(stderr, "[info] rac3: room: READY %s\n", ready ? "on" : "off");
    request_slot(0, p->team, p->skin, ready ? STATE_READY : STATE_JOINED);
    return 1;
}

void rac3_room_set_team(int team) {
    const rac3_room_player* p = me();
    if (p != NULL && staging() && rac3_room_team_play()) {
        request_slot(0, team, p->skin, p->ready ? STATE_READY : STATE_JOINED);
    }
}

void rac3_room_set_skin(int skin) {
    const rac3_room_player* p = me();
    if (p != NULL && staging()) {
        request_slot(0, p->team, skin, p->ready ? STATE_READY : STATE_JOINED);
    }
}

int rac3_room_ready_count(void) {
    int ready = 0;
    for (int s = 0; s < room.width; s++) {
        const int c = room.slot_client[s];
        if (c < 0 || !room.players[c].p.present) {
            continue;
        }
        /* The host is counted ready, as the game's own START counts it (L39_006C30A0). */
        if (room.players[c].p.ready || room.players[c].p.session == rac3_mp_net_host()) {
            ready++;
        }
    }
    return ready;
}

int rac3_room_start(void) {
    if (!rac3_room_is_host() || !staging()) {
        return 0;
    }
    const int ready = rac3_room_ready_count();
    const int count = rac3_room_count();
    fprintf(
        stderr,
        "[info] rac3: room: START with %d of %d ready: %s\n",
        ready,
        count,
        ready == count ? "starting" : "refused"
    );
    if (ready != count) {
        return 0;
    }
    /* What the map reads, for every slot: names, clans, teams, skins, every joiner ready. The game's
     * START (slot 0's state 7, the start time) follows in the same frame, and both go out in one
     * publish of the settings. */
    for (int s = 0; s < room.width; s++) {
        const int c = room.slot_client[s];
        if (c >= 0 && room.players[c].p.present) {
            write_slot(s, &room.players[c], 1);
        }
    }
    if (GREF(int32_t, gs() + GS_PLAYERS) != count) {
        fprintf(
            stderr,
            "[warning] rac3: room: the settings count %d players, the room %d\n",
            GREF(int32_t, gs() + GS_PLAYERS),
            count
        );
        GREF(int32_t, gs() + GS_PLAYERS) = count;
    }
    return 1;
}

/* ---- What the screens show ---- */

int rac3_room_phase(void) {
    if (!rac3_room_active() || gs() == 0) {
        return RAC3_ROOM_NONE;
    }
    const int program = openrac_game_loaded_overlay();
    if (program != MENU_PROGRAM) {
        return RAC3_ROOM_MATCH;
    }
    if (room.result_count > 0 && GREF(int16_t, at(GS_STATE, 0)) >= STATE_STARTED) {
        return RAC3_ROOM_RESULTS;
    }
    return staging() ? RAC3_ROOM_STAGING : RAC3_ROOM_STARTING;
}

int rac3_room_width(void) {
    return room.width;
}

int rac3_room_count(void) {
    int count = 0;
    for (int s = 0; s < room.width; s++) {
        count += room.slot_client[s] >= 0;
    }
    return count;
}

int rac3_room_team_play(void) {
    return gs() != 0 && GREF(uint8_t, gs() + GS_TEAM_PLAY) != 0;
}

int rac3_room_countdown(void) {
    if (rac3_room_phase() != RAC3_ROOM_STARTING) {
        return -1;
    }
    const int32_t left = GREF(int32_t, gs() + GS_MATCH_START) - GREF(int32_t, NET_CLOCK);
    return left <= 0 ? 0 : (left + 999) / 1000;
}

int rac3_room_slot_client(int slot) {
    return slot >= 0 && slot < room.width ? room.slot_client[slot] : -1;
}

const rac3_room_player* rac3_room_player_of(int client) {
    return client >= 0 && client < RAC3_ROOM_MAX && room.players[client].p.present
               ? &room.players[client].p
               : NULL;
}

/* The results block (the game's for slots 0-7, the port's companions after): a name, the team,
 * kills and deaths, suicides; the score is kills less suicides, as the results screen shows. */
#define RESULTS_NAME_BYTES 32u
#define RESULTS_TEAMS 0x120u
#define RESULTS_KILLS 0x140u
#define RESULTS_SUICIDES 0x1B8u

void rac3_room_capture_results(void) {
    room.result_count = 0;
    for (int s = 0; s < rac3_mp_wide_slots() && s < RAC3_ROOM_MAX; s++) {
        const gaddr name = rac3_mp_wide_results(0, s);
        if (GREF(uint8_t, name) == 0) {
            continue;
        }
        rac3_room_result* r = &room.results[room.result_count++];
        memcpy(r->name, G(name), RESULTS_NAME_BYTES);
        r->name[RESULTS_NAME_BYTES] = 0;
        r->team = GREF(int32_t, rac3_mp_wide_results(RESULTS_TEAMS, s));
        r->kills = GREF(int16_t, rac3_mp_wide_results(RESULTS_KILLS, s));
        r->deaths = GREF(int16_t, rac3_mp_wide_results(RESULTS_KILLS, s) + 2u);
        r->suicides = GREF(int16_t, rac3_mp_wide_results(RESULTS_SUICIDES, s));
        r->score = r->kills - r->suicides;
    }
}

int rac3_room_result_count(void) {
    return room.result_count;
}

const rac3_room_result* rac3_room_result_at(int n) {
    return n >= 0 && n < room.result_count ? &room.results[n] : NULL;
}

/* ---- The game's staging helpers, from the room ---- */

int rac3_room_my_row(int n) {
    const int me_client = rac3_room_my_client();
    int row = -1;
    for (int s = 0; s < room.width; s++) {
        if (room.slot_client[s] < 0) {
            continue;
        }
        row++;
        if (room.slot_client[s] == me_client && n-- == 0) {
            return row;
        }
    }
    return -1;
}

int rac3_room_row_slot(int row) {
    for (int s = 0; s < room.width; s++) {
        if (room.slot_client[s] >= 0 && row-- == 0) {
            return s;
        }
    }
    return -1;
}

/* L39_006C4788(screen, n): the staging row of this console's n-th slot (-1 none). */
int mp_func_L39_006C4788(int screen, int n) {
    if (!rac3_room_active() || gs() == 0) {
        return mp_func_L39_006C4788__game(screen, n);
    }
    read_slots();
    return rac3_room_my_row(n);
}

/* L39_006C46F0(screen, row): the slot of a staging row (the game matches the row's name text
 * against the slots' names; the room's rows are the filled slots in order, as the game fills its
 * rows, and go past the game's eight). */
int mp_func_L39_006C46F0(int screen, int row) {
    if (!rac3_room_active() || gs() == 0) {
        return mp_func_L39_006C46F0__game(screen, row);
    }
    read_slots();
    return rac3_room_row_slot(row);
}

/* L39_006C45C8(screen, row, &local): whether a staging row is one of this console's slots; `local`
 * gets which of them (-1 when it is not). */
int mp_func_L39_006C45C8(int screen, int row, int local_out) {
    if (!rac3_room_active() || gs() == 0) {
        return mp_func_L39_006C45C8__game(screen, row, local_out);
    }
    read_slots();
    const int slot = rac3_room_row_slot(row);
    const int me_client = rac3_room_my_client();
    GREF(int32_t, (gaddr)local_out) = -1;
    if (slot < 0 || room.slot_client[slot] != me_client) {
        return 0;
    }
    int local = 0;
    for (int s = 0; s < slot; s++) {
        local += room.slot_client[s] == me_client;
    }
    GREF(int32_t, (gaddr)local_out) = local;
    return 1;
}

/* L39_006D4ED8(screen, local, name, skin, team, state, clan): this console's slot `local` changed
 * (READY, a team or skin, a leave). The game's host writes its settings and a joiner sends the host
 * a slot update (channel 17); the room takes both: the host's room applies it, a joiner's asks the
 * host's room. A state the room does not hold (a leave, a kick) still goes as the game's channel 17
 * message, which the host's core handler (0x0019A4D0) applies. Returns 1 when sent, 0 when the
 * console has no such slot (the game's own refusal). */
int mp_func_L39_006D4ED8(int screen, int local, int name, int skin, int team, int state, int clan) {
    if (!rac3_room_active() || gs() == 0) {
        return mp_func_L39_006D4ED8__game(screen, local, name, skin, team, state, clan);
    }
    read_members();
    read_slots();
    if (slot_of(rac3_room_my_client(), local) < 0) {
        return mp_func_L39_006D4ED8__game(screen, local, name, skin, team, state, clan);
    }
    if (state == STATE_READY || state == STATE_JOINED || rac3_room_is_host()) {
        request_slot(local, team, skin, state);
    } else {
        rac3_mp_net_slot_update(local, team, skin, state, (gaddr)name, (gaddr)clan);
    }
    return 1;
}
