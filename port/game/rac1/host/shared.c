/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the progress a party shares beyond the items, gold bolts and planets (interface
 * 1.12; the multiplayer mod's shared.hpp). Every table here is one of the save's (memcard_PrepData's
 * tag tables, mc_gamedata and mc_leveldata): the same ids, sizes and order on both discs, a byte
 * each, so a section read on one disc means the same on the other.
 *
 * Gold weapons: gold_weapon_purchased (save tag 0x1E), by item id. The vendor's case
 * (gold_weapon_case_update) sets one byte, and everything else reads it where it is needed: the
 * weapons' strength, and the gold texture when a weapon's class is next loaded (a weapon in hand
 * turns gold when it is next taken out). No other state goes with it.
 *
 * Skill points: gSkillPoints (tag 8, item_unlocked in the NTSC decomp), one byte each, 30 used. An
 * award is the byte, a sound and a banner; the byte alone is what counts (count_skill_points).
 *
 * The levels' flags: saved_mission_state (tag 0xBBC), 16 bytes for each of 20 levels, 0 or 0xFF:
 * the only writer is SetMissionDone (jet_clear_persistent), which sets 0xFF, and a moby's `mission`
 * names the byte. At a level's load the game copies its row to mission_state, which nothing writes
 * after that: mission_state[m] != 0xFF && saved[m] == 0xFF means "done since this level loaded".
 * That copy is left alone here (it is also at a different address in some levels' programs). Mobys
 * read the row where it is, in their first state or each tick, to open a door or stand down; no
 * reader starts a scene because a byte was set.
 *
 * Special items: gSpecialItems (tag 5), a byte each, read by the conditions of NPCs' talk and the
 * pause menu. The game clears one when a talk spends it, which merging cannot undo: the mod does
 * not give a game back what that game once had.
 *
 * All of it is written only upward and in place: no sound, no banner, nothing on the memory card.
 */
#include <string.h>

#include "rac1.h"

#define GOLD_WEAPONS OPENRAC_DATA(RAC1_ADDR(0x0013E620u))
#define SKILL_POINTS OPENRAC_DATA(RAC1_ADDR(0x0013D510u))
#define SPECIAL_ITEMS OPENRAC_DATA(RAC1_ADDR(0x0013D490u))
#define LEVEL_FLAGS OPENRAC_DATA(RAC1_ADDR(0x0014C150u))

enum {
    GOLD_WEAPON_BYTES = 40,
    SKILL_POINT_BYTES = 32,
    SPECIAL_ITEM_BYTES = 128,
    LEVELS = 20,
    LEVEL_FLAG_BYTES = 16,
    DONE = 0xFF,
};

/* A section's place and size; 0 for a tag this game does not know. */
static uint32_t section(int tag, gaddr* at) {
    switch (tag) {
    case OPENRAC_GAME_SHARED_GOLD_WEAPONS:
        *at = GOLD_WEAPONS;
        return GOLD_WEAPON_BYTES;
    case OPENRAC_GAME_SHARED_SKILL_POINTS:
        *at = SKILL_POINTS;
        return SKILL_POINT_BYTES;
    case OPENRAC_GAME_SHARED_LEVEL_FLAGS:
        *at = LEVEL_FLAGS;
        return LEVELS * LEVEL_FLAG_BYTES;
    case OPENRAC_GAME_SHARED_SPECIAL_ITEMS:
        *at = SPECIAL_ITEMS;
        return SPECIAL_ITEM_BYTES;
    default:
        return 0;
    }
}

/* What was not set and `from` has, set: `value` for a flag of the levels', 1 for the others'. */
static void merge(uint8_t* to, const uint8_t* from, uint32_t size, uint8_t value) {
    for (uint32_t i = 0; i < size; ++i) {
        if (from[i] != 0 && to[i] == 0) {
            to[i] = value;
        }
    }
}

int rac1_shared_read(int tag, void* out, uint32_t size) {
    gaddr at = 0;
    const uint32_t bytes = section(tag, &at);
    if (bytes == 0 || size < bytes || !rac1_level_running()) {
        return -1;
    }
    memcpy(out, G(at), bytes);
    return (int)bytes;
}

int rac1_shared_give(int tag, const void* data, uint32_t size) {
    gaddr at = 0;
    const uint32_t bytes = section(tag, &at);
    /* A size that is not the section's is another build's or a bad one: nothing is guessed. */
    if (bytes == 0 || size != bytes || !rac1_level_running()) {
        return 0;
    }
    merge(G(at), data, bytes, tag == OPENRAC_GAME_SHARED_LEVEL_FLAGS ? DONE : 1);
    return 1;
}

int rac1_mission_state_read(int level, uint8_t* out) {
    if (level < 0 || level >= LEVELS || !rac1_level_running()) {
        return 0;
    }
    memcpy(out, (uint8_t*)G(LEVEL_FLAGS) + level * LEVEL_FLAG_BYTES, LEVEL_FLAG_BYTES);
    return 1;
}

int rac1_mission_state_write(int level, const uint8_t* in) {
    if (level < 0 || level >= LEVELS || !rac1_level_running()) {
        return 0;
    }
    merge((uint8_t*)G(LEVEL_FLAGS) + level * LEVEL_FLAG_BYTES, in, LEVEL_FLAG_BYTES, DONE);
    return 1;
}
