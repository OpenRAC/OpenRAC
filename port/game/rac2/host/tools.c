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
#include "rac2.h"

void func_00288CB0(int level, int fade);
void func_00298E08(void);
/* The on-screen pad's context (touch.c). */
int rac2_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]);

/* The levels, by the game's own numbers (games/rac2/decomp/docs/LEVELS.md): 21 and 27 to 29 have no
 * program on the disc. */
#define LEVELS RAC2_LEVELS
const char* const rac2_level_names[RAC2_LEVELS] = {
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
#define STATE_LOADING 6

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

/* How a program's load is drawn while game state 6 runs (g_exitSceneMode; func_002E9268 updates
 * it): 0 and 3 a scene, which skips on Start itself, 1 the ship leaving a planet, 2 the flight. */
#define EXIT_SCENE_TAKE_OFF 1
#define EXIT_SCENE_FLIGHT 2

static int exit_scene(void) {
    return game_state() == STATE_LOADING ? GREF(int16_t, RAC2_g_exitSceneMode) : -1;
}

/* The level a start is taking us to, or -1. New Game's own path writes the destination too, so one
 * write does not last: the boot program's word is held at the level, tick by tick, until the level's
 * program is in (probes through the interpreter reached levels 1 to 20 and 22 to 26 this way; 30 did
 * not run its own code). A level's copy of the word is elsewhere, so this holds in the boot program
 * only; from inside a level RequestLevelExit's argument does it. */
static int held_level = -1;

/* --level N with OPENRAC_DIRECT, the developer menu, and the play calls (play.c). */
void rac2_start_level(int level, int new_game) {
    if (level < 0 || level >= LEVELS || rac2_level_names[level] == NULL) {
        return;
    }
    if (level == 30) {
        level =
            21; /* The museum: 30 is its archive's number, 21 the game's and its disc table's. */
    }
    if (!in_level()) {
        if (new_game) {
            func_00298E08();
        }
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
    rac2_start_level(openrac_game_start_level, 1);
}

void rac2_options_frame(void);
int rac2_planets_take(void);
static void flight_skip(void);

static void on_frame(unsigned frame) {
    (void)frame;
    rac2_options_frame();
    direct_start();
    /* The developer mode's Planets page (options.c): the level chosen there. */
    const int planet = rac2_planets_take();
    if (planet >= 0) {
        rac2_start_level(planet, 1);
    }
    hold_destination();
    flight_skip();
    rac2_play_frame();
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

/* The flight's lump is the world the game draws while the sky bound is the lump's: func_002EAB18
 * decompresses it at the arena's header place (+0x18) and binds the sky at the lump's data (header
 * +4) plus its +0x48 word. A level's load binds its own sky, elsewhere. */
#define ARENA_HEADER 0x18u
#define LUMP_DATA 0x04u
#define LUMP_SKY 0x48u

static int flight_bound(void) {
    const uint32_t lump = GREF(uint32_t, OPENRAC_DATA(RAC2_g_memoryArenaTable) + ARENA_HEADER);
    if (lump == 0) {
        return 0;
    }
    const uint32_t sky = GREF(uint32_t, OPENRAC_DATA(RAC2_g_pSkyData));
    return sky == lump + GREF(uint32_t, lump + LUMP_DATA) + GREF(uint32_t, lump + LUMP_SKY);
}

/* While a program loads, func_002EA8E8 draws by g_exitSceneMode: 2 is the flight (func_002E8BA8), whose
 * sky and mobys are the lump func_002EAB18 loaded, not the level's. A level's exit can play a scene in
 * that world before the next program loads (level 0's ends so: game state 2, then 1, then 6 again), and
 * the scene's draw (func_0027C480's copy) draws the world bound, the lump's. g_exitSceneMode stays 2 into
 * the next level, whose opening scene is its own world: the bound sky tells them apart. */
static int flight(void) {
    if (GREF(int16_t, RAC2_g_exitSceneMode) != EXIT_SCENE_FLIGHT
        || (game_state() != STATE_LOADING && !flight_bound())) {
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

/* What the game's own skip of the scene playing now takes, for Start to press too (the window's
 * pad bits) while skip_cutscenes is on. A level's scene ends on Start with the four shoulders, as in
 * Ratchet & Clank. The ship leaving a planet (func_002E9268's mode 1) fades out and ends on
 * Triangle or Cross pressed once its first 120 ticks have played; Start alone does nothing there,
 * so Start presses Cross, as rac1's take-off does. The flight itself has no skip on the console:
 * skip() below. */
static int skip_buttons(void) {
    if (in_level() && game_state() == STATE_SCENE) {
        return 0x0F00;
    }
    if (exit_scene() == EXIT_SCENE_TAKE_OFF) {
        return 0x4000;
    }
    return 0;
}

/*
 * Start during the flight between planets, a port feature (skip_cutscenes, differences.json): the
 * flight ends and the next planet's own arrival plays. The flight (func_002E8368, game state 6,
 * g_exitSceneMode 2) plays laps of a scene at 0x1B8840 (+0x34 its tick, +0x40 its last): its first
 * lap, a random one of four after it while the next level's archives load, a last random one, then
 * the approach to the planet (lap 4). A lap ends when its tick reaches the last. The phase word
 * (0x152C68) counts the laps since the archives were read (g_levelStagingState's stage 2; before,
 * every lap's end starts another): at 0 a lap's end starts the last random lap, at 1 the approach,
 * at 2 the flight is over.
 *
 * One press is kept until the flight is over (flight_skip): from the press on, once the archives
 * are read, the phase is put at 1, so the lap playing is the last before the approach, and each lap
 * is ended as soon as it may be, after its first two ticks, where it starts its sound. The approach
 * waits for the level's banks (stage 5, which it reaches by its third tick) and is then moved to
 * its second last tick: its last tick before the end runs as it would have, which is the one that
 * asks for the landing (the mode block's +0x2C) and leaves the ship's fade at 0, as an approach
 * played through leaves them. The level then comes in as it would have, with its landing (a scene
 * of game state 6, g_exitSceneMode 0 or 3), which plays: the press is not carried into it, and
 * Start skips it as on the console (func_002E9268 reads the press there).
 */
/* The boot program's scene; a level's copy of it through OPENRAC_DATA, as the fade's. */
#define FLIGHT_SCENE OPENRAC_DATA(RAC2_ADDR(0x001B8840u))
#define FLIGHT_TICK (FLIGHT_SCENE + 0x34u)
#define FLIGHT_LAST (FLIGHT_SCENE + 0x40u)
#define FLIGHT_FIRST_TICKS 2
#define FLIGHT_LAP                                                                                 \
    RAC2_ADDR(0x00152C64u) /* the lap playing: 0 to 3 the laps through space, 4 the approach */
#define FLIGHT_PHASE RAC2_ADDR(0x00152C68u) /* laps begun since the archives were read */
#define FLIGHT_APPROACH 4
#define PHASE_LAST_LAP 1
#define ARCHIVES_READ 2
#define BANKS_LOADING 5

static int flight_skipping;

static void flight_skip(void) {
    if (exit_scene() != EXIT_SCENE_FLIGHT) {
        flight_skipping = 0;
        return;
    }
    const int32_t stage = GREF(int32_t, RAC2_g_levelStagingState);
    if (!flight_skipping || stage < ARCHIVES_READ) {
        return;
    }
    const int approach = GREF(int32_t, FLIGHT_LAP) == FLIGHT_APPROACH;
    if (!approach && GREF(int32_t, FLIGHT_PHASE) < PHASE_LAST_LAP) {
        GREF(int32_t, FLIGHT_PHASE) = PHASE_LAST_LAP;
    }
    if (approach && stage < BANKS_LOADING) {
        return;
    }
    const int32_t last = GREF(int16_t, FLIGHT_LAST) - (approach ? 2 : 0);
    const int32_t tick = GREF(int32_t, FLIGHT_TICK);
    if (tick >= FLIGHT_FIRST_TICKS && tick < last) {
        GREF(int32_t, FLIGHT_TICK) = last;
    }
}

/* Called while Start is held with skip_cutscenes on. */
static void skip(void) {
    if (exit_scene() == EXIT_SCENE_FLIGHT) {
        flight_skipping = 1;
    }
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
            const gaddr slot = RAC2_g_equippedItemSlots + 4u * i;
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

/*
 * OPENRAC_SKIP_SCENES, the test variable (a player's Start leaves the arrival to play): the scene
 * ends as soon as it can. A level's own scene is ended by the pad (skip_buttons); the arrival after
 * a flight (g_exitSceneMode 0 or 3) by its skip word, the mode block's +0x26 that func_002E9268
 * reads with the press, once the scene takes a press: from its 13th tick, faded in (D_001B15A0 0),
 * and before its last 30.
 */
#define SCENE_SKIP RAC2_ADDR(0x00152C26u) /* a short: the scene ends at the next tick */
#define SCENE_FADE                                                                                 \
    OPENRAC_DATA(RAC2_ADDR(0x001B15A0u)) /* a float: the fade, 0 when the picture is in */
#define SCENE_FIRST_TICKS 13
#define SCENE_LAST_TICKS 30

static int end_scene(void) {
    const int mode = exit_scene();
    if (mode == 0 || mode == 3) {
        const int32_t tick = GREF(int32_t, FLIGHT_TICK);
        if (tick >= SCENE_FIRST_TICKS && GREF(float, SCENE_FADE) == 0.0f
            && tick < GREF(int16_t, FLIGHT_LAST) - SCENE_LAST_TICKS) {
            GREF(int16_t, SCENE_SKIP) = 1;
        }
    }
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

static void front_end_start_level(int level) {
    rac2_start_level(level, 1);
}

static const openrac_front_end front_end = {
    .levels = LEVELS,
    .level_names = rac2_level_names,
    .start_level = front_end_start_level,
    .pad = RAC2_ADDR(0x00138180u),
    /* D_001C2740: the 16 directional light sets MobyProc lights each moby with, filled from the
     * gameplay file and changed at run time (set 13 is the bolts' gold, 1.0 0.8 0.0). */
    .light_bank = RAC2_ADDR(0x001C2740u),
    /* D_001BADC0: the frame's occlusion mask (0x80 bytes). The terrain's, the ties' and MobyProc's
     * culls copy it to the scratchpad and leave out what the camera's cell cannot see: a tfrag
     * whose bit (+0x3A) is clear in byte +0x3B, a tie (+0x10, byte +0x11), a moby (+0x36, +0x37). */
    .occlusion = RAC2_ADDR(0x001BADC0u),
    .start_level_anywhere = 1,
    /* D_001A7BB4: the language, a byte the menus' action type 9 sets (0 English, 2 French, 3 German,
     * 4 Spanish, 5 Italian: the values of the language list's items), read as a word as the text lookup reads it
     * (func_002D5E98); the port's rows on the Video pages follow it. */
    .language = RAC2_ADDR(0x001A7BB4u),
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
    openrac_game_skip = skip;
    openrac_game_touch = rac2_touch;
    openrac_game_front_end = &front_end;
    openrac_game_tools_host = &tools;
    rac2_install_play();
    rac2_install_mod_pages();
}
