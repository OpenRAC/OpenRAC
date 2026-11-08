/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: what the mods' play calls need of the game (game_host.h, openrac_game_play; the
 * multiplayer mod): the phase and the level, the hero's pose, figures that look like the hero, a
 * game started or a level travelled to from code, the saves, and the level's mobys read and posed.
 * rac1/host/play.c is the model; this is the same approach on rac2's records. tools.c installs it.
 * One copy for every disc (README.md): the hero's fields are read from his record's base, which is
 * g_heroPos less 0x80 on every disc (rac2_addresses.h carries g_heroPos to each).
 *
 * The phase. The executable's program is the title and its menus; a start from it holds the level
 * on its way (tools.c) and asks RequestLevelExit (0x00288CB0), which sets g_nLevelExitRequested and
 * g_nLevelExitDestination: a level is on its way. In a level's program the game state
 * (g_nGameState, the level loop's switch, func_L00_002A3B88) is 0 play, 1 and 2 a scene, 3 the pause
 * menu, 4 a dialog, 5 a vendor, 6 a program's load or the landing that follows it (func_002B49E0
 * requests them). A level asked to end (g_nLevelExitRequested) is on its way to the next.
 *
 * A figure is a moby of the hero's class (his moby's +0xAA), made by SpawnMoby (func_0029EFD0), the
 * routine the game makes the hero's items with (func_L00_002AFBF0). InitMobyFromClass
 * (func_0029F0C8) gives it the class's update, which would run the hero's code: it is cleared (+0x64).
 * The moby loop (UpdateMobys, func_L00_0032CF60) rebuilds its matrix from its rotation (flag 4
 * clear, func_00312B78: +0xF0 into the rows at +0xC0, scaled by +0x2C), so the mod sets its place
 * and the game turns it. Its collision pointer (+0x98) is cleared: func_00312B78 then never puts it
 * in the moby cells (func_002A0F48) and nothing collides with it; its cell rectangle stays the empty
 * one InitMobyFromClass writes (+0xA4, negative), which FreeMoby's cell removal (func_002A0C68)
 * skips. Its look is the hero's: alpha, scale, colour words (+0x38, as the item mobys copy them) and
 * bangles. Its pvars (+0x68, zeroed by SpawnMoby) carry a mark and the program load it was made in.
 *
 * Its animation does not follow the pose yet. The hero's class keeps its frames packed (seq_plain):
 * only his own update unpacks them, through his pose layers, into the two frames his moby shows. A
 * figure shows a copy of the frame the hero showed when it was made, and the moby loops leave its
 * animation alone (flag 0x40); a figure that animated by the moby loop's advance read a packed
 * frame's tagged pointer as an address. Unpacking the pose's seq and key for a figure is the next
 * step (the pose layers, func_L00_00310D30 and MobyBuildPose, func_L00_00316F60). Mobys of other
 * classes, posed by moby_pose, animate as rac1's do (openrac/pose_anim.h).
 *
 * Starting and travelling. A new game is tools.c's start (NewGameInit, 0x00298E08, then the level
 * held as the destination). A save: NewGameInit's fresh state, then SaveUnpack (0x00298D28) of a copy
 * in game memory, the 40-byte settings block kept as NewGameInit keeps it, then the same start to
 * the save's level (or the one asked). A travel is RequestLevelExit to the level, as tools.c's
 * start from a level; -1 is the title, as Quit Game asks it.
 *
 * Saves: save_format.h. SavePack (0x0029ADA0) writes the progress now, as the card's file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "openrac/pose_anim.h"
#include "rac2.h"
#include "save_format.h"

#define LEVELS RAC2_LEVELS
/* The museum is 21 to the game's level word and 30 to its program (level.c). */
#define MUSEUM_GAME 21
#define MUSEUM_PROGRAM 30

/* The hero's record (D_00189E20 on v1.01): g_heroPos is its +0x80. */
#define HERO (RAC2_g_heroPos - 0x80u)
#define HERO_POS RAC2_g_heroPos
#define HERO_ROT RAC2_g_heroOrientVec
#define HERO_MOBY RAC2_g_pHeroMoby
#define HERO_STATE RAC2_g_heroState
#define HERO_HEALTH RAC2_g_health
/* A short counted while he is off the ground (the follow camera's, func_L00_00420C38). */
#define HERO_FRAMES_OFF_GROUND (HERO + 0x34Cu)
/* His item slots, 0x50 bytes each (func_L00_002AFBF0): slot 0 the hand, its moby +0, its item +0x28. */
#define HERO_ITEM_MOBY (HERO + 0x1220u)
#define HERO_ITEM_ID (HERO + 0x1248u)

/* The game state (g_nGameState) and the end of a level's loop: the boot program's words, a level's
 * copy through OPENRAC_DATA. */
#define GAME_STATE OPENRAC_DATA(RAC2_g_nGameState)
#define EXIT_REQUESTED OPENRAC_DATA(RAC2_g_nLevelExitRequested)
#define EXIT_DESTINATION OPENRAC_DATA(RAC2_g_nLevelExitDestination)
#define STATE_PLAY 0
#define STATE_LOADING 6

/* The settings block NewGameInit keeps over its SaveUnpack (D_001A7B90, 40 bytes): g_playerProgress
 * + 0x1A0 on every disc (the PAL executable's data from g_playerProgress is 0x88 bytes on, alike). */
#define SETTINGS (RAC2_g_playerProgress + 0x1A0u)
#define SETTINGS_SIZE 40u

/* A moby (InitMobyFromClass's layout, func_0029F0C8; the moby loop's and MobyProc's reads). */
enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20, /* 0x80 and over: dead or free (0xFD a placed one deleted, 0xFE free) */
    MOBY_ALPHA = 0x23,
    MOBY_CLASS = 0x24,
    MOBY_SCALE = 0x2C,
    MOBY_DRAWN = 0x31,
    MOBY_DRAW_DIST = 0x32,
    MOBY_FLAGS = 0x34,
    MOBY_COLOUR = 0x38, /* 8 bytes */
    MOBY_FRAME = 0x40,
    MOBY_NEXT_FRAME = 0x41,
    MOBY_SEQ = 0x42,        /* 0xFF while a blend plays (func_002A7470) */
    MOBY_TARGET_SEQ = 0x43, /* the seq a blend goes to; the seq itself otherwise */
    MOBY_ANIM_TIME = 0x44,
    MOBY_ANIM_SPEED = 0x48,
    MOBY_FRAME_NOW = 0x58, /* the frame shown and the next (func_0029F390) */
    MOBY_FRAME_NEXT = 0x5C,
    MOBY_UPDATE = 0x64,
    MOBY_PVARS = 0x68,
    MOBY_BANGLES = 0x7C,
    MOBY_COLLISION = 0x98,
    MOBY_OCLASS = 0xAA,
    MOBY_UID = 0xB2,
    MOBY_ROT = 0xF0
};

#define MOBY_SIZE 0x100u

/* Moby flags: not run by the moby loop, matrix not rebuilt, animation not advanced. */
#define FLAG_NOT_RUN 0x2u
#define FLAG_KEEP_MATRIX 0x4u
#define FLAG_NO_ANIMATION 0x40u
/* A class's seq count and seq table, a seq's key count (func_002A7398, MobyAnimAdvance). */
#define CLASS_SEQ_COUNT 0x0Cu
#define CLASS_SEQS 0x48u
#define SEQ_FRAMES 0x10u
#define SEQ_FRAME_TABLE 0x1Cu

#define FIGURE_MARK 0x4F504647u /* "OPFG" */
/* Below this no moby's pvars are (the executable's code). */
#define RAC2_PVARS_LOW 0x00100000u
#define FIGURE_BLEND 4
/* How far away a figure is drawn (+0x32; MobyProc caps it by the level's own limit). */
#define FIGURE_DRAW_DIST 400

/* The largest save a trace reads (the game's are RAC2_SAVE_SIZE). A save on its way in or out
 * of the game is on the game's stack for the call (GFRAME): the statics region has no room for
 * one. */
#define SAVE_ROOM 0x20000u

/* The moby table (the level's copy of the pointers): its first moby, the first that is not one the
 * level placed, and its end. */
#define MOBY_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define MOBY_DYNAMIC OPENRAC_DATA(RAC2_g_mobyTableBase + 4u)
#define MOBY_END OPENRAC_DATA(RAC2_g_mobyTableEnd)

static int level_loaded(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < LEVELS;
}

/* A level's program runs and its loop is not told to end: its mobys and the hero are the level's. */
static int in_level(void) {
    return level_loaded() && GREF(int32_t, EXIT_REQUESTED) == 0;
}

/* The game's level word as the play calls number it (the museum's program number). */
static int32_t play_level(int32_t game_level) {
    if (game_level == MUSEUM_GAME) {
        return MUSEUM_PROGRAM;
    }
    return game_level >= 0 && game_level < LEVELS && rac2_level_names[game_level] != NULL
               ? game_level
               : -1;
}

static int phase(int32_t* level) {
    if (!level_loaded()) {
        /* The title, until a level is on its way. */
        *level = -1;
        if (GREF(int32_t, EXIT_REQUESTED) != 0 || GREF(int32_t, GAME_STATE) == STATE_LOADING) {
            *level = play_level(GREF(int32_t, EXIT_DESTINATION));
            return OPENRAC_GAME_PHASE_LOADING;
        }
        return OPENRAC_GAME_PHASE_FRONT_END;
    }
    if (!in_level()) {
        /* The loop's end: the flight or the load of the level asked for (-1: the title). */
        *level = play_level(GREF(int32_t, EXIT_DESTINATION));
        return OPENRAC_GAME_PHASE_LOADING;
    }
    *level = openrac_game_loaded_overlay();
    switch (GREF(int32_t, GAME_STATE)) {
        case STATE_PLAY:
            return GREF(gaddr, HERO_MOBY) != 0 ? OPENRAC_GAME_PHASE_PLAYING
                                               : OPENRAC_GAME_PHASE_LOADING;
        case 1:
        case 2:
        case STATE_LOADING: /* the landing after the load, in the new level */
            return OPENRAC_GAME_PHASE_SCENE;
        default:
            return OPENRAC_GAME_PHASE_MENU;
    }
}

static uint32_t loads(void) {
    return rac2_program_loads();
}

static uint32_t hero_moby(void) {
    return in_level() ? GREF(gaddr, HERO_MOBY) : 0;
}

/* The seq `seq` of a moby's class: its key count, 0 for a seq the class has not. */
static int seq_frames(gaddr moby, uint32_t seq) {
    const gaddr cls = GREF(gaddr, moby + MOBY_CLASS);
    if (cls == 0 || seq >= GREF(uint8_t, cls + CLASS_SEQ_COUNT)) {
        return 0;
    }
    const gaddr entry = GREF(gaddr, cls + CLASS_SEQS + 4u * seq);
    return entry != 0 ? GREF(uint8_t, entry + SEQ_FRAMES) : 0;
}

/* His velocity: his move between the last two frames (play_frame), in units a second. The record's
 * own velocity is not known on this game. */
static struct {
    uint32_t loads;
    int valid;
    float last[3];
    float velocity[3];
} motion;

static void follow_motion(void) {
    if (hero_moby() == 0) {
        motion.valid = 0;
        return;
    }
    const float* now = (const float*)G(HERO_POS);
    if (motion.valid && motion.loads == rac2_program_loads()) {
        for (unsigned i = 0; i < 3; ++i) {
            motion.velocity[i] = (now[i] - motion.last[i]) * (float)openrac_game.frame_rate;
        }
    } else {
        memset(motion.velocity, 0, sizeof motion.velocity);
    }
    memcpy(motion.last, now, sizeof motion.last);
    motion.loads = rac2_program_loads();
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
    out->item = GREF(gaddr, HERO_ITEM_MOBY) != 0 ? GREF(int32_t, HERO_ITEM_ID) : -1;
    /* A blend (func_002A7470): the seq is 0xFF, the target the new one; the key it arrives at is
     * the next key. */
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
    if (GREF(int16_t, HERO_FRAMES_OFF_GROUND) != 0) {
        out->flags |= 16u; /* OPENRAC_POSE_AIRBORNE */
    }
    out->anim_speed = GREF(float, moby + MOBY_ANIM_SPEED);
    return 1;
}

static int32_t slot_of(gaddr moby);

static int figure_alive(gaddr moby) {
    /* A moby of this level's table (a figure of a level before is an address of another table),
     * live, with its pvars in memory and marked as made in this program load. */
    if (moby == 0 || !in_level() || slot_of(moby) < 0 || GREF(uint8_t, moby + MOBY_STATE) >= 0x80) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    return pvars >= RAC2_PVARS_LOW && pvars < openrac_guest_ram_size - 16u
           && GREF(uint32_t, pvars) == FIGURE_MARK
           && GREF(uint32_t, pvars + 4u) == rac2_program_loads()
           && GREF(int32_t, pvars + 8u) == GREF(int16_t, moby + MOBY_OCLASS);
}

/* Whether seq `seq` of a moby's class has frames the moby loop's animation reads (MobyAnimAdvance,
 * func_002A7470): the hero's class (0) keeps its frames packed, their pointers tagged with 0xF in
 * the top nibble, and only his own update unpacks them (his pose layers at +0x50, func_L00_00310D30
 * masks the tag) into two buffers his moby's +0x58 and +0x5C point at; the moby loop's advance
 * reads a tagged pointer as an address (a crash). */
static int seq_plain(gaddr moby, uint32_t seq) {
    if (seq_frames(moby, seq) == 0) {
        return 0;
    }
    const gaddr cls = GREF(gaddr, moby + MOBY_CLASS);
    const gaddr entry = GREF(gaddr, cls + CLASS_SEQS + 4u * seq);
    return (GREF(uint32_t, entry + SEQ_FRAME_TABLE) >> 28) == 0;
}

/* A figure's frame: room for one unpacked frame of the hero's (his two buffers are 0x600 bytes
 * apart), a few figures' worth taken in turn. */
#define FIGURE_FRAME_BYTES 0x600u
#define FIGURE_FRAMES 8

static gaddr figure_frame(void) {
    static gaddr room;
    static unsigned next;
    if (room == 0) {
        room = openrac_guest_static(FIGURE_FRAME_BYTES * FIGURE_FRAMES);
    }
    return room + FIGURE_FRAME_BYTES * (next++ % FIGURE_FRAMES);
}

static gaddr figure_spawn(void) {
    const gaddr hero = hero_moby();
    if (hero == 0) {
        return 0;
    }
    const int32_t oclass = GREF(int16_t, hero + MOBY_OCLASS);
    /* No room in the level's pool: no figure. */
    const gaddr moby = func_0029EFD0(oclass);
    if (moby == 0) {
        return 0;
    }
    GREF(gaddr, moby + MOBY_UPDATE) = 0;
    GREF(gaddr, moby + MOBY_COLLISION) = 0;
    GREF(uint16_t, moby + MOBY_FLAGS) =
        (uint16_t)(GREF(uint16_t, moby + MOBY_FLAGS) & ~(FLAG_NOT_RUN | FLAG_KEEP_MATRIX));
    /* The hero's class keeps its frames packed (below): the figure holds the frame he shows now,
     * a copy of his unpacked one, and the moby loops leave its animation alone. */
    if (!seq_plain(moby, GREF(uint8_t, moby + MOBY_TARGET_SEQ))) {
        const gaddr frame = figure_frame();
        memcpy(G(frame), G(GREF(gaddr, hero + MOBY_FRAME_NOW)), FIGURE_FRAME_BYTES);
        GREF(gaddr, moby + MOBY_FRAME_NOW) = frame;
        GREF(gaddr, moby + MOBY_FRAME_NEXT) = frame;
        GREF(float, moby + MOBY_ANIM_TIME) = 0.0f;
        GREF(uint16_t, moby + MOBY_FLAGS) =
            (uint16_t)(GREF(uint16_t, moby + MOBY_FLAGS) | FLAG_NO_ANIMATION);
    }
    /* The hero's look, drawn as far as a player can be seen (the item mobys' 1 at +0x31 too). */
    GREF(uint8_t, moby + MOBY_ALPHA) = GREF(uint8_t, hero + MOBY_ALPHA);
    GREF(float, moby + MOBY_SCALE) = GREF(float, hero + MOBY_SCALE);
    GREF(uint8_t, moby + MOBY_DRAWN) = 1;
    GREF(int16_t, moby + MOBY_DRAW_DIST) = FIGURE_DRAW_DIST;
    memcpy(G(moby + MOBY_COLOUR), G(hero + MOBY_COLOUR), 8);
    GREF(int16_t, moby + MOBY_BANGLES) = GREF(int16_t, hero + MOBY_BANGLES);
    /* Where the hero is until the mod poses it. */
    memcpy(G(moby + MOBY_POS), G(hero + MOBY_POS), 16);
    memcpy(G(moby + MOBY_ROT), G(hero + MOBY_ROT), 16);
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars != 0) {
        GREF(uint32_t, pvars) = FIGURE_MARK;
        GREF(uint32_t, pvars + 4u) = rac2_program_loads();
        GREF(int32_t, pvars + 8u) = oclass;
    }
    return moby;
}

/* A moby put in a pose: its place, rotation and scale, and its animation, blended into a new seq and
 * kept to the pose's place in it by its speed (openrac/pose_anim.h). */
static void pose_moby(gaddr moby, const openrac_game_pose* pose) {
    for (unsigned i = 0; i < 3; ++i) {
        GREF(float, moby + MOBY_POS + 4u * i) = pose->position[i];
        GREF(float, moby + MOBY_ROT + 4u * i) = pose->rotation[i];
    }
    if (pose->scale > 0.0f) {
        GREF(float, moby + MOBY_SCALE) = pose->scale;
    }
    const int frames = seq_frames(moby, pose->seq);
    if (frames == 0 || !seq_plain(moby, pose->seq)
        || (GREF(uint16_t, moby + MOBY_FLAGS) & FLAG_NO_ANIMATION) != 0) {
        return;
    }
    const pose_anim_in in = {
        .frames = frames,
        .moby_seq = GREF(uint8_t, moby + MOBY_SEQ),
        .moby_target = GREF(uint8_t, moby + MOBY_TARGET_SEQ),
        .moby_frame = GREF(uint8_t, moby + MOBY_FRAME),
        .moby_time = GREF(float, moby + MOBY_ANIM_TIME),
        .pose_seq = (int)pose->seq,
        .pose_frame = (int)pose->frame < frames ? (int)pose->frame : frames - 1,
        .pose_time = pose->anim_time,
        .pose_speed = pose->anim_speed,
        .pose_blending = (pose->flags & 1u) != 0, /* OPENRAC_POSE_BLENDING */
    };
    float speed = 0.0f;
    switch (pose_anim_step(&in, &speed)) {
        case POSE_ANIM_BLEND:
            /* A new seq: blended into, as the game's mobys change theirs (func_002A7470, which sets
             * the speed itself). */
            func_002A7470(moby, in.pose_seq, in.pose_frame, FIGURE_BLEND);
            return;
        case POSE_ANIM_BLENDING:
            return;
        case POSE_ANIM_SET:
            func_002A7398(moby, in.pose_seq, in.pose_frame);
            GREF(float, moby + MOBY_ANIM_TIME) = in.pose_blending ? 0.0f : in.pose_time;
            break;
        default:
            break;
    }
    GREF(float, moby + MOBY_ANIM_SPEED) = speed;
}

static void figure_pose(gaddr moby, const openrac_game_pose* pose) {
    if (figure_alive(moby)) {
        pose_moby(moby, pose);
    }
}

static void figure_remove(gaddr moby) {
    if (!figure_alive(moby)) {
        return;
    }
    GREF(uint32_t, GREF(gaddr, moby + MOBY_PVARS)) = 0;
    func_0029F338(moby);
}

/* ---- The level's mobys (read, posed, deleted) ---- */

static int moby_table(gaddr* base, gaddr* dynamic, gaddr* end) {
    if (!in_level()) {
        return 0;
    }
    *base = GREF(gaddr, MOBY_BASE);
    *dynamic = GREF(gaddr, MOBY_DYNAMIC);
    *end = GREF(gaddr, MOBY_END);
    return *base != 0 && *end > *base && (*end - *base) % MOBY_SIZE == 0 && *dynamic >= *base
           && *dynamic <= *end;
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

static void moby_kill(gaddr moby) {
    if (slot_of(moby) >= 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80 && moby != hero_moby()) {
        func_0029F338(moby);
    }
}

static int32_t hero_slot(void) {
    return slot_of(hero_moby());
}

static int camera_position(float out[3]) {
    if (!in_level()) {
        return 0;
    }
    memcpy(out, G(OPENRAC_DATA(RAC2_g_cameraPos)), 12);
    return 1;
}

static int32_t bolts(int32_t add) {
    GREF(int32_t, RAC2_g_boltCount) += add;
    return GREF(int32_t, RAC2_g_boltCount);
}

/* ---- Starting, travelling, saves ---- */

static int save_info(
    const uint8_t* save,
    uint32_t size,
    int32_t* level,
    int32_t* bolts_out,
    int32_t* gold_bolts,
    int32_t* seconds
) {
    return rac2_save_info(save, size, level, bolts_out, gold_bolts, seconds);
}

static int save_check(const uint8_t* save, uint32_t size) {
    int32_t level, b, gold, seconds;
    return rac2_save_whole(save, size) && rac2_save_info(save, size, &level, &b, &gold, &seconds)
           && play_level(level) >= 0;
}

static int start(int level, const uint8_t* save, uint32_t size) {
    if (level >= LEVELS || level_loaded()) {
        return 0;
    }
    if (save == NULL) {
        const int to = level < 0 ? 0 : level;
        if (rac2_level_names[to] == NULL) {
            return 0;
        }
        rac2_start_level(to, 1);
        return 1;
    }
    if (!save_check(save, size) || (level >= 0 && rac2_level_names[level] == NULL)) {
        return 0;
    }
    GFRAME(room, RAC2_SAVE_SIZE);
    memcpy(G(room), save, size);
    /* A fresh state under the save, then the save over it, the player's settings kept. */
    func_00298E08();
    uint8_t settings[SETTINGS_SIZE];
    memcpy(settings, G(SETTINGS), SETTINGS_SIZE);
    func_00298D28(room);
    memcpy(G(SETTINGS), settings, SETTINGS_SIZE);
    const int to = level >= 0 ? level : play_level(GREF(int32_t, RAC2_g_playerProgress));
    if (to < 0) {
        return 0;
    }
    rac2_start_level(to, 0);
    return 1;
}

static int travel(int level) {
    /* -1 is the title, as Quit Game goes there. */
    if (level < -1 || level >= LEVELS || !in_level()
        || (level >= 0 && rac2_level_names[level] == NULL)) {
        return 0;
    }
    if (level < 0) {
        func_00288CB0(-1, 0);
    } else {
        rac2_start_level(level, 0);
    }
    return 1;
}

static uint32_t save_capture(uint8_t* out, uint32_t size) {
    if (!in_level()) {
        return 0;
    }
    GFRAME(room, RAC2_SAVE_SIZE);
    func_0029ADA0(room);
    const uint8_t* bytes = (const uint8_t*)G(room);
    if (rac2_save_u32(bytes) != RAC2_SAVE_GLOBAL_SIZE
        || rac2_save_u32(bytes + 4) != RAC2_SAVE_LEVEL_SIZE) {
        return 0;
    }
    if (out != NULL && size >= RAC2_SAVE_SIZE) {
        memcpy(out, bytes, RAC2_SAVE_SIZE);
    }
    return RAC2_SAVE_SIZE;
}

/* The card's folder (the game forms it from the disc's serial: "B" + the region's letter + the
 * serial, then "RATCHET"), and the world a game's discs belong to. */
static char save_folder[32];
static char world_family[32];

static void play_trace(void);

void rac2_play_frame(void) {
    follow_motion();
    play_trace();
}

static openrac_game_play play = {
    .levels = LEVELS,
    .level_names = rac2_level_names,
    .phase = phase,
    .loads = loads,
    .hero_pose = hero_pose,
    .figure_spawn = figure_spawn,
    .figure_alive = figure_alive,
    .figure_pose = figure_pose,
    .figure_remove = figure_remove,
    .start = start,
    .travel = travel,
    .save_capture = save_capture,
    .save_folder = save_folder,
    .save_file = "save%d.bin",
    .save_slots = 4,
    .save_info = save_info,
    .save_check = save_check,
    .mobys = mobys,
    .moby_slot = moby_slot,
    .moby_at = moby_at,
    .slot_of = slot_of,
    .moby_pose = pose_moby,
    .moby_kill = moby_kill,
    .hero_slot = hero_slot,
    .bolts = bolts,
    .camera_position = camera_position,
    .world_family = world_family,
    .hero_moby = hero_moby,
};

void rac2_install_play(void) {
    /* SCUS_972.68 -> BASCUS-97268RATCHET; SCES_516.07 -> BESCES-51607RATCHET. */
    const char* serial = openrac_game.serial != NULL ? openrac_game.serial : "SCUS_972.68";
    char digits[8] = {0};
    int n = 0;
    for (const char* c = serial + 4; *c != '\0' && n < 5; ++c) {
        if (*c >= '0' && *c <= '9') {
            digits[n++] = *c;
        }
    }
    snprintf(
        save_folder,
        sizeof save_folder,
        "B%c%.4s-%sRATCHET",
        serial[2] == 'E' ? 'E' : 'A',
        serial,
        digits
    );
    /* Each disc its own world until the three are shown to share their levels' data and ticks:
     * PAL runs at 50 Hz with code of its own, v2.00 has 1,117 functions of its own. */
    snprintf(world_family, sizeof world_family, "%s-1", openrac_game.id);
    openrac_game_play_host = &play;
}

/* ---- OPENRAC_PLAY_TRACE: the play calls run without a mod, for a hidden run's log ----
 *
 * Its value lists what to do, by commas: every change of the phase, the level and the loads, and the
 * hero's pose every 120 frames, are printed on stderr (PTRACE lines) whatever it says; `figure` makes
 * a figure once the hero plays and poses it each frame as the hero was 30 frames before, 2 units to
 * his side; `capture` takes the save 300 frames into play and checks it whole; `travel=N` leaves for
 * level N 600 frames into play; `start=FILE` starts the save in FILE from the title, `start=N` a new
 * game in level N (docs/port/DEBUGGING.md). */
#define TRACE_DELAY 30

static void print_pose(const char* who, const openrac_game_pose* p) {
    fprintf(
        stderr,
        "PTRACE %s level %d pos %.2f %.2f %.2f rot %.3f %.3f %.3f scale %.3f vel %.2f %.2f %.2f "
        "state %d health %d item %d seq %u frame %u next %u time %.3f speed %.3f flags %u\n",
        who,
        (int)p->level,
        p->position[0],
        p->position[1],
        p->position[2],
        p->rotation[0],
        p->rotation[1],
        p->rotation[2],
        p->scale,
        p->velocity[0],
        p->velocity[1],
        p->velocity[2],
        (int)p->state,
        (int)p->health,
        (int)p->item,
        (unsigned)p->seq,
        (unsigned)p->frame,
        (unsigned)p->next_frame,
        p->anim_time,
        p->anim_speed,
        (unsigned)p->flags
    );
}

static void trace_start(const char* what) {
    if (strncmp(what, "start=", 6) != 0) {
        return;
    }
    const char* arg = what + 6;
    char* end = NULL;
    const long level = strtol(arg, &end, 10);
    int ok;
    if (end != arg && (*end == '\0' || *end == ',')) {
        ok = start((int)level, NULL, 0);
        fprintf(stderr, "PTRACE start new game %ld: %d\n", level, ok);
        return;
    }
    char path[512];
    size_t n = 0;
    while (arg[n] != '\0' && arg[n] != ',' && n + 1 < sizeof path) {
        path[n] = arg[n];
        ++n;
    }
    path[n] = '\0';
    static uint8_t bytes[SAVE_ROOM];
    FILE* f = fopen(path, "rb");
    const size_t size = f != NULL ? fread(bytes, 1, sizeof bytes, f) : 0;
    if (f != NULL) {
        fclose(f);
    }
    int32_t lv = -1, b = 0, gold = 0, seconds = 0;
    const int info = save_info(bytes, (uint32_t)size, &lv, &b, &gold, &seconds);
    ok = start(-1, bytes, (uint32_t)size);
    fprintf(
        stderr,
        "PTRACE start save %s: %zu bytes, info %d level %d bolts %d seconds %d, check %d, started "
        "%d\n",
        path,
        size,
        info,
        (int)lv,
        (int)b,
        (int)seconds,
        save_check(bytes, (uint32_t)size),
        ok
    );
}

static void play_trace(void) {
    static const char* script;
    static int read;
    if (!read) {
        read = 1;
        script = openrac_env_string("OPENRAC_PLAY_TRACE");
    }
    if (script == NULL) {
        return;
    }
    static int last_phase = -99, last_level = -99, started, captured, travelled, playing;
    static uint32_t last_loads;
    static unsigned frame;
    static gaddr figure;
    static openrac_game_pose history[TRACE_DELAY];
    ++frame;
    int32_t level = -1;
    const int now = phase(&level);
    if (now != last_phase || level != last_level || loads() != last_loads) {
        fprintf(
            stderr,
            "PTRACE frame %u phase %d level %d loads %u overlay %d game_state %d exit %d dest %d "
            "hero_moby 0x%X\n",
            frame,
            now,
            (int)level,
            (unsigned)loads(),
            openrac_game_loaded_overlay(),
            (int)GREF(int32_t, GAME_STATE),
            (int)GREF(int32_t, EXIT_REQUESTED),
            (int)GREF(int32_t, EXIT_DESTINATION),
            (unsigned)GREF(gaddr, HERO_MOBY)
        );
        last_phase = now;
        last_level = level;
        last_loads = loads();
        if (now != OPENRAC_GAME_PHASE_PLAYING) {
            playing = 0;
        }
    }
    if (now == OPENRAC_GAME_PHASE_FRONT_END && !started
        && GREF(int32_t, RAC2_g_frameCounter) >= 30) {
        const char* s = strstr(script, "start=");
        if (s != NULL) {
            started = 1;
            trace_start(s);
        }
    }
    openrac_game_pose pose;
    if (now != OPENRAC_GAME_PHASE_PLAYING || !hero_pose(&pose)) {
        return;
    }
    ++playing;
    history[playing % TRACE_DELAY] = pose;
    if (playing % 120 == 1) {
        const gaddr hero = hero_moby();
        const gaddr cls = GREF(gaddr, hero + MOBY_CLASS);
        print_pose("hero", &pose);
        fprintf(
            stderr,
            "PTRACE hero moby 0x%X class %d seqs %u record rot %.3f %.3f %.3f pos %.2f %.2f %.2f "
            "slot %d mobys %d camera %d\n",
            (unsigned)hero,
            (int)GREF(int16_t, hero + MOBY_OCLASS),
            cls != 0 ? (unsigned)GREF(uint8_t, cls + CLASS_SEQ_COUNT) : 0u,
            GREF(float, HERO_ROT),
            GREF(float, HERO_ROT + 4u),
            GREF(float, HERO_ROT + 8u),
            GREF(float, HERO_POS),
            GREF(float, HERO_POS + 4u),
            GREF(float, HERO_POS + 8u),
            (int)hero_slot(),
            mobys(NULL, 0),
            (int)(moby_at(hero_slot()) == hero)
        );
    }
    if (strstr(script, "figure") != NULL) {
        if (!figure_alive(figure)) {
            if (figure != 0) {
                fprintf(stderr, "PTRACE figure 0x%X gone\n", (unsigned)figure);
            }
            figure = figure_spawn();
            fprintf(
                stderr,
                "PTRACE figure spawn 0x%X slot %d alive %d\n",
                (unsigned)figure,
                (int)slot_of(figure),
                figure_alive(figure)
            );
        }
        if (figure != 0 && playing > TRACE_DELAY) {
            openrac_game_pose late = history[(playing + 1) % TRACE_DELAY];
            late.position[0] += 2.0f;
            figure_pose(figure, &late);
            if (playing % 120 == 1) {
                openrac_game_pose seen = late;
                seen.seq = GREF(uint8_t, figure + MOBY_TARGET_SEQ);
                seen.frame = GREF(uint8_t, figure + MOBY_FRAME);
                seen.next_frame = GREF(uint8_t, figure + MOBY_NEXT_FRAME);
                seen.anim_time = GREF(float, figure + MOBY_ANIM_TIME);
                seen.anim_speed = GREF(float, figure + MOBY_ANIM_SPEED);
                for (unsigned i = 0; i < 3; ++i) {
                    seen.position[i] = GREF(float, figure + MOBY_POS + 4u * i);
                    seen.rotation[i] = GREF(float, figure + MOBY_ROT + 4u * i);
                }
                print_pose("figure", &seen);
                fprintf(
                    stderr,
                    "PTRACE figure state %u flags 0x%X update 0x%X drawn %u row0 %.3f %.3f %.3f\n",
                    (unsigned)GREF(uint8_t, figure + MOBY_STATE),
                    (unsigned)GREF(uint16_t, figure + MOBY_FLAGS),
                    (unsigned)GREF(gaddr, figure + MOBY_UPDATE),
                    (unsigned)GREF(uint8_t, figure + MOBY_DRAWN),
                    GREF(float, figure + 0xC0u),
                    GREF(float, figure + 0xC4u),
                    GREF(float, figure + 0xC8u)
                );
            }
        }
    }
    if (strstr(script, "capture") != NULL && !captured && playing == 300) {
        captured = 1;
        static uint8_t bytes[SAVE_ROOM];
        const uint32_t size = save_capture(bytes, sizeof bytes);
        int32_t lv = -1, b = 0, gold = 0, seconds = 0;
        const int info = save_info(bytes, size, &lv, &b, &gold, &seconds);
        fprintf(
            stderr,
            "PTRACE capture %u bytes whole %d info %d level %d bolts %d gold %d seconds %d\n",
            (unsigned)size,
            rac2_save_whole(bytes, size),
            info,
            (int)lv,
            (int)b,
            (int)gold,
            (int)seconds
        );
        const char* out = openrac_env_string("OPENRAC_PLAY_TRACE_SAVE");
        FILE* f = out != NULL ? fopen(out, "wb") : NULL;
        if (f != NULL) {
            fwrite(bytes, 1, size, f);
            fclose(f);
        }
    }
    const char* t = strstr(script, "travel=");
    if (t != NULL && !travelled && playing == 600) {
        travelled = 1;
        if (figure != 0) {
            figure_remove(figure);
            fprintf(stderr, "PTRACE figure removed, alive %d\n", figure_alive(figure));
        }
        fprintf(stderr, "PTRACE travel %d: %d\n", atoi(t + 7), travel(atoi(t + 7)));
    }
}
