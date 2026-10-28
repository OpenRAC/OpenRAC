/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: what a figure (play.c) wears and holds, and the shots it fires (game_host.h,
 * openrac_game_play; interface 1.9): another player's wrench, weapon, boots and pack, drawn on the
 * figure and animated as that player's are, and that player's shots shown in this game, hitting
 * nothing here (the shooter's game tells what they hit).
 *
 * The hero's pieces. Each of the hero's seven item slots (Hero +0x1090, 0x50 bytes: its moby, its
 * second moby, +0x1A thrown, +0x28 the item) has up to two mobys, made by spawn_hero_item_mobies
 * (func_L00_0020F118) from the item table (D_L00_00179BC0, 0x4C bytes an item: the joint at +0x0C)
 * and put on a joint of the hero by update_hero_item_mobys (func_L00_0020FC18): the hero's nine
 * attach matrices (Hero +0xAC0, 0x40 bytes each, the place in the last row) are
 * compute_selected_world_matrices (func_L00_00250928) of his moby over the joint list
 * hero_world_matrix_joints (D_L00_001E7F90), which works for any moby of the hero's class. A
 * piece's moby is not run by the moby loop (flags 6): the hero's update advances its animation,
 * puts it at its joint's place, builds its matrix and copies the joint's rotation rows over it. The
 * gloves (items 10, 17, 20 and 25) are posed from the hand instead: pack_pose (func_L00_00209940)
 * of the body's joints into a pose buffer the glove's moby then draws.
 *
 * A figure's pieces are mobys of the same classes, made here, never run (their own updates read
 * the hero), and put on the figure's joints the same way after the moby loop (UpdateMobys,
 * crash_state.c, has advanced and built the figure's), each tick: rac1_figure_items_tick. A loose
 * piece (a thrown wrench) is where the other game says.
 *
 * Classes. Most of a level's classes stay loaded, but the weapons' and gadgets'
 * (class_resource_ids, D_001CBE40) share two decode buffers of 0x18000 bytes (the fifth of
 * level_resource_buffers, D_001941C0): select_world_object_resource_tables (func_00205270) decodes
 * one into a buffer and points its class's resources there (moby_class_resources, D_001B3580, by
 * resident_class_slot_by_id, D_001B3E40), so a class is loaded only while its buffer still holds
 * it. That function is wrapped
 * (libraries.tsv): it notes which class each buffer holds, and before a buffer is decoded over, the
 * figures' pieces drawn from it go (the hero's own pieces are the game's). A figure's class not
 * loaded is decoded into the buffer the hero's hand item is not in, and only in play (never under a
 * menu, whose previews and vendor use the buffers), never over another figure's piece; a hand item
 * that cannot be had falls back to the wrench.
 *
 * Shots. The hero's Blaster shots are spawn_homing_shot (func_L00_002CC3C0) with the hero's moby
 * as their owner, his bombs launch_bomb (func_L00_002AE278) of a bomb the glove holds; both wrapped
 * and noted for hero_shots. A figure's shot is the same call with the figure as the owner. A moby
 * whose owner is a figure (a shot's pvars +0x20, a bomb's +0x50) is a figure's: while its update
 * runs (rac1_figure_owned of the moby being updated), the collision queries post no hit records
 * (collision.c), its blast sends no radial message (SendRadialMessageToMobys, func_L00_0025BA50,
 * wrapped) and the hits posted are dropped (world.c).
 *
 * OPENRAC_ITEMS_DEBUG=1 says on stderr what the hero holds, the pieces made, the classes decoded
 * for figures and the figures' shots.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rac1.h"

/* The hero (play.c), his item slots, his attach matrices. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_SLOTS (HERO + 0x1090u)
#define SLOT_SIZE 0x50u
#define SLOT_THROWN 0x1Au
#define SLOT_ITEM 0x28u
#define SLOTS 7
/* The item table, the joint list, the gloves' source joints (level data, level 0's addresses). */
#define ITEM_DEFS OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00179BC0u))
#define ITEM_DEF_SIZE 0x4Cu
#define ITEM_DEF_JOINT 0x0Cu
#define ITEMS 0x25
#define JOINT_LIST OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001E7F90u))
#define GLOVE_JOINTS OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0017A6C0u))
#define JOINTS 9
#define JOINT_SIZE 0x40u
/* A level's game mode (play.c: 0 play) and its moby context (world.c: 0 the level's own). */
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define CONTEXT OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F808u))
/* The class tables (executable data). */
#define CLASS_RESOURCES OPENRAC_DATA(RAC1_ADDR(0x001B3580u))
#define CLASS_SLOT_BY_ID OPENRAC_DATA(RAC1_ADDR(0x001B3E40u))
#define SWAP_IDS OPENRAC_DATA(RAC1_ADDR(0x001CBE40u))
#define SWAP_COUNT OPENRAC_DATA(RAC1_ADDR(0x00160048u))
#define DECODE_BUFFER OPENRAC_DATA(RAC1_ADDR(0x00160050u))
#define RESOURCE_BUFFERS OPENRAC_DATA(RAC1_ADDR(0x001941C0u))
#define SWAP_BUFFER_SIZE 0x18000u
/* The most classes the swap list has that are looked through. */
#define SWAP_MAX 64

/* The wrench, the Blaster and the Bomb Glove, their classes and the classes of their shots. */
#define ITEM_WRENCH 8
#define ITEM_BOMB_GLOVE 0x0A
#define ITEM_BLASTER 0x0F
#define CLASS_WRENCH 0x47
#define CLASS_HOMING_SHOT 0x131
#define CLASS_BOMB 0x79
#define SHOT_OWNER 0x20u
#define BOMB_OWNER 0x50u

enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_CLASS = 0x24,
    MOBY_UPDATE_DIST = 0x30,
    MOBY_VISIBLE = 0x31,
    MOBY_DRAW_DIST = 0x32,
    MOBY_FLAGS = 0x34,
    MOBY_COLOUR = 0x38,
    MOBY_ROT = 0x40,
    MOBY_FRAME = 0x50,
    MOBY_ANIM_TIME = 0x54,
    MOBY_CUR_FRAME = 0x68,
    MOBY_PREV_FRAME = 0x6C,
    MOBY_UNK73 = 0x73,
    MOBY_UPDATE = 0x74,
    MOBY_PVARS = 0x78,
    MOBY_LIGHT = 0x80,
    MOBY_COLLISION = 0x94,
    MOBY_COLLISION_OFF = 0x98,
    MOBY_OCLASS = 0xA6,
    MOBY_MATRIX = 0xC0
};

/* Moby flags: hidden, not run by the moby loop, matrix not rebuilt, animation not advanced, in the
 * target list. */
#define FLAG_HIDDEN 0x1u
#define FLAG_NOT_RUN 0x2u
#define FLAG_KEEP_MATRIX 0x4u
#define FLAG_NO_ANIMATION 0x40u
#define FLAG_TARGET 0x1000u

/* openrac_held_piece.flags (openrac_mod.h). */
#define HELD_HIDDEN 0x1u
#define HELD_BLENDING 0x2u
#define HELD_LOOSE 0x4u

/* How far away a piece is drawn (play.c's figure's). */
#define PIECE_DRAW_DIST 400
/* A glove's pose buffer (pack_pose: a header and three packed rows a joint). */
#define POSE_BUFFER 0x400u

/* The figures' pieces: which figure, the level load, the piece as asked and its moby. */
#define RECORDS 32
typedef struct {
    gaddr figure;
    uint32_t load;
    openrac_game_held_piece piece;
    gaddr moby;
    gaddr pose_buffer; /* a glove's, made once and kept */
} Record;
static Record records[RECORDS];
static gaddr pose_buffers[RECORDS];

/* The class each decode buffer holds, as noted by the wrapper (-1 not known), for the load. */
static int32_t buffer_class[2] = {-1, -1};
static uint32_t buffer_load = 0xFFFFFFFFu;

/* The shots the hero fired since hero_shots last took them. */
static openrac_game_shot shots[16];
static int shot_count;
/* Set while a figure's shot is made: not the hero's. */
static int firing_for_figure;

static int debug_items(void) {
    static int on = -1;
    if (on < 0) {
        const char* v = getenv("OPENRAC_ITEMS_DEBUG");
        on = v != NULL && v[0] != '\0' && v[0] != '0';
    }
    return on;
}

static int in_level(void) {
    return rac1_level_running();
}

static int alive(gaddr moby, int32_t oclass) {
    return moby != 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80
           && GREF(int16_t, moby + MOBY_OCLASS) == oclass;
}

static void buffers_for_load(void) {
    const uint32_t load = rac1_program_loads();
    if (load != buffer_load) {
        buffer_load = load;
        buffer_class[0] = buffer_class[1] = -1;
    }
}

static gaddr buffer_base(int b) {
    return GREF(gaddr, RESOURCE_BUFFERS + 4u * 4u) + (gaddr)b * SWAP_BUFFER_SIZE;
}

/* The decode buffer resources `res` are in, -1 for none. */
static int buffer_of(gaddr res) {
    for (int b = 0; b < 2; ++b) {
        if (res != 0 && res == buffer_base(b)) {
            return b;
        }
    }
    return -1;
}

/* Whether `oclass` is one of the classes that share the decode buffers. */
static int swapped(int32_t oclass) {
    const int32_t count = GREF(int32_t, SWAP_COUNT);
    for (int32_t i = 0; i < count && i < SWAP_MAX; ++i) {
        if (GREF(int32_t, SWAP_IDS + 4u * (uint32_t)i) == oclass) {
            return 1;
        }
    }
    return 0;
}

static gaddr class_resources(int32_t oclass) {
    if (oclass < 0 || oclass >= 0x1000) {
        return 0;
    }
    const uint8_t slot = GREF(uint8_t, CLASS_SLOT_BY_ID + (uint32_t)oclass);
    return GREF(gaddr, CLASS_RESOURCES + 4u * slot);
}

/* The decode buffer the hero's hand item is drawn from, -1 for none. */
static int hero_buffer(void) {
    const gaddr hand = GREF(gaddr, HERO_SLOTS);
    return hand != 0 ? buffer_of(GREF(gaddr, hand + MOBY_CLASS)) : -1;
}

/* Whether a swapped class is in its buffer now. */
static int swapped_loaded(int32_t oclass) {
    const int b = buffer_of(class_resources(oclass));
    if (b < 0) {
        return 0;
    }
    if (buffer_class[b] >= 0) {
        return buffer_class[b] == oclass;
    }
    /* Not noted since the load: loaded when a moby of the hero's draws from it. */
    for (int s = 0; s < SLOTS; ++s) {
        for (int part = 0; part < 2; ++part) {
            const gaddr m = GREF(gaddr, HERO_SLOTS + SLOT_SIZE * (uint32_t)s + 4u * (uint32_t)part);
            if (alive(m, oclass) && GREF(gaddr, m + MOBY_CLASS) == class_resources(oclass)) {
                return 1;
            }
        }
    }
    return 0;
}

/* CreateMoby and DeleteMoby work on the current moby context: the level's is put up for them
 * while the pause menu's is (and put back after), as world.c does. */
static int enter_level_context(void) {
    if (GREF(int32_t, CONTEXT) == 0) {
        return 0;
    }
    func_L00_00235FF8(0);
    return 1;
}

static void leave_level_context(int entered) {
    if (entered) {
        func_L00_00235FF8(1);
    }
}

/* Removes record `r`'s moby (when still its own) and frees it. */
static void drop(Record* r) {
    if (r->moby != 0 && r->load == rac1_program_loads() && in_level()
        && alive(r->moby, r->piece.oclass)) {
        const int entered = enter_level_context();
        func_0020D678(r->moby);
        leave_level_context(entered);
    }
    r->figure = 0;
    r->moby = 0;
}

/* select_world_object_resource_tables: the figures' pieces drawn from the buffer it decodes over
 * go first; the class it holds after is noted. */
void func_00205270(int class_id, int buffer_index) {
    buffers_for_load();
    const int32_t active = GREF(int32_t, OPENRAC_DATA(RAC1_ADDR(0x0016004Cu)));
    const int same = active >= 0 && GREF(int32_t, SWAP_IDS + 4u * (uint32_t)active) == class_id;
    if (!same) {
        const int b = buffer_index == -1 ? GREF(int32_t, DECODE_BUFFER) == 0 : buffer_index;
        const gaddr base = b == 0 || b == 1 ? buffer_base(b) : 0;
        for (int i = 0; i < RECORDS; ++i) {
            Record* r = &records[i];
            if (r->figure != 0 && r->moby != 0 && base != 0
                && GREF(gaddr, r->moby + MOBY_CLASS) == base) {
                drop(r);
            }
        }
    }
    func_00205270__game(class_id, buffer_index);
    const int32_t now = GREF(int32_t, DECODE_BUFFER);
    if (now == 0 || now == 1) {
        buffer_class[now] = class_id;
    }
}

/* Whether `oclass` can be drawn now, decoding it when it may be (above). */
static int have_class(int32_t oclass) {
    if (!swapped(oclass)) {
        return 1; /* CreateMoby tells (no resources: no class) */
    }
    buffers_for_load();
    if (swapped_loaded(oclass)) {
        return 1;
    }
    /* Only in play, in the level's own mobys, with the hero's hand item made. */
    const int hb = hero_buffer();
    if (GREF(int32_t, LEVEL_MODE) != 0 || GREF(int32_t, CONTEXT) != 0 || hb < 0) {
        return 0;
    }
    const int b = hb == 0 ? 1 : 0;
    const gaddr base = buffer_base(b);
    /* Never over another figure's piece. */
    for (int i = 0; i < RECORDS; ++i) {
        const Record* r = &records[i];
        if (r->figure != 0 && r->moby != 0 && r->load == rac1_program_loads()
            && GREF(gaddr, r->moby + MOBY_CLASS) == base) {
            return 0;
        }
    }
    func_00205270(oclass, b);
    if (debug_items()) {
        fprintf(stderr, "[debug] figure items: class %d decoded into buffer %d\n", oclass, b);
    }
    return swapped_loaded(oclass);
}

/* A piece's moby for `figure`: the figure's look, never run, colliding with nothing; 0 for none. */
static gaddr make_piece(gaddr figure, const openrac_game_held_piece* p) {
    if (!have_class(p->oclass)) {
        return 0;
    }
    const int entered = enter_level_context();
    const gaddr m = func_0020D348(p->oclass);
    /* No room, or a class the level has not (InitMobyInstance leaves it undrawn). */
    if (m != 0 && GREF(gaddr, m + MOBY_CLASS) == 0) {
        func_0020D678(m);
        leave_level_context(entered);
        return 0;
    }
    leave_level_context(entered);
    if (m == 0) {
        return 0;
    }
    GREF(gaddr, m + MOBY_UPDATE) = 0;
    GREF(uint32_t, m + MOBY_COLLISION) = 0;
    GREF(int32_t, m + MOBY_COLLISION_OFF) = -1;
    GREF(uint16_t, m + MOBY_FLAGS) =
        (uint16_t)((GREF(uint16_t, m + MOBY_FLAGS) | FLAG_NOT_RUN | FLAG_KEEP_MATRIX
                    | FLAG_NO_ANIMATION)
                   & ~FLAG_TARGET);
    GREF(uint8_t, m + MOBY_UPDATE_DIST) = 0xFF;
    GREF(uint8_t, m + MOBY_VISIBLE) = 1;
    GREF(int16_t, m + MOBY_DRAW_DIST) = PIECE_DRAW_DIST;
    if (GREF(uint8_t, GREF(gaddr, m + MOBY_CLASS) + 6u) != 0) {
        GREF(uint8_t, m + MOBY_UNK73) = 0x18;
    }
    memcpy(G(m + MOBY_COLOUR), G(figure + MOBY_COLOUR), 8);
    GREF(uint32_t, m + MOBY_LIGHT) = GREF(uint32_t, figure + MOBY_LIGHT);
    memcpy(G(m + MOBY_POS), G(figure + MOBY_POS), 16);
    memcpy(G(m + MOBY_ROT), G(figure + MOBY_ROT), 16);
    if (p->item == ITEM_WRENCH) {
        func_00213DE0(m, 1, 0, 1);
    }
    return m;
}

static int glove(int32_t item) {
    return item == 0x0A || item == 0x11 || item == 0x14 || item == 0x19;
}

static int same_piece(const openrac_game_held_piece* a, const openrac_game_held_piece* b) {
    return a->slot == b->slot && a->part == b->part && a->oclass == b->oclass;
}

/* The wrench in the hand, for a hand item that cannot be had. */
static openrac_game_held_piece wrench_for(const openrac_game_held_piece* p) {
    openrac_game_held_piece w = *p;
    w.item = ITEM_WRENCH;
    w.oclass = CLASS_WRENCH;
    w.joint = GREF(int32_t, ITEM_DEFS + ITEM_DEF_SIZE * ITEM_WRENCH + ITEM_DEF_JOINT);
    w.seq = 1;
    w.frame = w.next_frame = 0;
    w.anim_time = 0.0f;
    w.flags &= HELD_HIDDEN;
    return w;
}

/* The records of a figure gone, or of an earlier load, are let go (their mobys went with the
 * level, or with the figure). */
static void forget_stale(void) {
    const uint32_t load = rac1_program_loads();
    for (int i = 0; i < RECORDS; ++i) {
        Record* r = &records[i];
        if (r->figure != 0 && (r->load != load || !rac1_figure_alive(r->figure))) {
            if (r->load == load) {
                drop(r);
            }
            r->figure = 0;
            r->moby = 0;
        }
    }
}

/* A piece's animation, as a figure's body is posed (rac1_pose_moby; its place is the tick's). */
static void animate(gaddr m, const openrac_game_held_piece* p) {
    openrac_game_pose pose;
    memset(&pose, 0, sizeof pose);
    for (unsigned i = 0; i < 3; ++i) {
        pose.position[i] = GREF(float, m + MOBY_POS + 4u * i);
        pose.rotation[i] = GREF(float, m + MOBY_ROT + 4u * i);
    }
    pose.seq = p->seq;
    pose.frame = p->frame;
    pose.next_frame = p->next_frame;
    pose.anim_time = p->anim_time;
    pose.anim_speed = p->anim_speed;
    pose.flags = (p->flags & HELD_BLENDING) != 0 ? 1u : 0u;
    rac1_pose_moby(m, &pose);
    uint16_t flags = GREF(uint16_t, m + MOBY_FLAGS);
    flags = (p->flags & HELD_HIDDEN) != 0 ? (uint16_t)(flags | FLAG_HIDDEN)
                                           : (uint16_t)(flags & ~FLAG_HIDDEN);
    GREF(uint16_t, m + MOBY_FLAGS) = flags;
}

void rac1_figure_held(gaddr figure, const openrac_game_held* held) {
    if (!in_level() || !rac1_figure_alive(figure)) {
        return;
    }
    forget_stale();
    const uint32_t load = rac1_program_loads();
    int wanted[RECORDS];
    memset(wanted, 0, sizeof wanted);
    for (int k = 0; k < held->count && k < OPENRAC_GAME_HELD_MAX; ++k) {
        openrac_game_held_piece p = held->pieces[k];
        if (p.joint < 0 || p.joint >= JOINTS || p.oclass < 0) {
            continue;
        }
        int at = -1;
        for (int i = 0; i < RECORDS && at < 0; ++i) {
            if (records[i].figure == figure && same_piece(&records[i].piece, &p)) {
                at = i;
            }
        }
        if (at >= 0 && !alive(records[at].moby, records[at].piece.oclass)) {
            records[at].figure = 0;
            records[at].moby = 0;
            at = -1;
        }
        if (at < 0) {
            int free_at = -1;
            for (int i = 0; i < RECORDS && free_at < 0; ++i) {
                if (records[i].figure == 0) {
                    free_at = i;
                }
            }
            if (free_at < 0) {
                continue;
            }
            gaddr m = make_piece(figure, &p);
            if (m == 0 && p.slot == 0 && p.oclass != CLASS_WRENCH) {
                p = wrench_for(&p);
                for (int i = 0; i < RECORDS && at < 0; ++i) {
                    if (records[i].figure == figure && same_piece(&records[i].piece, &p)
                        && alive(records[i].moby, p.oclass)) {
                        at = i;
                    }
                }
                if (at < 0) {
                    m = make_piece(figure, &p);
                }
            }
            if (at < 0) {
                if (m == 0) {
                    continue;
                }
                Record* r = &records[free_at];
                r->figure = figure;
                r->load = load;
                r->moby = m;
                if (pose_buffers[free_at] == 0) {
                    pose_buffers[free_at] = openrac_guest_static(POSE_BUFFER);
                }
                r->pose_buffer = pose_buffers[free_at];
                at = free_at;
                if (debug_items()) {
                    fprintf(stderr, "[debug] figure items: slot %d part %d item %d class %d on\n",
                            p.slot, p.part, p.item, p.oclass);
                }
            } else {
                /* The wrench kept for the hand's item: its own pose (still). */
                p = records[at].piece;
            }
        }
        Record* r = &records[at];
        r->piece = p;
        animate(r->moby, &r->piece);
        wanted[at] = 1;
    }
    /* What was left out comes off. */
    for (int i = 0; i < RECORDS; ++i) {
        if (records[i].figure == figure && !wanted[i]) {
            drop(&records[i]);
        }
    }
}

void rac1_figure_items_remove(gaddr figure) {
    for (int i = 0; i < RECORDS; ++i) {
        if (records[i].figure == figure) {
            drop(&records[i]);
        }
    }
}

/* After the moby loop: each figure's pieces put on its joints (update_hero_item_mobys' way). */
void rac1_figure_items_tick(void) {
    if (!in_level() || GREF(int32_t, CONTEXT) != 0) {
        return;
    }
    const uint32_t load = rac1_program_loads();
    GFRAME(joints, JOINTS * JOINT_SIZE);
    gaddr figure_done = 0;
    for (int i = 0; i < RECORDS; ++i) {
        Record* r = &records[i];
        if (r->figure == 0 || r->load != load || !alive(r->moby, r->piece.oclass)
            || !rac1_figure_alive(r->figure)) {
            continue;
        }
        const gaddr m = r->moby;
        if ((r->piece.flags & HELD_LOOSE) != 0) {
            for (unsigned c = 0; c < 3; ++c) {
                GREF(float, m + MOBY_POS + 4u * c) = r->piece.position[c];
                GREF(float, m + MOBY_ROT + 4u * c) = r->piece.rotation[c];
            }
            func_L00_002514B8(m);
            func_L00_00251E30(m);
            func_0020EEE8(m);
            continue;
        }
        if (r->figure != figure_done) {
            func_L00_00250928(r->figure, JOINTS, JOINT_LIST, joints);
            figure_done = r->figure;
        }
        const gaddr joint = joints + JOINT_SIZE * (uint32_t)r->piece.joint;
        const int is_glove = glove(r->piece.item);
        if (!is_glove) {
            func_L00_002514B8(m);
        }
        memcpy(G(m + MOBY_POS), G(joint + 0x30u), 16);
        func_L00_00251E30(m);
        func_001FA480(m + MOBY_MATRIX, joint);
        if (!is_glove) {
            func_00214F78(m + MOBY_MATRIX);
        }
        func_0020EEE8(m);
        if (is_glove) {
            func_L00_00209940(GLOVE_JOINTS, r->pose_buffer, GREF(gaddr, m + MOBY_CLASS), 0,
                              (int)r->figure);
            GREF(float, m + MOBY_ANIM_TIME) = 0.0f;
            GREF(uint8_t, m + MOBY_FRAME) = 0;
            GREF(gaddr, m + MOBY_CUR_FRAME) = r->pose_buffer;
            GREF(gaddr, m + MOBY_PREV_FRAME) = r->pose_buffer;
        }
        GREF(uint16_t, m + MOBY_FLAGS) =
            (uint16_t)(GREF(uint16_t, m + MOBY_FLAGS) | FLAG_NOT_RUN | FLAG_KEEP_MATRIX);
    }
}

/* What the hero wears and holds: his slots' mobys, the hand first. */
int rac1_hero_held(openrac_game_held* out) {
    memset(out, 0, sizeof *out);
    if (!in_level() || GREF(gaddr, HERO_MOBY) == 0) {
        return 0;
    }
    /* The hand, the back, the boots, the head, then the rest. */
    static const int order[SLOTS] = {0, 3, 1, 2, 4, 5, 6};
    for (int o = 0; o < SLOTS; ++o) {
        const int s = order[o];
        const gaddr slot = HERO_SLOTS + SLOT_SIZE * (uint32_t)s;
        for (int part = 0; part < 2 && out->count < OPENRAC_GAME_HELD_MAX; ++part) {
            const gaddr m = GREF(gaddr, slot + 4u * (uint32_t)part);
            if (m == 0 || GREF(uint8_t, m + MOBY_STATE) >= 0x80) {
                continue;
            }
            const int32_t item = GREF(int32_t, slot + SLOT_ITEM);
            if (item < 0 || item >= ITEMS) {
                continue;
            }
            openrac_game_held_piece* p = &out->pieces[out->count++];
            p->slot = s;
            p->part = part;
            p->item = item;
            p->oclass = GREF(int16_t, m + MOBY_OCLASS);
            p->joint = s == 1 && part == 1
                           ? 3
                           : GREF(int32_t, ITEM_DEFS + ITEM_DEF_SIZE * (uint32_t)item
                                               + ITEM_DEF_JOINT);
            if ((GREF(uint16_t, m + MOBY_FLAGS) & FLAG_HIDDEN) != 0) {
                p->flags |= HELD_HIDDEN;
            }
            if (GREF(uint8_t, slot + SLOT_THROWN) != 0) {
                p->flags |= HELD_LOOSE;
            }
            for (unsigned c = 0; c < 3; ++c) {
                p->position[c] = GREF(float, m + MOBY_POS + 4u * c);
                p->rotation[c] = GREF(float, m + MOBY_ROT + 4u * c);
            }
            /* The animation as hero_pose reads the body's (play.c). */
            const uint8_t seq = GREF(uint8_t, m + 0x52), target = GREF(uint8_t, m + 0x53);
            p->seq = target;
            p->next_frame = GREF(uint8_t, m + 0x51);
            if (seq != target) {
                p->frame = p->next_frame;
                p->anim_time = 0.0f;
                p->flags |= HELD_BLENDING;
            } else {
                p->frame = GREF(uint8_t, m + MOBY_FRAME);
                p->anim_time = GREF(float, m + MOBY_ANIM_TIME);
            }
            p->anim_speed = GREF(float, m + 0x58);
        }
    }
    if (debug_items()) {
        static int32_t told_hand = -2;
        static uint32_t told_load;
        const int32_t hand =
            out->count > 0 && out->pieces[0].slot == 0 ? out->pieces[0].oclass : -1;
        if (hand != told_hand || told_load != rac1_program_loads()) {
            told_hand = hand;
            told_load = rac1_program_loads();
            fprintf(stderr,
                    "[debug] figure items: hero holds %d pieces, hand class %d (item %d, "
                    "joint %d):",
                    out->count, hand, out->count > 0 ? out->pieces[0].item : -1,
                    out->count > 0 ? out->pieces[0].joint : -1);
            for (int k = 0; k < out->count; ++k) {
                fprintf(stderr, " %d.%d:%d", out->pieces[k].slot, out->pieces[k].part,
                        out->pieces[k].oclass);
            }
            fprintf(stderr, "\n");
        }
    }
    return 1;
}

/* ---- Shots ---- */

static void note_shot(const openrac_game_shot* s) {
    const int max = (int)(sizeof shots / sizeof shots[0]);
    if (shot_count == max) {
        memmove(&shots[0], &shots[1], sizeof shots[0] * (size_t)(max - 1));
        --shot_count;
    }
    shots[shot_count++] = *s;
}

int rac1_hero_shots(openrac_game_shot* out, int max) {
    const int n = shot_count < max ? shot_count : max;
    memcpy(out, shots, sizeof shots[0] * (size_t)n);
    memmove(&shots[0], &shots[n], sizeof shots[0] * (size_t)(shot_count - n));
    shot_count -= n;
    return n;
}

/* spawn_homing_shot (the Blaster's): the hero's noted. */
gaddr func_L00_002CC3C0(
    gaddr owner, gaddr start, gaddr target, float yaw, float pitch, float yaw0, float pitch0
) {
    if (!firing_for_figure && owner != 0 && owner == GREF(gaddr, HERO_MOBY)) {
        openrac_game_shot s;
        memset(&s, 0, sizeof s);
        s.item = ITEM_BLASTER;
        for (unsigned c = 0; c < 3; ++c) {
            s.position[c] = GREF(float, start + 4u * c);
        }
        s.aim[0] = yaw;
        s.aim[1] = pitch;
        s.aim[2] = yaw0;
        s.aim[3] = pitch0;
        note_shot(&s);
    }
    return func_L00_002CC3C0__game(owner, start, target, yaw, pitch, yaw0, pitch0);
}

/* launch_bomb: a bomb the hero's glove throws noted, with its velocity as launched. */
void func_L00_002AE278(gaddr pos, gaddr bomb, gaddr velocity) {
    func_L00_002AE278__game(pos, bomb, velocity);
    if (firing_for_figure || bomb == 0) {
        return;
    }
    const gaddr pvars = GREF(gaddr, bomb + MOBY_PVARS);
    const gaddr hand = GREF(gaddr, HERO_SLOTS);
    if (pvars == 0 || hand == 0 || GREF(gaddr, pvars + BOMB_OWNER) != hand) {
        return;
    }
    openrac_game_shot s;
    memset(&s, 0, sizeof s);
    s.item = ITEM_BOMB_GLOVE;
    for (unsigned c = 0; c < 3; ++c) {
        s.position[c] = GREF(float, bomb + MOBY_POS + 4u * c);
        s.velocity[c] = GREF(float, pvars + 4u * c);
    }
    note_shot(&s);
}

int rac1_figure_shot(gaddr figure, const openrac_game_shot* shot) {
    if (!in_level() || !rac1_figure_alive(figure) || GREF(int32_t, CONTEXT) != 0) {
        return 0;
    }
    GFRAME(v, 32);
    for (unsigned c = 0; c < 4; ++c) {
        GREF(float, v + 4u * c) = c < 3 ? shot->position[c] : 1.0f;
        GREF(float, v + 0x10u + 4u * c) = c < 3 ? shot->velocity[c] : 0.0f;
    }
    int made = 0;
    firing_for_figure = 1;
    switch (shot->item) {
        case ITEM_BLASTER:
            made = func_L00_002CC3C0(figure, v, 0, shot->aim[0], shot->aim[1], shot->aim[2],
                                     shot->aim[3])
                   != 0;
            break;
        case ITEM_BOMB_GLOVE: {
            const gaddr bomb = func_L00_002AE110((int)figure, v);
            if (bomb != 0) {
                func_L00_002AE278(v, bomb, v + 0x10u);
                /* As the shooter's game launched it (launch_bomb adds this hero's motion). */
                const gaddr pvars = GREF(gaddr, bomb + MOBY_PVARS);
                if (pvars != 0) {
                    for (unsigned c = 0; c < 3; ++c) {
                        GREF(float, pvars + 4u * c) = shot->velocity[c];
                    }
                }
                made = 1;
            }
            break;
        }
        default:
            break;
    }
    firing_for_figure = 0;
    if (debug_items()) {
        fprintf(stderr, "[debug] figure items: shot of item %d %s\n", shot->item,
                made ? "made" : "refused");
    }
    return made;
}

/* A figure's moby's hit or blast not let through, said with OPENRAC_ITEMS_DEBUG. */
void rac1_figure_hit_dropped(const char* what) {
    if (debug_items()) {
        fprintf(stderr, "[debug] figure items: a figure's %s dropped\n", what);
    }
}

/* Whether `moby` is a figure's (its owner a figure): what it does hits nothing here. */
int rac1_figure_owned(gaddr moby) {
    if (moby == 0 || !in_level()) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars == 0) {
        return 0;
    }
    switch (GREF(int16_t, moby + MOBY_OCLASS)) {
        case CLASS_HOMING_SHOT:
            return rac1_figure_alive(GREF(gaddr, pvars + SHOT_OWNER));
        case CLASS_BOMB:
            return rac1_figure_alive(GREF(gaddr, pvars + BOMB_OWNER));
        default:
            return 0;
    }
}

/* SendRadialMessageToMobys: a figure's blast sends none. */
void func_L00_0025BA50(
    gaddr source,
    gaddr centre,
    gaddr list,
    int count,
    gaddr template_,
    int mask,
    int a6,
    int a7,
    float a8,
    float a9,
    float a10
) {
    if (rac1_figure_owned(rac1_updating_moby()) || rac1_figure_owned(source)) {
        rac1_figure_hit_dropped("blast");
        return;
    }
    func_L00_0025BA50__game(source, centre, list, count, template_, mask, a6, a7, a8, a9, a10);
}
