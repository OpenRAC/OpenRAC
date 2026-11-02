/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's developer tools (docs/port/DEBUGGING.md, Going anywhere): a level started from
 * the command line and from the developer menu, what the hero holds, his health and his place, and
 * what the crash report, the trace and the state dump say of the game. The level is started the way
 * the game leaves one: RequestLevelExit (0x00288CB0) with the level, which its loop acts on at the
 * end of the tick, from the title as from another level (main, 0x00131D98, runs each program's loop
 * in turn and installs what the transition, 0x002EC408, loaded). A new game's save (NewGameInit,
 * 0x00298E08) goes first when the title is what is left, as the menu's New Game does. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "rac2_addresses.h"

void func_00288CB0(int level, int fade);
void func_00298E08(void);

/* The levels, by the game's own numbers (games/rac2/ntsc/docs/LEVELS.md): 21 and 27 to 29 have no
 * program on the disc. */
#define LEVELS 31
static const char* const level_names[LEVELS] = {
    "Aranos (tutorial)",
    "Oozla",
    "Maktar Nebula",
    "Endako",
    "Barlow",
    "Feltzin System",
    "Notak",
    "Siberius",
    "Tabora",
    "Dobbo",
    "Hrugis Cloud",
    "Joba",
    "Todano",
    "Boldan",
    "Aranos Prison",
    "Gorn",
    "Snivelak",
    "Smolg",
    "Damosel",
    "Grelbin",
    "Yeedil",
    NULL,
    "Dobbo Orbit",
    "Damosel Orbit",
    "Ship Shack",
    "Wupash Nebula",
    "Jamming Array",
    NULL,
    NULL,
    NULL,
    "Insomniac Museum",
};

/* The item numbers: the owned table's bytes, up to the next table. */
#define ITEMS 56
#define RING_SLOTS 8
#define ITEM_SIZE 0xE0u
#define STATE_PLAY 0
#define STATE_SCENE 2

/* An item's record, of the loaded program. */
#define ITEM(id)                                                                                   \
    (OPENRAC_DATA(RAC2_g_weaponTable)                                                              \
     + ITEM_SIZE * GREF(uint8_t, RAC2_g_itemEquippedSlot + (unsigned)(id)))
#define ITEM_WEAPON(r) GREF(int32_t, (r) + 0x00u) /* set for a weapon, whatever else it is */
#define ITEM_KIND(r) GREF(int32_t, (r) + 0x0Cu) /* 1 to 4 a gadget's or an item's group, -1 none */
#define ITEM_CLASS(r) GREF(uint16_t, (r) + 0x3Cu) /* 0: nothing the ring could show */
#define ITEM_AMMO_PRICE(r) GREF(uint16_t, (r) + 0x88u)
#define ITEM_AMMO_FULL(r) GREF(uint16_t, (r) + 0x8Eu)

static int in_level(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < LEVELS;
}

static int game_state(void) {
    return GREF(int32_t, OPENRAC_DATA(RAC2_g_nGameState));
}

/* The level a start is taking us to, or -1. New Game's own path writes the destination too, so one
 * write does not last: the boot program's word is held at the level, tick by tick, until the level's
 * program is in (probes through the interpreter reached levels 1 to 20 and 22 to 26 this way; 30 did
 * not run its own code). A level's copy of the word is elsewhere, so this holds in the boot program
 * only; from inside a level RequestLevelExit's argument does it. */
static int held_level = -1;

/* --level N with OPENRAC_DIRECT, and the developer menu. */
static void start_level(int level) {
    if (level < 0 || level >= LEVELS || level_names[level] == NULL) {
        return;
    }
    if (level == 30) {
        level =
            21; /* The museum: 30 is its archive's number, 21 the game's and its disc table's. */
    }
    if (!in_level()) {
        func_00298E08();
        held_level = level;
        GREF(int32_t, RAC2_g_nLevelExitDestination) = level;
    }
    GREF(int32_t, RAC2_g_playerProgress) = level;
    func_00288CB0(level, 0);
}

static void hold_destination(void) {
    if (held_level < 0) {
        return;
    }
    if (in_level()) {
        held_level = -1;
        return;
    }
    GREF(int32_t, RAC2_g_nLevelExitDestination) = held_level;
}

/* Straight into the level once the front end's state loop runs (func_0026EDE8): before it, the
 * splash screens wait for a press and its set-up clears what a start wrote. g_frameCounter counts
 * the loop's ticks and is 0 until then. */
#define LOOP_TICKS 30

static void direct_start(void) {
    static int done;
    if (done) {
        return;
    }
    if (openrac_game_start_level < 0 || openrac_env_string("OPENRAC_DIRECT") == NULL) {
        done = 1;
        return;
    }
    if (GREF(int32_t, RAC2_g_frameCounter) < LOOP_TICKS) {
        return;
    }
    done = 1;
    start_level(openrac_game_start_level);
}

void rac2_options_frame(void);

static void on_frame(unsigned frame) {
    (void)frame;
    rac2_options_frame();
    direct_start();
    hold_destination();
}

static int bolts(void) {
    return GREF(int32_t, RAC2_g_boltCount);
}

static int game_mode(uint32_t* mode) {
    if (!in_level()) {
        return 0;
    }
    *mode = (uint32_t)game_state();
    return 1;
}

/* While a program loads, func_002EA8E8 draws by g_exitSceneMode: 2 is the flight (func_002E8BA8), whose
 * sky and mobys are the lump func_002EAB18 loaded, not the level's. */
static int flight(void) {
    if (game_state() != 6 || GREF(int16_t, RAC2_g_exitSceneMode) != 2) {
        return -1;
    }
    return GREF(int16_t, RAC2_g_loadingSceneVariant);
}

static int hero_position(float xyz[3]) {
    if (!in_level()) {
        return 0;
    }
    memcpy(xyz, G(RAC2_g_heroPos), 12);
    return 1;
}

static int hero(float position[3], float rotation[3], int32_t* state) {
    if (!in_level()) {
        return 0;
    }
    memcpy(position, G(RAC2_g_heroPos), 12);
    memcpy(rotation, G(RAC2_g_heroOrientVec), 12);
    *state = GREF(int32_t, RAC2_g_heroState);
    return 1;
}

/* A scene ends on Start with the four shoulders, as in Ratchet & Clank, while skip_cutscenes is on. */
static int skip_buttons(void) {
    return in_level() && game_state() == STATE_SCENE ? 0x0F00 : 0;
}

/* An item into the hero's hands as GiveInventoryItem (0x00288308) leaves it: owned, shown as new,
 * full of ammunition, in the weapon lists when it is a weapon, and on the quick select ring's first
 * free slot when it is one the ring takes (0x00288208: kind 0, and a class to show). The game's own routine is in each
 * level at an address of its own; what it writes is here. */
static int give_item(int id) {
    const gaddr record = ITEM(id);
    int changed = 0;
    if (GREF(uint8_t, RAC2_g_inventoryOwned + (unsigned)id) == 0) {
        GREF(uint8_t, RAC2_g_inventoryOwned + (unsigned)id) = 1;
        GREF(uint8_t, RAC2_g_inventoryNewFlag + (unsigned)id) = 1;
        changed = 1;
    }
    if (ITEM_AMMO_PRICE(record) != 0) {
        GREF(int32_t, RAC2_g_weaponAmmo + 4u * (unsigned)id) = ITEM_AMMO_FULL(record);
    }
    if (ITEM_WEAPON(record) != 0) {
        for (unsigned i = 0; i < 32; ++i) {
            const uint8_t slot = GREF(uint8_t, RAC2_g_inventoryOrder + i);
            if ((slot & 0x3F) == id) {
                break;
            }
            if (slot == 0xFF) {
                GREF(uint8_t, RAC2_g_inventoryOrder + i) = (uint8_t)(id | 0x40);
                break;
            }
        }
        for (unsigned i = 0; ITEM_KIND(record) == 0 && ITEM_CLASS(record) != 0 && i < RING_SLOTS;
             ++i) {
            const gaddr slot = RAC2_g_quickSelectRing + 4u * i;
            if (GREF(int32_t, slot) == 0 || GREF(int32_t, slot) == id) {
                GREF(int32_t, slot) = id;
                break;
            }
        }
    }
    return changed;
}

static int give_kind(int weapons, int gadgets) {
    int changed = 0;
    for (int id = 1; id < ITEMS; ++id) {
        const gaddr record = ITEM(id);
        const int weapon = ITEM_WEAPON(record) != 0;
        const int gadget = !weapon && ITEM_KIND(record) > 0;
        if ((weapon && weapons) || (gadget && gadgets)) {
            changed += give_item(id);
        }
    }
    return changed;
}

static int give(const char* what) {
    if (strcmp(what, "all") == 0) {
        return give_kind(1, 1);
    }
    if (strcmp(what, "weapons") == 0) {
        return give_kind(1, 0);
    }
    if (strcmp(what, "gadgets") == 0) {
        return give_kind(0, 1);
    }
    if (strncmp(what, "bolts:", 6) == 0) {
        GREF(int32_t, RAC2_g_boltCount) = atoi(what + 6);
        return 1;
    }
    char* end = NULL;
    const long id = strtol(what, &end, 0);
    if (end == what || *end != '\0' || id <= 0 || id >= ITEMS) {
        return -1;
    }
    return give_item((int)id);
}

/* g_health counts down from what the level's start gave him: the most it has been is full. */
static void heal(void) {
    static int32_t full;
    const int32_t health = GREF(int32_t, RAC2_g_health);
    if (!in_level()) {
        full = 0;
        return;
    }
    if (health > full) {
        full = health;
    }
    if (health > 0 && health < full) {
        GREF(int32_t, RAC2_g_health) = full;
    }
}

static int teleport(const float xyz[3]) {
    if (!in_level()) {
        return 0;
    }
    memcpy(G(RAC2_g_heroPos), xyz, 12);
    return 1;
}

/* The game's own skip: Start with the shoulders, pressed for it (skip_buttons). */
static int end_scene(void) {
    return in_level() && game_state() == STATE_SCENE;
}

static int state_json(char* out, int size) {
    int at = snprintf(
        out,
        (size_t)size,
        "\"bolts\":%d,\"health\":%d,\"game_state\":%d,\"progress_level\":%d,\"owned\":[",
        (int)GREF(int32_t, RAC2_g_boltCount),
        (int)GREF(int32_t, RAC2_g_health),
        game_state(),
        (int)GREF(int32_t, RAC2_g_playerProgress)
    );
    int first = 1;
    for (int id = 1; id < ITEMS && at < size - 16; ++id) {
        if (GREF(uint8_t, RAC2_g_inventoryOwned + (unsigned)id) != 0) {
            at += snprintf(out + at, (size_t)(size - at), "%s%d", first ? "" : ",", id);
            first = 0;
        }
    }
    at += snprintf(out + at, (size_t)(size - at), "],\"ammo\":{");
    first = 1;
    for (int id = 1; id < ITEMS && at < size - 24; ++id) {
        const int ammo = GREF(int32_t, RAC2_g_weaponAmmo + 4u * (unsigned)id);
        if (ammo != 0) {
            at +=
                snprintf(out + at, (size_t)(size - at), "%s\"%d\":%d", first ? "" : ",", id, ammo);
            first = 0;
        }
    }
    at += snprintf(out + at, (size_t)(size - at), "}");
    return at;
}

static const openrac_game_tools tools = {
    .give = give,
    .heal = heal,
    .teleport = teleport,
    .end_scene = end_scene,
    .state_json = state_json,
};

static const openrac_front_end front_end = {
    .levels = LEVELS,
    .level_names = level_names,
    .start_level = start_level,
    .pad = 0x00138180u,
    /* D_001C2740: the 16 directional light sets MobyProc lights each moby with, filled from the
     * gameplay file and changed at run time (set 13 is the bolts' gold, 1.0 0.8 0.0). */
    .light_bank = 0x001C2740u,
    .start_level_anywhere = 1,
};

/* From crt0 (entry.c), not a constructor: the game is a static library, and nothing else names this
 * file, so a constructor here is dropped with it at the link. */
void rac2_install_tools(void) {
    openrac_game_on_frame = on_frame;
    openrac_game_bolts = bolts;
    openrac_game_mode = game_mode;
    openrac_game_flight = flight;
    openrac_game_hero_position = hero_position;
    openrac_game_hero = hero;
    openrac_game_skip_buttons = skip_buttons;
    openrac_game_front_end = &front_end;
    openrac_game_tools_host = &tools;
}
