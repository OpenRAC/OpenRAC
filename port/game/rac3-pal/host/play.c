/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's play calls (game_host.h's openrac_game_play): for now its memory card only,
 * as Going Commando's (rac2/host/play.c) has it. The card's library is the shared one
 * (common/lib/mc.c) and the game's own card code runs as it is; these say what the files on the
 * card are, for the save marks beside each save, the mods' save calls and the launcher:
 *
 *   BESCES-52456RATCHET/       the game's folder ("B" + the region's letter + the serial,
 *                              "RATCHET": the card code's pattern, twelve stars before
 *                              RATCHET, filled in)
 *     icon.sys, static.ico     the card browser's title and icon
 *     net.bin                  the network configuration (4,028 bytes)
 *     save0.bin .. save3.bin   the four save slots (save_format.h), written whole, every one
 *                              (an empty slot's level is -1) when the game makes the folder
 *     BESCES-52456RATCHET      28,672 bytes the game writes beside them when it makes the folder
 *
 * The front end also opens Ratchet & Clank's and Going Commando's saves (BASCUS-97199RATCHET and
 * the other six regions' folders) when it starts, as Going Commando does for the Barlow vendor:
 * mc.c's other games' saves let it read them.
 *
 * The phase. Outside a level the boot program (or frontbin) runs: its mode word (0x001D5B94) is 6
 * while a level's program loads (the way out of the menus, the level asked for at 0x001D9D04),
 * else it is the front end. In a level, its own mode word (tools.c, rac3_level_mode; game_mode_push
 * writes it, game_mode_apply's switch reads it): 0 play, once the hero's moby is made (0x001A71A0),
 * 2 a scene and 6 a level's landing (the opening of Veldin, Ratchet leaving the ship), 1 the
 * level's movie; 4 the pause menus and the quick select pause, 5 the vendor, 8 a weapon's upgrade
 * and the others are menus.
 *
 * In a level, the hero and the level's mobys, after Going Commando's play.c (#386): the moby
 * layout is Going Commando's (MobyProc's reads are its instruction for instruction: +0x10 the
 * place, +0x20 the state, 0x80 and over dead or free, +0x2C the scale, +0x34 the flags, +0x40 the
 * frame, +0x42 the seq, +0x44 the time in it, +0x64 the update, +0x68 the pvars, +0xAA the class,
 * +0xB2 a placed moby's id, +0xF0 the rotation), 0x100 bytes a moby, in the level's table: Veldin's
 * D_L01_001E001C (its start, the placed mobys), D_L01_001E0020 (the first the game made, the
 * placed mobys' loop's end) and D_L01_001E0024 (its end), each level's at its own place
 * (OPENRAC_LDATA). The hero's moby is 0x001A71A0, his state 0x001A71A4, his health 0x001A7430,
 * the item in his hand 0x001A5E08. A moby is deleted by the level's moby_remove (Veldin's
 * func_L01_00439300, catalogue row func_003C2880), travel goes through request_level_change
 * (program.c's rac3_start_level). Not yet: figures (the co-op mod's), a save started or captured,
 * the moby's animation set by a pose (its seq functions are not paired yet: a pose places, turns
 * and scales). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"
#include "save_format.h"

#define LEVELS RAC3_SAVE_LEVEL_LIMIT

/* The levels by the game's numbers (launcher/src/lib/labels.ts names them the same). */
static const char* const rac3_level_names[LEVELS] = {
    [1] = "Veldin",
    [2] = "Florana",
    [3] = "Starship Phoenix",
    [4] = "Marcadia",
    [5] = "Daxx",
    [6] = "Starship Phoenix (Under Attack)",
    [7] = "Annihilation Nation",
    [8] = "Aquatos",
    [9] = "Tyhrranosis",
    [10] = "Zeldrin Starport",
    [11] = "Obani Moons",
    [12] = "Rilgar",
    [13] = "Holostar Studios Ratchet",
    [14] = "Koros",
    [16] = "Kerwan",
    [17] = "Crash Site",
    [18] = "Aridia",
    [19] = "Thran Asteroid Belt",
    [20] = "Final Boss",
    [21] = "Obani Draco",
    [22] = "Mylon",
    [23] = "Holostar Studios Clank",
    [24] = "Insomniac Museum",
    [26] = "Kerwan Ranger Missions",
    [27] = "Aquatos Base",
    [28] = "Aquatos Sewers",
    [29] = "Tyhrranosis Ranger Missions",
    [30] = "Vid-Comic Unnamed",
    [31] = "Vid-Comic 1",
    [32] = "Vid-Comic 4",
    [33] = "Vid-Comic 2",
    [34] = "Vid-Comic 3",
    [35] = "Vid-Comic 5",
    [36] = "Vid-Comic 1 Special Edition",
    [39] = "Multiplayer Menu",
    [40] = "Bakisi Isles",
    [41] = "Hoven Gorge",
    [42] = "Outpost X12",
    [43] = "Korgon Outpost",
    [44] = "Metropolis",
    [45] = "Blackwater City",
    [46] = "Command Center",
    [47] = "Blackwater Docks",
    [48] = "Aquatos Sewers (multiplayer)",
    [49] = "Marcadia Palace",
    [50] = "Bakisi Isles (split screen)",
    [51] = "Hoven Gorge (split screen)",
    [52] = "Outpost X12 (split screen)",
    [53] = "Korgon Outpost (split screen)",
    [54] = "Metropolis (split screen)",
    [55] = "Blackwater City (split screen)",
};

#define FRONT_END_MODE                                                                             \
    RAC3_ADDR(0x001D5B94u)                /* the boot program's (and frontbin's) mode: 6 loading */
#define LEVEL_NEXT RAC3_ADDR(0x001D9D04u) /* the level the front end goes to */
#define HERO_MOBY RAC3_ADDR(0x001A71A0u)  /* the hero's moby, 0 until the level makes it */
#define FRONT_END_LOADING 6

int rac3_level_mode(uint32_t* mode); /* tools.c */

static int phase(int32_t* level) {
    int32_t ignored;
    if (level == NULL) {
        level = &ignored;
    }
    const int loaded = openrac_game_loaded_overlay();
    uint32_t mode = 0;
    if (loaded > 0 && loaded < LEVELS && rac3_level_names[loaded] != NULL
        && rac3_level_mode(&mode)) {
        *level = loaded;
        switch (mode) {
            case 0:
                return GREF(gaddr, HERO_MOBY) != 0 ? OPENRAC_GAME_PHASE_PLAYING
                                                   : OPENRAC_GAME_PHASE_LOADING;
            case 1:
            case 2:
            case 6:
                return OPENRAC_GAME_PHASE_SCENE;
            default:
                return OPENRAC_GAME_PHASE_MENU;
        }
    }
    if (loaded < 0 || loaded >= LEVELS || rac3_level_names[loaded] == NULL) {
        /* The boot program or frontbin. */
        if (GREF(int32_t, FRONT_END_MODE) == FRONT_END_LOADING) {
            const int32_t next = GREF(int32_t, LEVEL_NEXT);
            *level = next > 0 && next < LEVELS && rac3_level_names[next] != NULL ? next : -1;
            return OPENRAC_GAME_PHASE_LOADING;
        }
        *level = -1;
        return OPENRAC_GAME_PHASE_FRONT_END;
    }
    /* A level without a known mode word (the multiplayer maps'). */
    *level = loaded;
    return GREF(gaddr, HERO_MOBY) != 0 ? OPENRAC_GAME_PHASE_PLAYING : OPENRAC_GAME_PHASE_LOADING;
}

uint32_t rac3_program_loads(void); /* program.c */

static uint32_t loads(void) {
    return rac3_program_loads();
}

/* ---- The hero and the level's mobys ---- */

#define HERO_STATE RAC3_ADDR(0x001A71A4u)
#define HERO_HEALTH RAC3_ADDR(0x001A7430u)
#define HELD_ITEM RAC3_ADDR(0x001A5E08u) /* 0 while the hand changes */
#define BOLTS RAC3_ADDR(0x00142660u)
#define MOBY_BASE OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E001Cu))
#define MOBY_DYNAMIC OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E0020u))
#define MOBY_END OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E0024u))
#define MOBY_SIZE 0x100u

enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_SCALE = 0x2C,
    MOBY_FLAGS = 0x34,
    MOBY_FRAME = 0x40,
    MOBY_NEXT_FRAME = 0x41,
    MOBY_SEQ = 0x42,
    MOBY_TARGET_SEQ = 0x43,
    MOBY_ANIM_TIME = 0x44,
    MOBY_ANIM_SPEED = 0x48,
    MOBY_OCLASS = 0xAA,
    MOBY_UID = 0xB2,
    MOBY_ROT = 0xF0
};

/* A story level's program runs (the multiplayer maps have no table here yet). */
static int in_level(void) {
    const int loaded = openrac_game_loaded_overlay();
    return loaded >= 1 && loaded <= 36 && rac3_level_names[loaded] != NULL;
}

static gaddr hero_moby(void) {
    return in_level() ? GREF(gaddr, HERO_MOBY) : 0;
}

static int moby_table(gaddr* base, gaddr* dynamic, gaddr* end) {
    if (!in_level()) {
        return 0;
    }
    *base = GREF(gaddr, MOBY_BASE);
    *dynamic = GREF(gaddr, MOBY_DYNAMIC);
    *end = GREF(gaddr, MOBY_END);
    return *base != 0 && *end > *base && *end < openrac_guest_ram_size
           && (*end - *base) % MOBY_SIZE == 0 && *dynamic >= *base && *dynamic <= *end;
}

static gaddr moby_at(int32_t slot) {
    gaddr base, dynamic, end;
    if (slot < 0 || !moby_table(&base, &dynamic, &end)) {
        return 0;
    }
    const gaddr m = base + (gaddr)slot * MOBY_SIZE;
    return m < end && GREF(uint8_t, m + MOBY_STATE) < 0x80 ? m : 0;
}

static int32_t slot_of(gaddr moby) {
    gaddr base, dynamic, end;
    if (moby == 0 || !moby_table(&base, &dynamic, &end) || moby < base || moby >= end
        || (moby - base) % MOBY_SIZE != 0) {
        return -1;
    }
    return (int32_t)((moby - base) / MOBY_SIZE);
}

/* A placed moby's key: its id in the level's file (+0xB2); one the game made has none. */
static int32_t moby_key(gaddr moby, gaddr dynamic) {
    return moby < dynamic ? GREF(int16_t, moby + MOBY_UID) : -1;
}

static int mobys(openrac_game_moby_state* out, int max) {
    gaddr base, dynamic, end;
    if (!moby_table(&base, &dynamic, &end)) {
        return 0;
    }
    int n = 0;
    for (gaddr m = base; m < end; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) >= 0x80) {
            continue;
        }
        if (out != NULL && n < max) {
            openrac_game_moby_state* s = &out[n];
            s->slot = (int32_t)((m - base) / MOBY_SIZE);
            s->key = moby_key(m, dynamic);
            s->oclass = GREF(int16_t, m + MOBY_OCLASS);
            s->flags = GREF(uint16_t, m + MOBY_FLAGS);
            s->state = GREF(uint8_t, m + MOBY_STATE);
            for (unsigned i = 0; i < 3; ++i) {
                s->position[i] = GREF(float, m + MOBY_POS + 4u * i);
                s->rotation[i] = GREF(float, m + MOBY_ROT + 4u * i);
            }
            s->scale = GREF(float, m + MOBY_SCALE);
            s->seq = GREF(uint8_t, m + MOBY_TARGET_SEQ);
            s->frame = GREF(uint8_t, m + MOBY_FRAME);
            s->next_frame = GREF(uint8_t, m + MOBY_NEXT_FRAME);
            s->anim_time = GREF(float, m + MOBY_ANIM_TIME);
            s->anim_speed = GREF(float, m + MOBY_ANIM_SPEED);
        }
        ++n;
    }
    return n;
}

static int32_t moby_slot(int32_t key) {
    gaddr base, dynamic, end;
    if (key < 0 || !moby_table(&base, &dynamic, &end)) {
        return -1;
    }
    for (gaddr m = base; m < dynamic; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(int16_t, m + MOBY_UID) == key) {
            return (int32_t)((m - base) / MOBY_SIZE);
        }
    }
    return -1;
}

/* A moby put in a pose: its place, rotation and scale. */
static void moby_pose(gaddr moby, const openrac_game_pose* pose) {
    if (slot_of(moby) < 0) {
        return;
    }
    for (unsigned i = 0; i < 3; ++i) {
        GREF(float, moby + MOBY_POS + 4u * i) = pose->position[i];
        GREF(float, moby + MOBY_ROT + 4u * i) = pose->rotation[i];
    }
    if (pose->scale > 0.0f) {
        GREF(float, moby + MOBY_SCALE) = pose->scale;
    }
}

static void moby_kill(gaddr moby) {
    if (slot_of(moby) >= 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80 && moby != hero_moby()) {
        func_L01_00439300(moby);
    }
}

static int32_t hero_slot(void) {
    return slot_of(hero_moby());
}

/* His velocity: his move between the last two frames (play_frame), in units a second. */
static struct {
    uint32_t loads;
    int valid;
    float last[3];
    float velocity[3];
} motion;

static void follow_motion(void) {
    const gaddr moby = hero_moby();
    if (moby == 0) {
        motion.valid = 0;
        return;
    }
    const float* now = (const float*)G(moby + MOBY_POS);
    if (motion.valid && motion.loads == rac3_program_loads()) {
        for (unsigned i = 0; i < 3; ++i) {
            motion.velocity[i] = (now[i] - motion.last[i]) * (float)openrac_game.frame_rate;
        }
    } else {
        memset(motion.velocity, 0, sizeof motion.velocity);
    }
    memcpy(motion.last, now, sizeof motion.last);
    motion.loads = rac3_program_loads();
    motion.valid = 1;
}

static int hero_pose(openrac_game_pose* out) {
    const gaddr moby = hero_moby();
    if (moby == 0) {
        return 0;
    }
    out->level = openrac_game_loaded_overlay();
    for (unsigned i = 0; i < 3; ++i) {
        out->position[i] = GREF(float, moby + MOBY_POS + 4u * i);
        out->rotation[i] = GREF(float, moby + MOBY_ROT + 4u * i);
        out->velocity[i] = motion.valid ? motion.velocity[i] : 0.0f;
    }
    out->scale = GREF(float, moby + MOBY_SCALE);
    out->state = GREF(int32_t, HERO_STATE);
    out->health = GREF(int32_t, HERO_HEALTH);
    const int32_t held = GREF(int32_t, HELD_ITEM);
    out->item = held > 0 ? held : -1;
    /* A blend: the seq is 0xFF, the target the new one (Going Commando's). */
    const uint8_t seq = GREF(uint8_t, moby + MOBY_SEQ);
    const uint8_t target = GREF(uint8_t, moby + MOBY_TARGET_SEQ);
    out->seq = target;
    out->next_frame = GREF(uint8_t, moby + MOBY_NEXT_FRAME);
    if (seq != target) {
        out->frame = out->next_frame;
        out->anim_time = 0.0f;
        out->flags = 1u; /* OPENRAC_POSE_BLENDING */
    } else {
        out->frame = GREF(uint8_t, moby + MOBY_FRAME);
        out->anim_time = GREF(float, moby + MOBY_ANIM_TIME);
        out->flags = 0;
    }
    out->anim_speed = GREF(float, moby + MOBY_ANIM_SPEED);
    return 1;
}

static int32_t bolts(int32_t add) {
    GREF(int32_t, BOLTS) += add;
    return GREF(int32_t, BOLTS);
}

/* ---- Starting and travelling ---- */

int rac3_start_level(int level); /* program.c */

static int known_level(int level) {
    return level >= 1 && level <= 36 && rac3_level_names[level] != NULL;
}

/* From the front end: a new game in `level` (0 or less: Veldin), as its menus leave for a level.
 * A save is refused for now (its unpacking is not paired yet). */
static int start(int level, const uint8_t* save, uint32_t size) {
    (void)size;
    int32_t ignored;
    if (save != NULL || phase(&ignored) != OPENRAC_GAME_PHASE_FRONT_END) {
        return 0;
    }
    const int to = level <= 0 ? 1 : level;
    return known_level(to) && rac3_start_level(to);
}

/* In a level: the level's own request_level_change, as its exits ask. */
static int travel(int level) {
    if (!in_level() || !known_level(level)) {
        return 0;
    }
    return rac3_start_level(level);
}

static int save_info(
    const uint8_t* save,
    uint32_t size,
    int32_t* level,
    int32_t* bolts,
    int32_t* gold_bolts,
    int32_t* seconds
) {
    *gold_bolts = 0;
    return rac3_save_info(save, size, level, bolts, seconds);
}

static int save_check(const uint8_t* save, uint32_t size) {
    int32_t level, bolts, seconds;
    return rac3_save_whole(save, size) && rac3_save_info(save, size, &level, &bolts, &seconds)
           && rac3_level_names[level] != NULL;
}

static char save_folder[32];
static char world_family[32];

static void play_trace(unsigned frame);
static void (*g_previous_frame)(unsigned frame);

static void play_frame(unsigned frame) {
    follow_motion();
    play_trace(frame);
    if (g_previous_frame != NULL) {
        g_previous_frame(frame);
    }
}

static openrac_game_play play = {
    .levels = LEVELS,
    .level_names = rac3_level_names,
    .phase = phase,
    .loads = loads,
    .save_folder = save_folder,
    .save_file = "save%d.bin",
    .save_slots = RAC3_SAVE_SLOTS,
    .save_info = save_info,
    .save_check = save_check,
    .world_family = world_family,
    .hero_pose = hero_pose,
    .start = start,
    .travel = travel,
    .mobys = mobys,
    .moby_slot = moby_slot,
    .moby_at = moby_at,
    .slot_of = slot_of,
    .moby_pose = moby_pose,
    .moby_kill = moby_kill,
    .hero_slot = hero_slot,
    .bolts = bolts,
    .hero_moby = hero_moby,
};

/* From crt0 (entry.c), as tools.c's: a constructor in a file nothing else names is dropped with it
 * at the link. */
void rac3_install_play(void);

void rac3_install_play(void) {
    /* SCES_524.56 -> BESCES-52456RATCHET (the folder the card trace shows the game make). */
    snprintf(save_folder, sizeof save_folder, "%s", "BESCES-52456RATCHET");
    snprintf(world_family, sizeof world_family, "%s", "rac3-pal-1");
    openrac_game_play_host = &play;
    g_previous_frame = openrac_game_on_frame;
    openrac_game_on_frame = play_frame;
}

/* ---- OPENRAC_PLAY_TRACE: the play calls run without a mod, for a hidden run's log ----
 *
 * Any value prints, on stderr (PTRACE lines), every change of the phase, the level and the loads,
 * and every 120 frames in a level the hero's pose and the level's mobys (how many, the hero's slot,
 * the first placed one); `travel=N` leaves for level N 600 frames into play (docs/port/DEBUGGING.md;
 * Going Commando's). */
static void play_trace(unsigned frame) {
    static int on = -1;
    static int last_phase = -2, last_level = -2, travel_to = -1;
    static uint32_t last_loads = 0xFFFFFFFFu;
    static unsigned played;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_PLAY_TRACE");
        on = v != NULL && *v != '\0';
        const char* t = on ? strstr(v, "travel=") : NULL;
        travel_to = t != NULL ? atoi(t + 7) : -1;
    }
    if (!on) {
        return;
    }
    int32_t level = -1;
    const int p = phase(&level);
    if (p != last_phase || level != last_level || loads() != last_loads) {
        fprintf(stderr, "PTRACE %u phase %d level %d loads %u\n", frame, p, (int)level, loads());
        last_phase = p;
        last_level = level;
        last_loads = loads();
        played = 0;
    }
    if (p != OPENRAC_GAME_PHASE_PLAYING) {
        return;
    }
    ++played;
    if (played % 120u == 1u) {
        openrac_game_pose pose;
        if (hero_pose(&pose)) {
            fprintf(
                stderr,
                "PTRACE %u hero at %.2f %.2f %.2f yaw %.2f state %d health %d item %d seq %u frame "
                "%u "
                "speed %.2f %.2f %.2f\n",
                frame,
                (double)pose.position[0],
                (double)pose.position[1],
                (double)pose.position[2],
                (double)pose.rotation[2],
                (int)pose.state,
                (int)pose.health,
                (int)pose.item,
                (unsigned)pose.seq,
                (unsigned)pose.frame,
                (double)pose.velocity[0],
                (double)pose.velocity[1],
                (double)pose.velocity[2]
            );
        }
        openrac_game_moby_state first;
        const int n = mobys(&first, 1);
        fprintf(
            stderr,
            "PTRACE %u mobys %d hero slot %d first slot %d key %d class %d at %.2f %.2f %.2f "
            "bolts %d\n",
            frame,
            n,
            (int)hero_slot(),
            n > 0 ? (int)first.slot : -1,
            n > 0 ? (int)first.key : -1,
            n > 0 ? (int)first.oclass : -1,
            n > 0 ? (double)first.position[0] : 0.0,
            n > 0 ? (double)first.position[1] : 0.0,
            n > 0 ? (double)first.position[2] : 0.0,
            (int)bolts(0)
        );
    }
    if (travel_to > 0 && played == 600u) {
        fprintf(stderr, "PTRACE %u travel %d: %d\n", frame, travel_to, travel(travel_to));
    }
}
