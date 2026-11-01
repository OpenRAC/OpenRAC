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
 * of the body's joints into a pose buffer the glove's moby then draws (its own seq stays at its
 * first key: the glove opens and throws with the hand).
 *
 * A figure's pieces are mobys of the same classes, made here, never run (their own updates read
 * the hero), and put on the figure's joints the same way after the moby loop (UpdateMobys,
 * crash_state.c, has advanced and built the figure's), each tick: rac1_figure_items_tick. A loose
 * piece (a thrown wrench) is where the other game says. A figure's glove is posed from the
 * figure's hand the same way, into a buffer in the game's memory, where the renderer reads a moby's
 * keys (scene/moby_pose.cpp: main RAM only, so not the port's statics): one of the game's sixteen
 * blend slots (0x800 bytes from D_001AAF40, the ones MobyBlendAnimation parks a pose in), taken for
 * the glove's moby with find_or_allocate_id_slot (func_0020DA68) and kept as the game keeps a
 * blending moby's (seq 0xFF, frame the slot: prune_moby_references, func_0020DAB0, frees the
 * slot of a moby that is neither), and given back when the glove goes. With no slot free, the
 * glove keeps its class's own key that tick.
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
 * menu, whose previews and vendor use the buffers), over another figure's piece only for a figure
 * nearer the hero (figure_tracks.h); a hand item that cannot be had falls back to the wrench.
 *
 * So one weapon class besides the hero's. More buffers cannot be had: moby_class_resources takes any
 * address, but a buffer must be in game memory, the heap has no 0x18000 to spare on every level
 * (moby_pool.c: no room for a bigger pool on levels 5, 6 and 15), and only expanded levels have more
 * memory than the disc's.
 *
 * Shots: figure_shots.c.
 *
 * OPENRAC_ITEMS_DEBUG=1 says on stderr what the hero holds, the pieces made, the classes decoded
 * for figures and the figures' shots.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "figure_items.h"
#include "figure_tracks.h"
#include "openrac/host_env.h"
#include "rac1.h"

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
/* The class tables (executable data). */
#define CLASS_RESOURCES OPENRAC_DATA(RAC1_ADDR(0x001B3580u))
#define CLASS_SLOT_BY_ID OPENRAC_DATA(RAC1_ADDR(0x001B3E40u))
#define CLASS_SLOTS 0x800
#define CLASS_SLOT_NONE 0xFFu
#define SWAP_IDS OPENRAC_DATA(RAC1_ADDR(0x001CBE40u))
#define SWAP_COUNT OPENRAC_DATA(RAC1_ADDR(0x00160048u))
#define DECODE_BUFFER OPENRAC_DATA(RAC1_ADDR(0x00160050u))
#define RESOURCE_BUFFERS OPENRAC_DATA(RAC1_ADDR(0x001941C0u))
#define SWAP_BUFFER_SIZE 0x18000u
/* The most classes the swap list has that are looked through. */
#define SWAP_MAX 64

/* The wrench and its class (the shots' are in figure_shots.c). */
#define ITEM_WRENCH 8
#define CLASS_WRENCH 0x47

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
/* The blend slots (anim.c) and the table of the mobys holding them (moby_render_slots). */
#define BLEND_SLOTS OPENRAC_DATA(RAC1_ADDR(0x001AAF40u))
#define BLEND_SLOT_SIZE 0x800u
#define BLEND_OWNERS OPENRAC_DATA(RAC1_ADDR(0x001B2F40u))
#define BLEND_SLOT_COUNT 16
/* A moby's seq while its key A is a blend slot's (the slot in its frame byte). */
#define SEQ_BLEND_SLOT 0xFFu

/* The figures' pieces: which figure, the level load, the piece as asked and its moby. */
#define RECORDS 32

typedef struct {
    gaddr figure;
    uint32_t load;
    openrac_game_held_piece piece;
    gaddr moby;
} Record;

static Record records[RECORDS];

/* The class each decode buffer holds, as noted by the wrapper (-1 not known), for the load. */
static int32_t buffer_class[2] = {-1, -1};
static uint32_t buffer_load = 0xFFFFFFFFu;
/* The tick each was last decoded for a figure, to keep one from changing hands too often. */
static uint32_t buffer_taken[2];
static uint32_t ticks;

int rac1_figure_items_debug(void) {
    static int on = -1;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_ITEMS_DEBUG");
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
    const uint32_t load = rac1_moby_generation();
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

/* The blend slot moby `m` holds, taken for it when it has none; -1 when all are taken. */
static int blend_slot(gaddr m) {
    return func_0020DA68((int)m);
}

/* Moby `m`'s blend slot given back (as prune_moby_references would at the next frame). */
static void release_blend_slot(gaddr m) {
    for (uint32_t i = 0; i < BLEND_SLOT_COUNT; ++i) {
        if (GREF(gaddr, BLEND_OWNERS + 4u * i) == m) {
            GREF(gaddr, BLEND_OWNERS + 4u * i) = 0;
        }
    }
}

/* Removes record `r`'s moby (when still its own) and frees it; `by_mod`: for the mod's
 * figure_remove (noted as a mod's deletion, moby_kill's way). */
static void drop_as(Record* r, int by_mod) {
    if (r->moby != 0 && r->load == rac1_moby_generation() && in_level()
        && alive(r->moby, r->piece.oclass)) {
        release_blend_slot(r->moby);
        if (by_mod) {
            rac1_world_moby_kill(r->moby);
        } else {
            const int entered = enter_level_context();
            func_0020D678(r->moby);
            leave_level_context(entered);
        }
    }
    r->figure = 0;
    r->moby = 0;
}

static void drop(Record* r) {
    drop_as(r, 0);
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

static float distance_to_hero(gaddr moby) {
    const gaddr hero = GREF(gaddr, HERO_MOBY);
    float sum = 0.0f;
    for (unsigned c = 0; c < 3; ++c) {
        const float d = GREF(float, moby + MOBY_POS + 4u * c) - GREF(float, hero + MOBY_POS + 4u * c);
        sum += d * d;
    }
    return sqrtf(sum);
}

/* Whether `figure` may decode its class over the pieces of another figure in buffer `b`: only one
 * nearer the hero, so of the players the buffers cannot all hold the nearest keeps its weapon. Those
 * pieces go, and their figure holds the wrench until it can have its own. */
static int buffer_free_for(gaddr figure, int b) {
    const uint32_t load = rac1_moby_generation();
    const gaddr base = buffer_base(b);
    gaddr holder = 0;
    for (int i = 0; i < RECORDS && holder == 0; ++i) {
        const Record* r = &records[i];
        if (r->figure != 0 && r->moby != 0 && r->load == load
            && GREF(gaddr, r->moby + MOBY_CLASS) == base) {
            holder = r->figure;
        }
    }
    if (holder == 0 || holder == figure) {
        return holder == 0;
    }
    if (!figure_takes_buffer(
            distance_to_hero(figure), distance_to_hero(holder), ticks - buffer_taken[b]
        )) {
        return 0;
    }
    for (int i = 0; i < RECORDS; ++i) {
        Record* r = &records[i];
        if (r->figure != 0 && r->moby != 0 && r->load == load
            && GREF(gaddr, r->moby + MOBY_CLASS) == base) {
            drop(r);
        }
    }
    return 1;
}

/* Whether `oclass` can be drawn now for `figure`, decoding it when it may be (above). */
static int have_class(gaddr figure, int32_t oclass) {
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
    /* Never over the piece of a figure that keeps it. */
    if (!buffer_free_for(figure, b)) {
        return 0;
    }
    buffer_taken[b] = ticks;
    func_00205270(oclass, b);
    if (rac1_figure_items_debug()) {
        fprintf(stderr, "[debug] figure items: class %d decoded into buffer %d\n", oclass, b);
    }
    return swapped_loaded(oclass);
}

/* A piece's moby for `figure`: the figure's look, never run, colliding with nothing; 0 for none. */
static gaddr make_piece(gaddr figure, const openrac_game_held_piece* p) {
    if (!have_class(figure, p->oclass)) {
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
    GREF(uint16_t, m + MOBY_FLAGS) = (uint16_t)((GREF(uint16_t, m + MOBY_FLAGS) | FLAG_NOT_RUN
                                                 | FLAG_KEEP_MATRIX | FLAG_NO_ANIMATION)
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
    const uint32_t load = rac1_moby_generation();
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

static int glove(int32_t item);

/* A piece's animation, as a figure's body is posed (rac1_pose_moby; its place is the tick's). A
 * glove's is its hand's (rac1_figure_items_tick): its own seq is left alone. */
static void animate(gaddr m, const openrac_game_held_piece* p) {
    if (glove(p->item)) {
        uint16_t flags = GREF(uint16_t, m + MOBY_FLAGS);
        flags = (p->flags & HELD_HIDDEN) != 0 ? (uint16_t)(flags | FLAG_HIDDEN)
                                              : (uint16_t)(flags & ~FLAG_HIDDEN);
        GREF(uint16_t, m + MOBY_FLAGS) = flags;
        return;
    }
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

int rac1_figure_class_ready(int32_t oclass) {
    if (oclass <= 0 || oclass >= CLASS_SLOTS
        || GREF(uint8_t, CLASS_SLOT_BY_ID + (uint32_t)oclass) == CLASS_SLOT_NONE) {
        return 0;
    }
    if (swapped(oclass)) {
        buffers_for_load();
        return swapped_loaded(oclass);
    }
    return class_resources(oclass) != 0;
}

gaddr rac1_figure_track_moby(gaddr figure, int32_t oclass) {
    openrac_game_held_piece p;
    memset(&p, 0, sizeof p);
    p.item = -1;
    p.oclass = oclass;
    return rac1_figure_class_ready(oclass) ? make_piece(figure, &p) : 0;
}

/* A moby on no joint: where it is said to be, its matrix built from that. */
static void place_loose(gaddr m) {
    func_L00_002514B8(m);
    func_L00_00251E30(m);
    func_0020EEE8(m);
}

void rac1_figure_track_moby_place(gaddr moby, const float* position) {
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, moby + MOBY_POS + 4u * c) = position[c];
    }
    place_loose(moby);
}

void rac1_figure_track_moby_drop(gaddr moby, int32_t oclass) {
    if (alive(moby, oclass)) {
        const int entered = enter_level_context();
        func_0020D678(moby);
        leave_level_context(entered);
    }
}

void rac1_figure_held(gaddr figure, const openrac_game_held* held) {
    if (!in_level() || !rac1_figure_alive(figure)) {
        return;
    }
    forget_stale();
    const uint32_t load = rac1_moby_generation();
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
                at = free_at;
                if (rac1_figure_items_debug()) {
                    fprintf(
                        stderr,
                        "[debug] figure items: slot %d part %d item %d class %d on\n",
                        p.slot,
                        p.part,
                        p.item,
                        p.oclass
                    );
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
    rac1_figure_shots_remove(figure);
    for (int i = 0; i < RECORDS; ++i) {
        if (records[i].figure == figure) {
            drop_as(&records[i], 1);
        }
    }
}

/* After the moby loop: each figure's pieces put on its joints (update_hero_item_mobys' way). */
void rac1_figure_items_tick(void) {
    if (!in_level() || GREF(int32_t, CONTEXT) != 0) {
        return;
    }
    ++ticks;
    rac1_figure_shots_tick();
    const uint32_t load = rac1_moby_generation();
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
            place_loose(m);
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
        const int slot = is_glove ? blend_slot(m) : -1;
        if (slot >= 0) {
            const gaddr pose = BLEND_SLOTS + BLEND_SLOT_SIZE * (uint32_t)slot;
            func_L00_00209940(GLOVE_JOINTS, pose, GREF(gaddr, m + MOBY_CLASS), 0, (int)r->figure);
            GREF(float, m + MOBY_ANIM_TIME) = 0.0f;
            GREF(uint8_t, m + MOBY_SEQ) = SEQ_BLEND_SLOT;
            GREF(uint8_t, m + MOBY_FRAME) = (uint8_t)slot;
            GREF(gaddr, m + MOBY_CUR_FRAME) = pose;
            GREF(gaddr, m + MOBY_PREV_FRAME) = pose;
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
            p->joint =
                s == 1 && part == 1
                    ? 3
                    : GREF(int32_t, ITEM_DEFS + ITEM_DEF_SIZE * (uint32_t)item + ITEM_DEF_JOINT);
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
    if (rac1_figure_items_debug()) {
        static int32_t told_hand = -2;
        static uint32_t told_load;
        const int32_t hand =
            out->count > 0 && out->pieces[0].slot == 0 ? out->pieces[0].oclass : -1;
        if (hand != told_hand || told_load != rac1_moby_generation()) {
            told_hand = hand;
            told_load = rac1_moby_generation();
            fprintf(
                stderr,
                "[debug] figure items: hero holds %d pieces, hand class %d (item %d, "
                "joint %d):",
                out->count,
                hand,
                out->count > 0 ? out->pieces[0].item : -1,
                out->count > 0 ? out->pieces[0].joint : -1
            );
            for (int k = 0; k < out->count; ++k) {
                fprintf(
                    stderr,
                    " %d.%d:%d",
                    out->pieces[k].slot,
                    out->pieces[k].part,
                    out->pieces[k].oclass
                );
            }
            fprintf(stderr, "\n");
        }
    }
    return 1;
}
