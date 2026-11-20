/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's game state for the window, the debug overlay (F3), crash reports and
 * OPENRAC_STATE_DUMP (game_host.h), as Going Commando's host/tools.c gives it: the levels by the
 * game's numbers, the front end's mode word, a level's own mode word, where the hero is, and the
 * front end's state as JSON. Everything here only reads game memory.
 *
 * The words, PAL's (rac3-decomp's names; IDENTIFIED.tsv):
 * - 0x001D5B94: the boot program's (and frontbin's) mode, 0 the menus, 1 a movie (its loop's
 *   switch, jtbl_0031BF80: 2 to 18 are its other screens).
 * - 0x001D5458: the front end's state in mode 0 (0 the main menu, 10 and 8 on the way to it).
 * - 0x001D545C: level_current, the level whose program is loaded or loading (core data, the same
 *   in every program; request_level_change and level_load read and write it).
 * - 0x001D9D04: level_next in the boot program and frontbin (a level's own copy is $gp-relative
 *   at another place).
 * - A level's mode word: game_mode_push (frontbin 0x003A0EC0, each level's copy) reads and writes
 *   it; 0 play (gameplay_update), 2 a scene, 4 the overlays (pause menus, the quick-select pause:
 *   overlay_update), 5 the vendor, 8 a weapon's upgrade, and others less known. The word before it
 *   is the mode asked for.
 * - 0x001A4C60: the hero's position (the hero code's base 0x001A4BE0 + 0x80).
 */
#include <stdint.h>
#include <stdio.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

#define RAC3_LEVELS 54
#define FRONT_END_MODE 0x001D5B94u
#define FRONT_END_STATE 0x001D5458u
#define LEVEL_CURRENT 0x001D545Cu
#define BOOT_LEVEL_NEXT 0x001D9D04u
#define HERO_POSITION 0x001A4C60u
#define PADS 0x001CD0C0u

/* The levels, by the game's own numbers (rac3-decomp config/levels/levels.tsv); NULL: no program
 * on the disc. */
static const char* const level_names[RAC3_LEVELS] = {
    NULL,
    "Veldin",
    "Florana",
    "Starship Phoenix",
    "Marcadia",
    "Daxx",
    "Starship Phoenix Under Attack",
    "Annihilation Nation",
    "Aquatos",
    "Tyhrranosis",
    "Zeldrin Starport",
    "Obani Moons",
    "Rilgar",
    "Holostar Studios (Ratchet)",
    "Koros",
    NULL,
    "Kerwan",
    "Crash Site",
    "Aridia",
    "Thran Asteroid Belt",
    "Final Boss",
    "Obani Draco",
    "Mylon",
    "Holostar Studios (Clank)",
    "Insomniac Museum",
    NULL,
    "Kerwan Ranger Missions",
    "Aquatos Base",
    "Aquatos Sewers",
    "Tyhrranosis Ranger Missions",
    "Vid-comic (unnamed)",
    "Vid-comic 1",
    "Vid-comic 4",
    "Vid-comic 2",
    "Vid-comic 3",
    "Vid-comic 5",
    "Vid-comic 1 Special Edition",
    NULL,
    NULL,
    "Multiplayer Menu",
    "Bakisi Isles",
    "Hoven Gorge",
    "Outpost X12",
    "Korgon Outpost",
    "Metropolis",
    "Blackwater City",
    "Command Center",
    "Blackwater Docks",
    "Aquatos Sewers (multiplayer)",
    "Marcadia Palace",
    "Bakisi Isles (split screen)",
    "Hoven Gorge (split screen)",
    "Outpost X12 (split screen)",
    "Korgon Outpost (split screen)",
};

/* Each level's mode word, read from its copy of game_mode_push (the level's .lit, whose size
 * moves it); 0 where the level catalogue has no copy (the multiplayer levels). */
static uint32_t level_mode_word(int level) {
    switch (level) {
        case 12:
        case 18:
        case 26:
        case 29:
            return 0x001D66E4u;
        case 19:
            return 0x001D66D4u;
        case 15:
        case 25:
            return 0;
        default:
            return level >= 1 && level <= 36 ? 0x001D6694u : 0;
    }
}

static int loaded_level(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < RAC3_LEVELS && level_names[level] != NULL ? level : -1;
}

/* openrac_game_mode: the loaded level's mode word; none outside a level (the front end's word is
 * openrac_front_end.game_mode). play.c's phase reads it too. */
int rac3_level_mode(uint32_t* mode);

int rac3_level_mode(uint32_t* mode) {
    const int level = loaded_level();
    const uint32_t word = level >= 0 ? level_mode_word(level) : 0;
    if (word == 0) {
        return 0;
    }
    *mode = GREF(uint32_t, word);
    return 1;
}

static int hero_position(float xyz[3]) {
    if (loaded_level() < 0) {
        return 0;
    }
    for (int i = 0; i < 3; ++i) {
        xyz[i] = GREF(float, HERO_POSITION + 4u * (unsigned)i);
    }
    return 1;
}

/* For OPENRAC_STATE_DUMP and the debug overlay: the front end's words, and the level's mode and
 * the hero's place while a level runs. */
static int state_json(char* out, int size) {
    int at = snprintf(
        out,
        (size_t)size,
        "\"front_end_mode\":%d,\"front_end_state\":%d,\"level_current\":%d",
        (int)GREF(int32_t, FRONT_END_MODE),
        (int)GREF(int32_t, FRONT_END_STATE),
        (int)GREF(int32_t, LEVEL_CURRENT)
    );
    const int level = loaded_level();
    if (level < 0 && at < size) {
        at += snprintf(
            out + at, (size_t)(size - at), ",\"level_next\":%d", (int)GREF(int32_t, BOOT_LEVEL_NEXT)
        );
    }
    uint32_t mode = 0;
    if (rac3_level_mode(&mode) && at < size) {
        at += snprintf(out + at, (size_t)(size - at), ",\"level_mode\":%d", (int)mode);
    }
    float xyz[3];
    if (hero_position(xyz) && at < size) {
        at += snprintf(
            out + at,
            (size_t)(size - at),
            ",\"hero_position\":[%.2f,%.2f,%.2f]",
            (double)xyz[0],
            (double)xyz[1],
            (double)xyz[2]
        );
    }
    return at < size ? at : size - 1;
}

static const openrac_game_tools tools = {
    .state_json = state_json,
};

int rac3_start_level(int level); /* program.c */

/* The developer menu's levels (debug_menu.cpp), from the front end or from a level. */
static void front_end_start_level(int level) {
    if (level >= 0 && level < RAC3_LEVELS && level_names[level] != NULL) {
        rac3_start_level(level);
    }
}

static const openrac_front_end front_end = {
    .levels = RAC3_LEVELS,
    .level_names = level_names,
    .start_level = front_end_start_level,
    .start_level_anywhere = 1,
    /* The front end's mode (0 menus, 1 movie). The window reads 6 here as Going Commando's flight
     * between planets: this game's boot program has no mode 6 (its switch's default). */
    .game_mode = FRONT_END_MODE,
    .level = LEVEL_CURRENT,
    /* The first of the eight pad records (pads_update): +0x1A0 the buttons held. */
    .pad = PADS,
};

/* From crt0 (entry.c), not a constructor: the game is a static library, and nothing else names this
 * file, so a constructor here is dropped with it at the link. */
void rac3_install_tools(void) {
    openrac_game_mode = rac3_level_mode;
    openrac_game_hero_position = hero_position;
    openrac_game_front_end = &front_end;
    openrac_game_tools_host = &tools;
}
