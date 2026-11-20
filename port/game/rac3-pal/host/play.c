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
 * No level loads yet, so the phase is always the front end. */
#include <stdint.h>
#include <stdio.h>

#include "openrac/game_host.h"
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

static int phase(int32_t* level) {
    if (level != NULL) {
        *level = -1;
    }
    return OPENRAC_GAME_PHASE_FRONT_END;
}

static uint32_t loads(void) {
    return 0;
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
};

__attribute__((constructor)) static void install_play(void) {
    /* SCES_524.56 -> BESCES-52456RATCHET (the folder the card trace shows the game make). */
    snprintf(save_folder, sizeof save_folder, "%s", "BESCES-52456RATCHET");
    snprintf(world_family, sizeof world_family, "%s", "rac3-pal-1");
    openrac_game_play_host = &play;
}
