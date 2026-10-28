/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: what the mods' play calls need of the game (game_host.h, openrac_game_play;
 * interface 1.8, multiplayer): the phase and the level, the hero's pose, figures that look like
 * the hero, a game started or a level travelled to from code, and the saves. boot.c installs it.
 * One table for every disc: the addresses are games/rac1's, each version's through its
 * rac1_version.h.
 *
 * The phase. In the boot program (the title, its menus, the flight between planets) the title
 * loop's exit (D_0015F690, space_load_pending) and the game mode (D_0015F6E8: 6 the flight) tell
 * the front end from a level on its way. In a level the level's game mode (D_L00_0015F6A8, the
 * level loop's switch, level_runtime_and_menus.c): 0 play, 1 a movie, 2 a cutscene, 6 the ship's
 * landing or take-off, 7 a slideshow, 3 paused, 4 a dialog, 5 a vendor, -1 the debug menu.
 *
 * A figure is a moby of the hero's class, 0, which every level has loaded (initialize_hero binds
 * the hero to the level's placed moby of class 0): CreateMoby (func_0020D348) makes one as the
 * space scene's companion is made (space.c). It is the hero's look only: no update function (the
 * class's would run the hero's), no collision (+0x94 0, every primitive off at +0x98), out of the
 * target list (flag 0x1000, which the hero's lock-on and weapons read). The moby loop (UpdateMobys)
 * still advances its animation (MobyAnimAdvance, flag 0x40 clear) and rebuilds its matrix from
 * its rotation (flag 4 clear), so the mod sets its pose and the game animates it. Its look is the
 * hero's moby's: its alpha, scale, drawing distances and colour words. Its pvars (zeroed by
 * CreateMoby) carry a mark and the level load it was made in, so a moby the level reused (or a
 * level loaded again) is not taken for it.
 *
 * Starting and travelling. A new game is rac1_start_level (boot.c), as the developer menu's. A
 * save: NewGameInit's fresh state (func_00209DC0), then the save restored over it
 * (memcard_RestoreGame, func_00209CE8, with the port's options kept: options.c) from a copy in
 * game memory, the level its own, and the same start as a new game's (0x13E15A, the title's exit).
 * A travel is what Quit Game does (to -1, the title), or the same with a level in place of -1:
 * the level asked for (initialize_global_state_entry, func_0022F4A0) and the level loop told to
 * end (level_loop_exit_requested, D_L00_0015F650), whose end runs DoSpaceTransition to it.
 *
 * Saves. A save file of the memory card (memcard_MakeWholeSave's layout, func_0020BA00): the sizes
 * of its game part and of each of its 20 level parts, then the game part, whose first blocks are
 * {id, size, value}: id 0 the level, 1 the bolts, 2 the gold bolts, 3 the play time in ticks (60 a
 * second: the Load page's hh:mm), as memcard_RestoreInfo reads them. An empty slot's level is -1.
 */
#include <stdio.h>
#include <string.h>

#include "openrac/host_env.h"
#include "pose_anim.h"
#include "rac1.h"

#define LEVELS 19

/* The level being played, or -1 (boot.c). */
#define CURRENT_LEVEL RAC1_ADDR(0x0015EE84u)
/* The boot program's game mode (6: the flight between planets), the title loop's exit (a short:
 * a level is on its way), and the level a load goes to (initialize_global_state_entry's; data the
 * level programs move, so relocated). */
#define BOOT_MODE RAC1_ADDR(0x0015F6E8u)
#define TITLE_EXIT RAC1_ADDR(0x0015F690u)
#define LOAD_LEVEL OPENRAC_DATA(RAC1_ADDR(0x0015F6E4u))
/* A level's game mode and its loop's end (level data: level 0's addresses, relocated). */
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define LEVEL_EXIT OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F650u))
/* The flag that starts the loaded new game (boot.c). */
#define START_GAME RAC1_ADDR(0x0013E15Au)

/* The hero (include/hero.h): his moby, his velocity a tick, his state, his health, his first item
 * slot (its moby, its item's id), his animation's speed. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_VELOCITY (HERO + 0xE0u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_HEALTH (HERO + 0x22A8u)
#define HERO_ITEM_MOBY (HERO + 0x1090u)
#define HERO_ITEM_ID (HERO + 0x10B8u)
#define HERO_ANIM_SPEED (HERO + 0xA90u)

/* A moby (include/moby.h). */
enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20, /* 0x80 and over: dead or free */
    MOBY_ALPHA = 0x23,
    MOBY_CLASS = 0x24,
    MOBY_SCALE = 0x2C,
    MOBY_UPDATE_DIST = 0x30,
    MOBY_DRAW_DIST = 0x32,
    MOBY_FLAGS = 0x34,
    MOBY_COLOUR = 0x38, /* 8 bytes */
    MOBY_ROT = 0x40,
    MOBY_FRAME = 0x50,
    MOBY_NEXT_FRAME = 0x51,
    MOBY_SEQ = 0x52,
    MOBY_TARGET_SEQ = 0x53, /* the seq a blend goes to; the seq itself otherwise */
    MOBY_ANIM_TIME = 0x54,
    MOBY_ANIM_SPEED = 0x58,
    MOBY_UPDATE = 0x74,
    MOBY_PVARS = 0x78,
    MOBY_LIGHT = 0x80,
    MOBY_COLLISION = 0x94,
    MOBY_COLLISION_OFF = 0x98,
    MOBY_OCLASS = 0xA6
};

/* Moby flags: not run by the moby loop (the hero's), matrix not rebuilt, animation not advanced,
 * in the target list. */
#define FLAG_NOT_RUN 0x2u
#define FLAG_KEEP_MATRIX 0x4u
#define FLAG_NO_ANIMATION 0x40u
#define FLAG_TARGET 0x1000u
/* A class's seq count, its seq table, a seq's key count. */
#define CLASS_SEQ_COUNT 0xCu
#define CLASS_SEQS 0x48u
#define SEQ_FRAMES 0x10u

/* The hero's class, the figure's mark in its pvars, and the ticks a new seq blends over. */
#define HERO_CLASS 0
#define FIGURE_MARK 0x4F504647u /* "OPFG" */
#define FIGURE_BLEND 4
/* How far away a figure is drawn, in game units (the renderer's depth test, +0x32). */
#define FIGURE_DRAW_DIST 400

/* The save file's parts (above), and the largest save the scratch room holds. */
#define SAVE_HEADER 8u
#define SAVE_LEVEL_PARTS 20u
#define SAVE_ROOM 0x10000u
#define TICKS_A_SECOND 60

/* Room in game memory for a save on its way in or out, made at its first use (a static of the
 * port's: a warm savestate taken before it refuses to restore after, as for any new static). */
static gaddr save_room(void) {
    static gaddr room;
    if (room == 0) {
        room = openrac_guest_static(SAVE_ROOM);
    }
    return room;
}

static int level_loaded(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < LEVELS;
}

int rac1_level_running(void) {
    return level_loaded() && GREF(int, LEVEL_EXIT) == 0;
}

static int in_level(void) {
    return rac1_level_running();
}

static int phase(int32_t* level) {
    /* The level's loop has ended: on the way to the level asked for (the flight between planets
     * plays in the old level's program). */
    if (level_loaded() && !in_level()) {
        const int next = GREF(int, LOAD_LEVEL);
        *level = next >= 0 && next < LEVELS ? next : -1;
        return OPENRAC_GAME_PHASE_LOADING;
    }
    if (!in_level()) {
        const int current = GREF(int, CURRENT_LEVEL);
        const int on_way = GREF(short, TITLE_EXIT) != 0 || GREF(int, BOOT_MODE) == 6;
        *level = -1;
        /* The title's menus, until a level is on its way. */
        if (!on_way) {
            return OPENRAC_GAME_PHASE_FRONT_END;
        }
        *level = current >= 0 && current < LEVELS ? current : GREF(int, LOAD_LEVEL);
        if (*level < 0 || *level >= LEVELS) {
            *level = -1;
        }
        return OPENRAC_GAME_PHASE_LOADING;
    }
    *level = openrac_game_loaded_overlay();
    switch (GREF(int, LEVEL_MODE)) {
        case 0:
            /* Play once the hero is bound to his moby (initialize_hero, after the level's load). */
            return GREF(gaddr, HERO_MOBY) != 0 ? OPENRAC_GAME_PHASE_PLAYING
                                               : OPENRAC_GAME_PHASE_LOADING;
        case 1:
        case 2:
        case 6:
        case 7:
            return OPENRAC_GAME_PHASE_SCENE;
        default:
            return OPENRAC_GAME_PHASE_MENU;
    }
}

static uint32_t loads(void) {
    return rac1_program_loads();
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

static int hero_pose(openrac_game_pose* out) {
    if (!in_level()) {
        return 0;
    }
    const gaddr moby = GREF(gaddr, HERO_MOBY);
    if (moby == 0) {
        return 0;
    }
    out->level = openrac_game_loaded_overlay();
    for (unsigned i = 0; i < 3; ++i) {
        out->position[i] = GREF(float, moby + MOBY_POS + 4u * i);
        out->rotation[i] = GREF(float, moby + MOBY_ROT + 4u * i);
        /* A tick's move, by the ticks a second. */
        out->velocity[i] = GREF(float, HERO_VELOCITY + 4u * i) * (float)openrac_game.frame_rate;
    }
    out->scale = GREF(float, moby + MOBY_SCALE);
    out->state = GREF(int32_t, HERO_STATE);
    out->health = GREF(int32_t, HERO_HEALTH);
    out->item = GREF(gaddr, HERO_ITEM_MOBY) != 0 ? GREF(int32_t, HERO_ITEM_ID) : -1;
    /* A blend (advance_hero_animation): the seq is still the old one, the target the new; the
     * key it arrives at is the next key. */
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
    out->anim_speed = GREF(float, HERO_ANIM_SPEED);
    return 1;
}

int rac1_figure_alive(gaddr moby) {
    if (moby == 0 || !in_level() || GREF(uint8_t, moby + MOBY_STATE) >= 0x80
        || GREF(int16_t, moby + MOBY_OCLASS) != HERO_CLASS) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    return pvars != 0 && GREF(uint32_t, pvars) == FIGURE_MARK
           && GREF(uint32_t, pvars + 4u) == rac1_program_loads();
}

static gaddr figure_spawn(void) {
    const gaddr hero = in_level() ? GREF(gaddr, HERO_MOBY) : 0;
    if (hero == 0) {
        return 0;
    }
    /* No room in the level's pool: no figure. */
    const gaddr moby = func_0020D348(HERO_CLASS);
    if (moby == 0) {
        return 0;
    }
    GREF(gaddr, moby + MOBY_UPDATE) = 0;
    GREF(uint32_t, moby + MOBY_COLLISION) = 0;
    GREF(int32_t, moby + MOBY_COLLISION_OFF) = -1;
    GREF(uint16_t, moby + MOBY_FLAGS) =
        (uint16_t)(GREF(uint16_t, moby + MOBY_FLAGS)
                   & ~(FLAG_NOT_RUN | FLAG_KEEP_MATRIX | FLAG_NO_ANIMATION | FLAG_TARGET));
    /* The hero's look; updated wherever it is, and drawn as far as a player can be seen (the
     * hero's own drawing distance is no guide: the game draws him apart). */
    GREF(uint8_t, moby + MOBY_ALPHA) = GREF(uint8_t, hero + MOBY_ALPHA);
    GREF(float, moby + MOBY_SCALE) = GREF(float, hero + MOBY_SCALE);
    GREF(uint8_t, moby + MOBY_UPDATE_DIST) = 0xFF;
    GREF(int16_t, moby + MOBY_DRAW_DIST) = FIGURE_DRAW_DIST;
    memcpy(G(moby + MOBY_COLOUR), G(hero + MOBY_COLOUR), 8);
    GREF(uint32_t, moby + MOBY_LIGHT) = GREF(uint32_t, hero + MOBY_LIGHT);
    /* Where the hero is until the mod poses it. */
    memcpy(G(moby + MOBY_POS), G(hero + MOBY_POS), 16);
    memcpy(G(moby + MOBY_ROT), G(hero + MOBY_ROT), 16);
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars != 0) {
        GREF(uint32_t, pvars) = FIGURE_MARK;
        GREF(uint32_t, pvars + 4u) = rac1_program_loads();
    }
    return moby;
}

/* What the last rac1_pose_moby did to the animation (for the trace). */
static const char* pose_action = "";

/* OPENRAC_ANIM_TRACE set: what a figure's animation did at each pose, a line on stderr. */
static void trace_moby(gaddr moby, const char* what) {
    static int on = -1;
    if (on < 0) {
        on = openrac_env_string("OPENRAC_ANIM_TRACE") != NULL;
    }
    if (on) {
        fprintf(
            stderr,
            "ATRACE F %s %u %u %u %u %.4f %.4f\n",
            what,
            (unsigned)GREF(uint8_t, moby + MOBY_SEQ),
            (unsigned)GREF(uint8_t, moby + MOBY_TARGET_SEQ),
            (unsigned)GREF(uint8_t, moby + MOBY_FRAME),
            (unsigned)GREF(uint8_t, moby + MOBY_NEXT_FRAME),
            GREF(float, moby + MOBY_ANIM_TIME),
            GREF(float, moby + MOBY_ANIM_SPEED)
        );
    }
}

/* A moby put in a pose (rac1.h): its place, rotation and scale, and its animation, blended into a
 * new seq and kept to the pose's place in it by its speed (pose_anim.h). */
void rac1_pose_moby(gaddr moby, const openrac_game_pose* pose) {
    for (unsigned i = 0; i < 3; ++i) {
        GREF(float, moby + MOBY_POS + 4u * i) = pose->position[i];
        GREF(float, moby + MOBY_ROT + 4u * i) = pose->rotation[i];
    }
    if (pose->scale > 0.0f) {
        GREF(float, moby + MOBY_SCALE) = pose->scale;
    }
    pose_action = "run";
    const int frames = seq_frames(moby, pose->seq);
    if (frames == 0) {
        pose_action = "noseq";
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
            /* A new seq: blended into, as the game's mobys change theirs (MobyBlendAnimation),
             * at the blend's own speed. */
            func_00213DE0(moby, in.pose_seq, in.pose_frame, FIGURE_BLEND);
            pose_action = "blend";
            return;
        case POSE_ANIM_BLENDING:
            pose_action = "blending";
            return;
        case POSE_ANIM_SET:
            func_00213D28(moby, in.pose_seq, in.pose_frame);
            GREF(float, moby + MOBY_ANIM_TIME) = in.pose_blending ? 0.0f : in.pose_time;
            pose_action = "snap";
            break;
        default:
            break;
    }
    GREF(float, moby + MOBY_ANIM_SPEED) = speed;
}

static void figure_pose(gaddr moby, const openrac_game_pose* pose) {
    if (rac1_figure_alive(moby)) {
        trace_moby(moby, "pre");
        rac1_pose_moby(moby, pose);
        trace_moby(moby, pose_action);
    }
}

static void figure_remove(gaddr moby) {
    if (!rac1_figure_alive(moby)) {
        return;
    }
    rac1_figure_items_remove(moby);
    GREF(uint32_t, GREF(gaddr, moby + MOBY_PVARS)) = 0;
    func_0020D678(moby);
}

/* A save's {id, size, value} block `index` of the game part, its value; 0 and *ok cleared for a
 * block that is not there as memcard_RestoreInfo reads it. */
static int32_t save_word(const uint8_t* save, unsigned index, int* ok) {
    int32_t block[3];
    memcpy(block, save + SAVE_HEADER + 8u + 12u * index, sizeof block);
    if (block[0] != (int32_t)index || block[1] != 4) {
        *ok = 0;
        return 0;
    }
    return block[2];
}

static int save_info(
    const uint8_t* save,
    uint32_t size,
    int32_t* level,
    int32_t* bolts,
    int32_t* gold_bolts,
    int32_t* seconds
) {
    int32_t parts[2];
    if (save == NULL || size < SAVE_HEADER + 8u + 12u * 4u) {
        return 0;
    }
    memcpy(parts, save, sizeof parts);
    /* The sizes add up to the file's: the game part and twenty level parts. */
    if (parts[0] <= 0 || parts[1] <= 0
        || (uint64_t)SAVE_HEADER + (uint64_t)parts[0] + SAVE_LEVEL_PARTS * (uint64_t)parts[1]
               != size) {
        return 0;
    }
    int ok = 1;
    *level = save_word(save, 0, &ok);
    *bolts = save_word(save, 1, &ok);
    *gold_bolts = save_word(save, 2, &ok);
    *seconds = save_word(save, 3, &ok) / TICKS_A_SECOND;
    return ok && *level >= 0 && *level < LEVELS;
}

static int start(int level, const uint8_t* save, uint32_t size) {
    if (level >= LEVELS || in_level()) {
        return 0;
    }
    if (save == NULL) {
        rac1_start_level(level < 0 ? 0 : level);
        return 1;
    }
    int32_t saved_level, bolts, gold, seconds;
    if (size > SAVE_ROOM || !save_info(save, size, &saved_level, &bolts, &gold, &seconds)) {
        return 0;
    }
    const gaddr room = save_room();
    memcpy(G(room), save, size);
    /* A fresh state under the save (NewGameInit, its level the game's own 0). */
    const int kept = openrac_game_start_level;
    openrac_game_start_level = -1;
    func_00209DC0();
    openrac_game_start_level = kept;
    func_00209CE8(room);
    if (level >= 0) {
        GREF(int, CURRENT_LEVEL) = level;
    }
    func_0022F4A0(0);
    GREF(short, START_GAME) = 1;
    return 1;
}

static int travel(int level) {
    /* -1 is the title, as Quit Game goes there. */
    if (level < -1 || level >= LEVELS || !in_level()) {
        return 0;
    }
    func_0022F4A0(level);
    GREF(int, LEVEL_EXIT) = 1;
    return 1;
}

static uint32_t save_capture(uint8_t* out, uint32_t size) {
    if (!in_level()) {
        return 0;
    }
    const gaddr room = save_room();
    func_0020BA00(room);
    int32_t parts[2];
    memcpy(parts, G(room), sizeof parts);
    const uint64_t total =
        (uint64_t)SAVE_HEADER + (uint64_t)parts[0] + SAVE_LEVEL_PARTS * (uint64_t)parts[1];
    if (parts[0] <= 0 || parts[1] <= 0 || total > SAVE_ROOM) {
        return 0;
    }
    if (out != NULL && size >= total) {
        memcpy(out, G(room), (size_t)total);
    }
    return (uint32_t)total;
}

const openrac_game_play rac1_play = {
    .levels = LEVELS,
    .level_names = rac1_level_names,
    .phase = phase,
    .loads = loads,
    .hero_pose = hero_pose,
    .figure_spawn = figure_spawn,
    .figure_alive = rac1_figure_alive,
    .figure_pose = figure_pose,
    .figure_remove = figure_remove,
    .start = start,
    .travel = travel,
    .save_capture = save_capture,
    .save_folder = RAC1_SAVE_FOLDER,
    .save_file = "save%d.bin",
    .save_slots = 5,
    .save_info = save_info,
    .mobys = rac1_world_mobys,
    .moby_slot = rac1_world_moby_slot,
    .moby_at = rac1_world_moby_at,
    .slot_of = rac1_moby_slot,
    .moby_pose = rac1_pose_moby,
    .moby_kill = rac1_world_moby_kill,
    .moby_spawn = rac1_world_moby_spawn,
    .hero_slot = rac1_world_hero_slot,
    .stand_in = rac1_world_stand_in,
    .post_hit = rac1_world_post_hit,
    .eyes = rac1_world_eyes,
    .bolts = rac1_world_bolts,
    .keep_running = rac1_world_keep_running,
    .hero_held = rac1_hero_held,
    .figure_held = rac1_figure_held,
    .hero_shots = rac1_hero_shots,
    .figure_shot = rac1_figure_shot,
};
