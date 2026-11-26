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
 * - 0x001A4C60: the hero's position (the hero code's base 0x001A4BE0 + 0x80), +0x10 his roll,
 *   pitch and yaw; 0x001A71A4 his state (rac3-decomp enums/hero_state.h), 0x001A7430 his health.
 * - What he holds (rac3-decomp include/names.h): player_bolts 0x00142660, hero_max_health
 *   0x00142668, item_ammo 0x001427F0 (a word an item), item_owned 0x00142CA0 (a byte an item, 0xA0
 *   items, up to the new flags at 0x00142D40), the item's version 0x001425C0 (a byte an item: its
 *   record among the 0xF0-byte item records, Veldin's D_L01_002C5600, each level's own copy; +0 set
 *   for a gun, +0x84 its ammunition's price, +0x8E a full load), held_item 0x001A5E08 and
 *   equipped_gun 0x001D4C40. item_give (Veldin's func_L01_00419188, every story level's copy
 *   folded with it) gives one as the game's own pick-ups and scenes do.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

void func_L01_00419188(int item);            /* item_give, the loaded level's copy */
int func_L01_003E4580(int camera, int move); /* camera_platform_ride, the loaded level's copy */

#define RAC3_LEVELS 54
#define FRONT_END_MODE RAC3_ADDR(0x001D5B94u)
#define FRONT_END_STATE RAC3_ADDR(0x001D5458u)
#define LEVEL_CURRENT RAC3_ADDR(0x001D545Cu)
#define BOOT_LEVEL_NEXT RAC3_ADDR(0x001D9D04u)
#define HERO_POSITION RAC3_ADDR(0x001A4C60u)
#define HERO_ROTATION RAC3_ADDR(0x001A4C70u)
#define HERO_STATE RAC3_ADDR(0x001A71A4u)
#define HERO_HEALTH RAC3_ADDR(0x001A7430u)
#define PADS RAC3_ADDR(0x001CD0C0u)
#define BOLTS RAC3_ADDR(0x00142660u)
#define MAX_HEALTH RAC3_ADDR(0x00142668u)
#define ITEM_AMMO RAC3_ADDR(0x001427F0u)
#define ITEM_OWNED RAC3_ADDR(0x00142CA0u)
/* Each item's experience, in 32nds (the boot program's 0x00142DE0; a kill adds to it). */
#define ITEM_EXPERIENCE RAC3_ADDR(0x00142DE0u)
#define ITEM_VERSION RAC3_ADDR(0x001425C0u)
#define HELD_ITEM RAC3_ADDR(0x001A5E08u)
#define EQUIPPED_GUN RAC3_ADDR(0x001D4C40u)
#define ITEMS 0xA0
#define ITEM_RECORDS OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x002C5600u))
/* Player 0's game camera (camera_manager_update's, 0x460 a player): +0x50 the current mode's update
 * object, whose +0x30 is last frame's camera and +0x8C its mode (0 the follow camera). */
#define CAMERA_ENTRY                                                                               \
    RAC3_LADDR(1, 0x001E2A40u) /* Veldin's; the loaded level's through OPENRAC_LDATA */
#define CAMERA_OBJECT 0x50u
#define CAMERA_MODE 0x8Cu
/* hero_carried: what the hero's platform moved him this frame, a quadword the follow camera's
 * platform ride takes. */
#define HERO_CARRIED RAC3_ADDR(0x001A4D30u)
#define ITEM_RECORD_BYTES 0xF0u
#define RECORD_AMMO_PRICE 0x84u
#define RECORD_AMMO_FULL 0x8Eu
#define RECORD_NEXT_LEVEL 0x68u

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
            return RAC3_LADDR(12, 0x001D66E4u);
        case 19:
            return RAC3_LADDR(19, 0x001D66D4u);
        case 15:
        case 25:
            return 0;
        default:
            return level >= 1 && level <= 36 ? RAC3_LADDR(1, 0x001D6694u) : 0;
    }
}

static int loaded_level(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < RAC3_LEVELS && level_names[level] != NULL ? level : -1;
}

/* openrac_game_flight: the loading flight between planets played by a level's own program. Leaving
 * a planet, the level's program runs its own copy of the loading screen (the catalogue's frontbin
 * rows func_003BD4B8, the loop, and func_003BD030, level_load: Veldin's 0x004D7AE0 and 0x004D7658),
 * not the boot program's: no program is loaded until the flight ends and 0x0013AFF8 copies the next
 * level over this one. level_load sets level_current to the level asked for as the loop starts, so
 * from then on it is not the loaded level. The window took the loaded level for the world drawn
 * and drew the planet being left (its sky, and its terrain, which the flight's DrawTfrag copy,
 * Veldin's 0x004B1728, marks) round the ship with the flight's camera: Veldin's sky over the loading
 * screen. It is the loading screen's world instead, as the boot program's flight is
 * (GameProfile::loading_world, level 97: the ship's scene's mobys, no terrain and no sky). The
 * multiplayer maps have no flight. */
int rac3_level_flight(void);

int rac3_level_flight(void) {
    const int level = loaded_level();
    if (level < 1 || level > 36) {
        return -1;
    }
    const int32_t current = GREF(int32_t, LEVEL_CURRENT);
    return current >= 0 && current < RAC3_LEVELS && current != level ? 0 : -1;
}

/* openrac_game_loading_draws_world: the transition the game chose (level_load's table row, its
 * `value` at 0x0016C580 + 0x2E, core data) is the flight, whose ship and stars come in a moby pass
 * of their own, unless it is the Vid-comic one (value 6: the Phoenix to a Vid-comic level and
 * back), a full-screen picture and two turning sprites that no world is under. */
#define LOADING_VALUE (RAC3_ADDR(0x0016C580u) + 0x2Eu)
#define LOADING_VIDCOMIC 6

static int loading_draws_world(void) {
    return GREF(int16_t, LOADING_VALUE) != LOADING_VIDCOMIC;
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

/* For OPENRAC_WALK, OPENRAC_RECORD and the crash report: where the hero is, which way he faces and
 * his state, as Going Commando's host/tools.c gives them. */
static int hero(float position[3], float rotation[3], int32_t* state) {
    if (loaded_level() < 0) {
        return 0;
    }
    memcpy(position, G(HERO_POSITION), 12);
    memcpy(rotation, G(HERO_ROTATION), 12);
    *state = GREF(int32_t, HERO_STATE);
    return 1;
}

/* OPENRAC_TELEPORT: the hero to xyz, and the follow camera with him, as a platform carries both
 * (camera_platform_ride, which the follow camera's update calls each frame with the platform's move:
 * the camera, its look point, follow point and height targets). Left behind, the camera's obstacle
 * test (camera_end_sphere) ran a line from it to the hero's new place; one over about 1000 units
 * crosses more than the 256 cells the line query's list holds in the scratchpad, and its next entry
 * was written at 0x70004000, past the scratchpad's end (where the console faults as well). */
static int teleport(const float xyz[3]) {
    if (loaded_level() < 0) {
        return 0;
    }
    float move[4];
    for (int i = 0; i < 3; ++i) {
        move[i] = xyz[i] - GREF(float, HERO_POSITION + 4u * (unsigned)i);
    }
    move[3] = 0.0f;
    memcpy(G(HERO_POSITION), xyz, 12);
    const gaddr entry = CAMERA_ENTRY != 0 ? OPENRAC_LDATA(1, (gaddr)CAMERA_ENTRY) : 0;
    const gaddr camera = entry != 0 ? GREF(gaddr, entry + CAMERA_OBJECT) : 0;
    if (camera != 0 && HERO_CARRIED != 0 && GREF(int16_t, camera + CAMERA_MODE) == 0) {
        uint8_t carried[16];
        memcpy(carried, G(HERO_CARRIED), sizeof carried);
        memcpy(G(HERO_CARRIED), move, sizeof move);
        func_L01_003E4580((int)camera, (int)HERO_CARRIED);
        memcpy(G(HERO_CARRIED), carried, sizeof carried);
    }
    return 1;
}

/* OPENRAC_INVINCIBLE: health counts down from what the level's start gave him, so the most it has
 * been is full (Going Commando's heal). */
static void heal(void) {
    static int32_t full;
    const int32_t health = GREF(int32_t, HERO_HEALTH);
    if (loaded_level() < 0) {
        full = 0;
        return;
    }
    if (health > full) {
        full = health;
    }
    if (health > 0 && health < full) {
        GREF(int32_t, HERO_HEALTH) = full;
    }
}

/* An item's record, of the loaded level (the item's version picks it). */
static gaddr item_record(int id) {
    return ITEM_RECORDS + ITEM_RECORD_BYTES * GREF(uint8_t, ITEM_VERSION + (unsigned)id);
}

/* OPENRAC_GIVE: an item as the game gives one (item_give: owned, shown as new, a first load of
 * ammunition, into the weapon lists and the quick select ring's first free slot), then a full load
 * of ammunition for one that takes any, as Going Commando's give does. */
static int give_item(int id) {
    int changed = 0;
    if (GREF(uint8_t, ITEM_OWNED + (unsigned)id) == 0) {
        func_L01_00419188(id);
        changed = GREF(uint8_t, ITEM_OWNED + (unsigned)id) != 0;
    }
    const gaddr record = item_record(id);
    if (GREF(uint16_t, record + RECORD_AMMO_PRICE) != 0) {
        GREF(int32_t, ITEM_AMMO + 4u * (unsigned)id) = GREF(uint16_t, record + RECORD_AMMO_FULL);
    }
    return changed;
}

/* The twenty guns, by their V1 item numbers (the save editor's table, launcher/core/src/
 * save_edit_rac3.rs, read from the disc's weapon table): an item's record is set at +0 for every
 * row of the table, the gadgets' and the weapons' later versions too, so the record does not tell
 * them apart. */
static const int guns[] = {39,  119, 47, 127, 55,  135, 71, 87, 111, 63,
                           103, 79,  95, 143, 151, 21,  17, 22, 19,  16};

static int give_guns(void) {
    int changed = 0;
    for (unsigned i = 0; i < sizeof guns / sizeof guns[0]; ++i) {
        changed += give_item(guns[i]);
    }
    return changed;
}

/* OPENRAC_GIVE=xp: every owned gun's experience to its version's next level (the record's +0x68,
 * as weapon_level_up reads it), so the game levels it up as it would after a kill: for checking
 * the upgrade's screen without playing to it. */
static int give_experience(void) {
    int changed = 0;
    for (unsigned i = 0; i < sizeof guns / sizeof guns[0]; ++i) {
        const int id = guns[i];
        if (GREF(uint8_t, ITEM_OWNED + (unsigned)id) == 0) {
            continue;
        }
        const int32_t next = GREF(int32_t, item_record(id) + RECORD_NEXT_LEVEL);
        if (next > 0 && GREF(int32_t, ITEM_EXPERIENCE + 4u * (unsigned)id) < next << 5) {
            GREF(int32_t, ITEM_EXPERIENCE + 4u * (unsigned)id) = next << 5;
            ++changed;
        }
    }
    return changed;
}

static int give(const char* what) {
    if (strncmp(what, "bolts:", 6) == 0) {
        GREF(int32_t, BOLTS) = atoi(what + 6);
        return 1;
    }
    /* The story levels, which have item_give and the item records (the mode word's levels). */
    const int level = loaded_level();
    if (level < 0 || level_mode_word(level) == 0 || level > 36) {
        return 0;
    }
    if (strcmp(what, "all") == 0 || strcmp(what, "weapons") == 0) {
        return give_guns();
    }
    if (strcmp(what, "xp") == 0) {
        return give_experience();
    }
    char* end = NULL;
    const long id = strtol(what, &end, 0);
    if (end == what || *end != '\0' || id <= 0 || id >= ITEMS) {
        return -1;
    }
    return give_item((int)id);
}

static int bolts(void) {
    return GREF(int32_t, BOLTS);
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
    if (level >= 0 && at < size) {
        at += snprintf(
            out + at,
            (size_t)(size - at),
            ",\"bolts\":%d,\"health\":%d,\"max_health\":%d,\"held_item\":%d,\"equipped_gun\":%d,"
            "\"owned\":[",
            (int)GREF(int32_t, BOLTS),
            (int)GREF(int32_t, HERO_HEALTH),
            (int)GREF(int32_t, MAX_HEALTH),
            (int)GREF(int32_t, HELD_ITEM),
            (int)GREF(int32_t, EQUIPPED_GUN)
        );
        int first = 1;
        for (int id = 1; id < ITEMS && at < size - 16; ++id) {
            if (GREF(uint8_t, ITEM_OWNED + (unsigned)id) != 0) {
                at += snprintf(out + at, (size_t)(size - at), "%s%d", first ? "" : ",", id);
                first = 0;
            }
        }
        if (at < size - 16) {
            at += snprintf(out + at, (size_t)(size - at), "],\"ammo\":{");
        }
        first = 1;
        for (int id = 1; id < ITEMS && at < size - 24; ++id) {
            const int ammo = GREF(int32_t, ITEM_AMMO + 4u * (unsigned)id);
            if (ammo != 0 && GREF(uint8_t, ITEM_OWNED + (unsigned)id) != 0) {
                at += snprintf(
                    out + at, (size_t)(size - at), "%s\"%d\":%d", first ? "" : ",", id, ammo
                );
                first = 0;
            }
        }
        if (at < size - 1) {
            at += snprintf(out + at, (size_t)(size - at), "}");
        }
    }
    return at < size ? at : size - 1;
}

/* What Start presses while skip_cutscenes is on, and what OPENRAC_SKIP_SCENES presses: a level's
 * scenes (its mode 2, and 6, the landing that opens a level) end on Start alone, as on the
 * console (Veldin's opening, frames 722 to 862 of a run, ends 7 frames after a press of Start, with
 * the setting on or off). Pressing it is all a skip takes; a level's loading flights are
 * loading.c's (openrac_game_skip). */
#define BUTTON_START 0x0008
#define MODE_SCENE 2
#define MODE_LANDING 6

static int skip_buttons(void) {
    uint32_t mode = 0;
    if (!rac3_level_mode(&mode)) {
        return 0;
    }
    return mode == MODE_SCENE || mode == MODE_LANDING ? BUTTON_START : 0;
}

static const openrac_game_tools tools = {
    .give = give,
    .heal = heal,
    .teleport = teleport,
    .state_json = state_json,
};

int rac3_start_level(int level); /* program.c */

/* The developer menu's levels (debug_menu.cpp), from the front end or from a level. */
static void front_end_start_level(int level) {
    if (level >= 0 && level < RAC3_LEVELS && level_names[level] != NULL) {
        rac3_start_level(level);
    }
}

static openrac_front_end front_end = {
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
    /* The frame's occlusion mask (0x80 bytes) in Veldin's program, as the profile's scenery words
     * (scenery_matrices_level): CullAndEmitTfrags (func_L01_004B1800), TieProc (0x004B3590) and
     * MobyProc (0x00441B78) all read it (Going Commando's D_001BADC0) and leave out what the
     * camera's cell cannot see; the window culls the scenery it draws by it too. The boot program's
     * copy, D_00227480 (set all visible at boot), is no level's. */
    .occlusion = RAC3_LADDR(1, 0x001EA840u),
};

/* From crt0 (entry.c), not a constructor: the game is a static library, and nothing else names this
 * file, so a constructor here is dropped with it at the link. */
void rac3_loading_skip(void);   /* host/loading.c */
void rac3_loading_install(void); /* host/loading.c */
gaddr rac3_language_word(void); /* host/options.c */
int rac3_touch(const char** context, const char* hints[OPENRAC_TOUCH_HINTS_MAX]); /* host/touch.c */

void rac3_install_tools(void) {
    openrac_game_skip = rac3_loading_skip;
    openrac_game_mode = rac3_level_mode;
    openrac_game_flight = rac3_level_flight;
    openrac_game_loading_draws_world = loading_draws_world;
    rac3_loading_install();
    openrac_game_bolts = bolts;
    openrac_game_skip_buttons = skip_buttons;
    openrac_game_touch = rac3_touch;
    openrac_game_hero_position = hero_position;
    openrac_game_hero = hero;
    front_end.language = rac3_language_word();
    openrac_game_front_end = &front_end;
    openrac_game_tools_host = &tools;
}
