/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's multiplayer maps: stock bugs of the game's own code, fixed in C around it.
 * Each is a stock bug, also worked around by the Horizon Private Server project (its UYA patch,
 * read for reference only, CREDITS.md); the fixes here are our own. Each map carries its own copy
 * of the engine (mp/maps.py), so each map's copy of each function is wrapped: mp/fixes.py writes
 * the rows (mp/libraries.tsv) and host/mp_fixes.h, one MP_FIX line a map with each function's
 * place in it, which the end of this file expands.
 *
 * - A gadget moby whose class the map never loaded: the hero's gadget equip creates the gadget's
 *   mobys, and CreateMoby gives one with no class (+0x24 zero) when the map's class table lacks
 *   it; the moby's update then reads through that class and the game crashes (Marcadia Palace,
 *   equipping weapons). Inside the equip, such a moby is deleted and none returned, which the
 *   equip already takes as "not yet"; the class is not tried again on this map.
 * - Kill stealing: a hit on a player already dead (no hit points left) still credited the player
 *   who hit; inside the hit, the credit is skipped for a dead target.
 * - Dead shooting: a dead player's weapon update still read its pad and fired; it reads nothing.
 * - Dead jumping: a dead local player could still jump and crouch; its no-input timer is held.
 * - The flux rifle's muzzle light: each shot creates a red point light and keeps its id for the
 *   light's life, over the id of a light still lit, which nothing then deletes: two shots within a
 *   light's life left one on the ground for good. The light still lit is deleted first.
 * - Wall sniping: a remote player's sniper shot with a declared target ran its collision line
 *   without the flag that lets it through the thin walls the shooter's own client let it through,
 *   so the victim's client stopped the shot at a wall the shooter shot through.
 * - Metropolis's base: the flag that keeps a player in the base below the death height from
 *   dying followed the base's triggers only, so a player falling off the map could keep it and
 *   be teleported into a base; it follows the player's height against the death height.
 * - Turrets waited for a remote player's state to be younger than 126 ms of net time, which a
 *   relayed peer's net time is not: the state is taken as recent.
 * - A remote player whose last state is older than 7.2 s of net time was dropped as lagging; on
 *   a relay a late packet is a slow one, not a lost player: it is kept.
 * - The local player stalled ("waiting for response", no input) when the host had been silent
 *   for 3 s of net time: the stall is left out, the room's own drop handles a host that left.
 * - Game time: in some states of the map's main loop (a jump table on the game state word, its
 *   values 3, 5, 6 and 16) the loop updates the pads but never the net (NWUpdate), so the net time
 *   and the room's messages stand still while the others' run on; when the net time has not moved
 *   since the last frame in such a state, the net is updated before the pads.
 * - A remote gadget event (a shot; the local ones come with no message) arrives with the net time
 *   it should run at, which on a relay is in the future or the past of this client's clock: it
 *   runs now.
 *
 * OPENRAC_RAC3_MP_STOCK=1 runs the game's own code with none of these; OPENRAC_RAC3_MP_FIXES_LOG=1
 * logs each fix the first times it acts. */
#include <stdint.h>
#include <stdio.h>

#include "mp/game_protos.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

/* The game's net state block (mp_net.c's NET_BLOCK): its net time, in milliseconds, and the
 * time the host was last heard from; the players in the game, and the word that has the game never
 * wait on the host (the stall below needs it zero and two players or more). */
#ifdef RAC3_PAL
#define NET_BLOCK 0x001A5CD0u
#define METROPOLIS_DEATH_HEIGHT 0x00248130u /* map 44's death height (a float) */
#else
#define NET_BLOCK 0x001A5E50u
#define METROPOLIS_DEATH_HEIGHT 0u /* not yet found in NTSC-U's map 44 */
#endif
#define NET_TIME (NET_BLOCK + 0x128u)
#define NET_PLAYERS (NET_BLOCK + 0x12Cu)
#define NET_NO_WAIT (NET_BLOCK + 0x170u) /* nonzero: the game never waits on the host */
#define NET_HOST_HEARD (NET_BLOCK + 0x7D0u)

/* The executable's net update (NWUpdate) and the game state word the maps' main loop switches on. */
#define NET_UPDATE RAC3_MADDR(0x001959F8u)
#define GAME_STATE RAC3_MADDR(0x00241128u)

/* The game's times, in milliseconds of net time (the stock code's own constants). */
#define LAG_OUT_TIME 7200
#define HOST_SILENT_TIME 3000

/* A player (the hero's struct, one per player). */
#define PLAYER_Z 0xA8u            /* its position's height (a float) */
#define PLAYER_NO_INPUT 0x320u    /* frames left with no input (a short) */
#define PLAYER_IS_LOCAL 0x1A14u   /* a byte: this console's player */
#define PLAYER_IN_BASE 0x1A15u    /* a byte: below the death height but in a base, not dying */
#define PLAYER_MOBY 0x2524u       /* its moby */
#define PLAYER_PAD 0x2534u        /* its pad */
#define PLAYER_NET 0x2558u        /* its net player (tNW_Player) */
#define NET_PLAYER_DATA 0x4u      /* the net player's replicated data (tNW_PlayerData) */
#define NET_DATA_HIT_POINTS 0x44u /* a float */
#define NET_DATA_RECEIVED 0x4Cu   /* the net time its last state was stamped with */
#define DEAD_NO_INPUT 10          /* the no-input frames a dead local player is held at */

/* A moby. */
#define MOBY_CLASS 0x24u /* its class, 0 when the map has none of its kind */
#define MOBY_VARS 0x68u  /* its variables */

/* A sniper shot's moby variables and the flux rifle's. */
#define SHOT_SHOOTER 0x3Cu     /* the player who fired */
#define SHOT_TARGET 0x4Cu      /* the moby the shooter's client said it hit, or 0 */
#define FLUX_LIGHT 0x10u       /* the muzzle light's id, -1 when none */
#define FLUX_LIGHT_TIMER 0x14u /* the muzzle light's frames left, 0 when out */
#define POINT_LIGHTS 8         /* the point light slots */
#define COLL_THROUGH_WALLS 1   /* the collision line's flag that lets a shot past thin walls */

/* A gadget event message (tNW_GadgetEventMessage): the net time it runs at. */
#define GADGET_EVENT_TIME 0x4u

/* The classes a gadget equip found missing on the map loaded now. */
#define MISSING_CLASSES_MAX 16

/* Metropolis's map: on PAL 44 (NTSC-U's 54 is a build of it). */
#define MAP_IS_METROPOLIS(m) ((m) == 44 || (m) == 54)

enum fix_kind {
    FIX_GADGET_CLASS,
    FIX_KILL_STEALING,
    FIX_DEAD_SHOOTING,
    FIX_DEAD_JUMPING,
    FIX_FLUX_LIGHT,
    FIX_WALL_SNIPING,
    FIX_METROPOLIS_BASE,
    FIX_TURRET_DELAY,
    FIX_LAG_OUT,
    FIX_HOST_STALL,
    FIX_GADGET_EVENT_TIME,
    FIX_GAME_TIME,
    FIX_KINDS
};

static const char* const FIX_NAMES[FIX_KINDS] = {
    "gadget moby with no class",
    "credit for hitting a dead player",
    "dead player firing",
    "dead player moving",
    "flux muzzle light still lit",
    "remote sniper shot through a thin wall",
    "Metropolis base flag",
    "turret waiting on a remote state",
    "remote player lagged out",
    "stall on a silent host",
    "remote gadget event time",
    "net updated in a state that skips it",
};

static int fixes_on(void) {
    static int on = -1;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_MP_STOCK");
        on = !(v != NULL && *v != 0 && *v != '0');
    }
    return on;
}

/* Logs a fix's first few acts. */
static void fixed(enum fix_kind kind, int map) {
    static int log = -1;
    static int count[FIX_KINDS];
    if (log < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_MP_FIXES_LOG");
        log = v != NULL && *v != 0 && *v != '0';
    }
    if (log && count[kind] < 8) {
        count[kind]++;
        fprintf(stderr, "[info] rac3: map %d: stock bug fixed: %s\n", map, FIX_NAMES[kind]);
    }
}

/* A player's hit points as the net data has them, or 1 when it has none (alive). */
static float hit_points(gaddr player) {
    if (player == 0) {
        return 1.0f;
    }
    const gaddr net = GREF(gaddr, player + PLAYER_NET);
    const gaddr data = net != 0 ? GREF(gaddr, net + NET_PLAYER_DATA) : 0;
    return data != 0 ? GREF(float, data + NET_DATA_HIT_POINTS) : 1.0f;
}

static int is_dead(gaddr player) {
    return hit_points(player) <= 0.0f;
}

/* ---- The gadget moby with no class ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static int g_equip_depth;
static int g_missing_map = -1;
static int g_missing_count;
static int16_t g_missing[MISSING_CLASSES_MAX];

static int fix_equip(int map, int hero, int (*game)(int)) {
    if (map != g_missing_map) {
        g_missing_map = map;
        g_missing_count = 0;
    }
    g_equip_depth++;
    const int result = game(hero);
    g_equip_depth--;
    return result;
}

static int fix_create_moby(
    int map, int oclass, int vars_size, int (*game)(int, int), int (*delete_moby)(int)
) {
    if (g_equip_depth == 0 || !fixes_on()) {
        return game(oclass, vars_size);
    }
    for (int i = 0; i < g_missing_count; i++) {
        if (g_missing[i] == (int16_t)oclass) {
            return 0;
        }
    }
    const int moby = game(oclass, vars_size);
    if (moby == 0 || GREF(gaddr, (gaddr)moby + MOBY_CLASS) != 0) {
        return moby;
    }
    delete_moby(moby);
    if (g_missing_count < MISSING_CLASSES_MAX) {
        g_missing[g_missing_count++] = (int16_t)oclass;
    }
    fixed(FIX_GADGET_CLASS, map);
    return 0;
}

/* ---- Kill stealing ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static int g_hit_depth;

static int fix_player_hit(int player, int source, int (*game)(int, int)) {
    g_hit_depth++;
    const int result = game(player, source);
    g_hit_depth--;
    return result;
}

static int fix_who_hit_me(int map, int target, int source, int a2, int (*game)(int, int, int)) {
    if (g_hit_depth > 0 && fixes_on() && is_dead((gaddr)target)) {
        fixed(FIX_KILL_STEALING, map);
        return 0;
    }
    return game(target, source, a2);
}

/* ---- Dead shooting and dead jumping ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static gaddr g_dead_pad;

static int fix_weapon_update(int player, int slot, int (*game)(int, int)) {
    const gaddr outer = g_dead_pad;
    if (fixes_on() && player != 0 && is_dead((gaddr)player)) {
        g_dead_pad = GREF(gaddr, (gaddr)player + PLAYER_PAD);
    }
    const int result = game(player, slot);
    g_dead_pad = outer;
    return result;
}

static int fix_pad_pressed(int map, int pad, int (*game)(int)) {
    if (pad != 0 && (gaddr)pad == g_dead_pad) {
        fixed(FIX_DEAD_SHOOTING, map);
        return 0;
    }
    return game(pad);
}

/* Each player's update: the dead local player's input held, Metropolis's base flag.
 * Stock bugs, also worked around by the Horizon Private Server project; our own fixes. */
static int fix_player_update(int map, int player, int (*game)(int)) {
    const gaddr p = (gaddr)player;
    if (fixes_on() && p != 0 && GREF(uint8_t, p + PLAYER_IS_LOCAL) != 0) {
        if (is_dead(p)) {
            if (GREF(int16_t, p + PLAYER_NO_INPUT) < DEAD_NO_INPUT) {
                fixed(FIX_DEAD_JUMPING, map);
            }
            GREF(int16_t, p + PLAYER_NO_INPUT) = DEAD_NO_INPUT;
        }
        if (MAP_IS_METROPOLIS(map) && METROPOLIS_DEATH_HEIGHT != 0) {
            const float death = GREF(float, METROPOLIS_DEATH_HEIGHT);
            const float z = GREF(float, p + PLAYER_Z);
            const uint8_t in_base = GREF(uint8_t, p + PLAYER_IN_BASE);
            if (in_base != 0 && z > death) {
                GREF(uint8_t, p + PLAYER_IN_BASE) = 0;
                fixed(FIX_METROPOLIS_BASE, map);
            } else if (in_base == 0 && z < death) {
                GREF(uint8_t, p + PLAYER_IN_BASE) = 1;
                fixed(FIX_METROPOLIS_BASE, map);
            }
        }
    }
    return game(player);
}

/* ---- The flux rifle's muzzle light ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static int fix_flux_fire(
    int map,
    int moby,
    int a1,
    int a2,
    int a3,
    int a4,
    int (*game)(int, int, int, int, int),
    int (*delete_light)(int)
) {
    const gaddr vars = moby != 0 ? GREF(gaddr, (gaddr)moby + MOBY_VARS) : 0;
    if (fixes_on() && vars != 0 && GREF(int32_t, vars + FLUX_LIGHT_TIMER) != 0) {
        const int32_t light = GREF(int32_t, vars + FLUX_LIGHT);
        if (light >= 0 && light < POINT_LIGHTS) {
            delete_light(light);
            GREF(int32_t, vars + FLUX_LIGHT) = -1;
            fixed(FIX_FLUX_LIGHT, map);
        }
    }
    return game(moby, a1, a2, a3, a4);
}

/* ---- Wall sniping ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static gaddr g_remote_shot_from;
static gaddr g_remote_shot_moby;

static int fix_shot_collision(int moby, int from, int (*game)(int, int)) {
    const gaddr outer_from = g_remote_shot_from;
    const gaddr outer_moby = g_remote_shot_moby;
    const gaddr vars = moby != 0 ? GREF(gaddr, (gaddr)moby + MOBY_VARS) : 0;
    const gaddr shooter = vars != 0 ? GREF(gaddr, vars + SHOT_SHOOTER) : 0;
    if (fixes_on() && shooter != 0 && GREF(uint8_t, shooter + PLAYER_IS_LOCAL) == 0
        && GREF(gaddr, vars + SHOT_TARGET) != 0) {
        g_remote_shot_from = (gaddr)from;
        g_remote_shot_moby = GREF(gaddr, shooter + PLAYER_MOBY);
    }
    const int result = game(moby, from);
    g_remote_shot_from = outer_from;
    g_remote_shot_moby = outer_moby;
    return result;
}

static int fix_coll_line(
    int map, int from, int to, int flags, int moby, int a4, int (*game)(int, int, int, int, int)
) {
    if (g_remote_shot_from != 0 && (gaddr)from == g_remote_shot_from
        && (gaddr)moby == g_remote_shot_moby && flags == 0) {
        flags = COLL_THROUGH_WALLS;
        fixed(FIX_WALL_SNIPING, map);
    }
    return game(from, to, flags, moby, a4);
}

/* ---- The net clock's stalls ----
 * Stock bug, also worked around by the Horizon Private Server project; our own fix. */

static int fix_state_fresh(int map, int player, int (*game)(int)) {
    if (!fixes_on()) {
        return game(player);
    }
    if (!game(player)) {
        fixed(FIX_TURRET_DELAY, map);
    }
    return 1;
}

static int fix_net_apply(int map, int player, int (*game)(int)) {
    const gaddr net = player != 0 ? GREF(gaddr, (gaddr)player + PLAYER_NET) : 0;
    const gaddr data = net != 0 ? GREF(gaddr, net + NET_PLAYER_DATA) : 0;
    if (fixes_on() && data != 0) {
        const int32_t oldest = GREF(int32_t, NET_TIME) - LAG_OUT_TIME;
        if (GREF(int32_t, data + NET_DATA_RECEIVED) < oldest) {
            GREF(int32_t, data + NET_DATA_RECEIVED) = oldest;
            fixed(FIX_LAG_OUT, map);
        }
    }
    return game(player);
}

static float fix_local_update(int map, int player, float (*game)(int)) {
    if (!fixes_on()) {
        return game(player);
    }
    const int32_t heard = GREF(int32_t, NET_HOST_HEARD);
    const int32_t oldest = GREF(int32_t, NET_TIME) - HOST_SILENT_TIME;
    const int silent =
        heard < oldest && GREF(int32_t, NET_NO_WAIT) == 0 && GREF(int32_t, NET_PLAYERS) >= 2;
    if (silent) {
        GREF(int32_t, NET_HOST_HEARD) = oldest;
    }
    const float result = game(player);
    if (silent && GREF(int32_t, NET_HOST_HEARD) == oldest) {
        GREF(int32_t, NET_HOST_HEARD) = heard;
        fixed(FIX_HOST_STALL, map);
    }
    return result;
}

static float fix_gadget_event(
    int map,
    int player,
    int type,
    int time,
    int gadget,
    int kind,
    int message,
    float (*game)(int, int, int, int, int, int)
) {
    if (message != 0 && fixes_on()) {
        const int32_t now = GREF(int32_t, NET_TIME);
        if (time != now) {
            time = time > now ? now : now - 1;
            GREF(int32_t, (gaddr)message + GADGET_EVENT_TIME) = time;
            fixed(FIX_GADGET_EVENT_TIME, map);
        }
    }
    return game(player, type, time, gadget, kind, message);
}

/* The main loop's states that update the pads with no net update. */
static int state_skips_net(int32_t state) {
    return state == 3 || state == 5 || state == 6 || state == 16;
}

static int g_last_net_time;

static int fix_pad_update(int map, int (*game)(void)) {
    if (fixes_on() && NET_UPDATE != 0 && GAME_STATE != 0) {
        const int32_t now = GREF(int32_t, NET_TIME);
        if (state_skips_net(GREF(int32_t, GAME_STATE)) && now == g_last_net_time) {
            GFN(int (*)(void), NET_UPDATE)();
            fixed(FIX_GAME_TIME, map);
        }
        g_last_net_time = GREF(int32_t, NET_TIME);
    }
    return game();
}

/* One map's wraps, in mp/fixes.py FIXED's order. */
#define MP_FIX(                                                                                    \
    m,                                                                                             \
    equip,                                                                                         \
    create_moby,                                                                                   \
    delete_moby,                                                                                   \
    player_hit,                                                                                    \
    who_hit_me,                                                                                    \
    weapon_update,                                                                                 \
    pad_pressed,                                                                                   \
    flux_fire,                                                                                     \
    delete_light,                                                                                  \
    shot_collision,                                                                                \
    coll_line,                                                                                     \
    state_fresh,                                                                                   \
    net_apply,                                                                                     \
    local_update,                                                                                  \
    player_update,                                                                                 \
    gadget_event,                                                                                  \
    pad_update                                                                                     \
)                                                                                                  \
    int mp_func_L##m##_##equip(int a0) {                                                           \
        return fix_equip(m, a0, mp_func_L##m##_##equip##__game);                                   \
    }                                                                                              \
    int mp_func_L##m##_##create_moby(int a0, int a1) {                                             \
        return fix_create_moby(                                                                    \
            m, a0, a1, mp_func_L##m##_##create_moby##__game, mp_func_L##m##_##delete_moby          \
        );                                                                                         \
    }                                                                                              \
    int mp_func_L##m##_##player_hit(int a0, int a1) {                                              \
        return fix_player_hit(a0, a1, mp_func_L##m##_##player_hit##__game);                        \
    }                                                                                              \
    int mp_func_L##m##_##who_hit_me(int a0, int a1, int a2) {                                      \
        return fix_who_hit_me(m, a0, a1, a2, mp_func_L##m##_##who_hit_me##__game);                 \
    }                                                                                              \
    int mp_func_L##m##_##weapon_update(int a0, int a1) {                                           \
        return fix_weapon_update(a0, a1, mp_func_L##m##_##weapon_update##__game);                  \
    }                                                                                              \
    int mp_func_L##m##_##pad_pressed(int a0) {                                                     \
        return fix_pad_pressed(m, a0, mp_func_L##m##_##pad_pressed##__game);                       \
    }                                                                                              \
    int mp_func_L##m##_##flux_fire(int a0, int a1, int a2, int a3, int a4) {                       \
        return fix_flux_fire(                                                                      \
            m,                                                                                     \
            a0,                                                                                    \
            a1,                                                                                    \
            a2,                                                                                    \
            a3,                                                                                    \
            a4,                                                                                    \
            mp_func_L##m##_##flux_fire##__game,                                                    \
            mp_func_L##m##_##delete_light                                                          \
        );                                                                                         \
    }                                                                                              \
    int mp_func_L##m##_##shot_collision(int a0, int a1) {                                          \
        return fix_shot_collision(a0, a1, mp_func_L##m##_##shot_collision##__game);                \
    }                                                                                              \
    int mp_func_L##m##_##coll_line(int a0, int a1, int a2, int a3, int a4) {                       \
        return fix_coll_line(m, a0, a1, a2, a3, a4, mp_func_L##m##_##coll_line##__game);           \
    }                                                                                              \
    int mp_func_L##m##_##state_fresh(int a0) {                                                     \
        return fix_state_fresh(m, a0, mp_func_L##m##_##state_fresh##__game);                       \
    }                                                                                              \
    int mp_func_L##m##_##net_apply(int a0) {                                                       \
        return fix_net_apply(m, a0, mp_func_L##m##_##net_apply##__game);                           \
    }                                                                                              \
    float mp_func_L##m##_##local_update(int a0) {                                                  \
        return fix_local_update(m, a0, mp_func_L##m##_##local_update##__game);                     \
    }                                                                                              \
    int mp_func_L##m##_##player_update(int a0) {                                                   \
        return fix_player_update(m, a0, mp_func_L##m##_##player_update##__game);                   \
    }                                                                                              \
    float mp_func_L##m##_##gadget_event(int a0, int a1, int a2, int a3, int a4, int a5) {          \
        return fix_gadget_event(m, a0, a1, a2, a3, a4, a5, mp_func_L##m##_##gadget_event##__game); \
    }                                                                                              \
    int mp_func_L##m##_##pad_update(void) {                                                        \
        return fix_pad_update(m, mp_func_L##m##_##pad_update##__game);                             \
    }

#include "mp_fixes.h"
