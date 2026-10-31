/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the level's in-game scenes for the mods (game_host.h, openrac_game_play's
 * scene_playing and scene_start; interface 1.9, multiplayer): the scene playing, and a scene
 * started as the level's own trigger starts it, so every game of a party plays the scenes the
 * host's plays.
 *
 * How the game starts one. StartCutscene (func_L00_00299B68, src/overlays/shared/
 * cutscene_and_movie_playback.c; the US disc's FUN_L00_00298840, host C there:
 * rac1-ntsc/host/scene_start.c) takes the scene's number in the level: it clears the scene's
 * block (cutscene_state, D_L00_0016C960: +0x30 the number, +0x34 the frames played, +0x40 the
 * last frame, +0x44 the actors' count, +0x178 the actors), sets the game mode (D_L00_0015F6A8) to
 * 2, takes the hero over (HeroEnterMoveState 0x64), reads the scene's chunk from the disc and makes
 * its actors (UpdateWorldObjectAnimation: a moby per part, scene_actors.c), then waits for its
 * music. From then on the level's loop runs CutsceneModeUpdate each tick until the last frame or
 * the skip (finish_cutscene: the actors deleted, game mode 0, the hero put where the scene leaves
 * him). Nothing else sets game mode 2. Its callers are the level's mobys' updates: a person's
 * dialog entry (Npc_TryStartDialog), trigger volumes and controllers (classes 684, 737, 834,
 * 1156, 1404, 1577), the ship on landing (story scenes 4 to 6), story pickups (the Pilot's Helmet),
 * the codebots, the infobots. Veldin's opening is class 834's (clank_cutscene_trigger_update,
 * func_L00_002DB278): on a new game its dialog range is the whole level, so its first update
 * starts the scene through Npc_TryStartDialog.
 *
 * Why a member's game does not start it by itself: a trigger is a moby whose update reads the
 * game's own hero, and the multiplayer mod drives the level's placed mobys of the members' games
 * from the host's (world.hpp: all but crates, pickups and interactables), so their updates, and
 * the scenes they start, run in the host's game only. Here a mod starts the same scene in its game.
 *
 * Which scenes count: those the level's mobys start, the game mode going from 0 to 2 during the
 * tick's moby loop (and those a mod starts here). The pause menu's own (its Extras: a scene watched
 * again, which closes the menu and starts it after the loop) do not: they are one player's.
 *
 * Where it starts: after the tick's moby loop (UpdateMobys, crash_state.c), where a trigger's
 * update calls StartCutscene, in game mode 0 with the hero bound and alive (StartCutscene kills a
 * hero at 0 health; the PAL one returns in a scene, the US one does not test). Asked in another
 * mode (a menu, another scene, the ship), the request waits for game mode 0; it goes with the
 * level (a new program load) and when a mod drops it (-1).
 *
 * The actors: a moby among the block's actors while game mode is 2 is the scene's (the world
 * listing's OPENRAC_MOBY_SCENE): each game playing the scene makes its own, so a mod does not
 * share them between games.
 */
#include "rac1.h"

#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define MODE_PLAY 0
#define MODE_CUTSCENE 2

/* cutscene_state: the scene's number, its frames played, its actors' count and list (at most 18:
 * the block is 0x1C0 bytes). */
#define SCENE_STATE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0016C960u))
#define SCENE_NUMBER 0x30u
#define SCENE_FRAME 0x34u
#define SCENE_ACTOR_COUNT 0x44u
#define SCENE_ACTORS 0x178u
#define SCENE_ACTORS_MAX 18

/* The hero (include/hero.h): his moby and his health. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_HEALTH (HERO + 0x22A8u)

/* A scene a mod asked for (-1 none), and the level load it was asked in. */
static int32_t wanted = -1;
static uint32_t wanted_load;

/* The game mode as the moby loop began (-1 outside a level), and the scene the level's mobys (or a
 * mod) started, with the level load it played in (-1 none). */
static int loop_mode = -1;
static int32_t level_scene = -1;
static uint32_t level_scene_load;

static int cutscene_playing(void) {
    return rac1_level_running() && GREF(int, LEVEL_MODE) == MODE_CUTSCENE;
}

static int32_t scene_number(void) {
    return GREF(int32_t, SCENE_STATE + SCENE_NUMBER);
}

/* The scene playing is the level's (started by its mobys or a mod) when it is the one noted. */
static int level_scene_playing(void) {
    return cutscene_playing() && level_scene >= 0 && level_scene == scene_number()
           && level_scene_load == rac1_moby_generation();
}

static void note_level_scene(void) {
    level_scene = scene_number();
    level_scene_load = rac1_moby_generation();
}

int32_t rac1_scene_playing(int32_t* frame) {
    if (!level_scene_playing()) {
        return -1;
    }
    if (frame != NULL) {
        *frame = GREF(int32_t, SCENE_STATE + SCENE_FRAME);
    }
    return scene_number();
}

int rac1_scene_start(int32_t scene) {
    if (scene < 0) {
        wanted = -1;
        return 1;
    }
    if (!rac1_level_running()) {
        return 0;
    }
    wanted = scene;
    wanted_load = rac1_moby_generation();
    return 1;
}

void rac1_scenes_loop_begin(void) {
    loop_mode = rac1_level_running() ? GREF(int, LEVEL_MODE) : -1;
}

void rac1_scenes_tick(void) {
    /* A scene the loop's mobys started. */
    if (loop_mode == MODE_PLAY && cutscene_playing()) {
        note_level_scene();
    }
    loop_mode = -1;
    if (wanted < 0) {
        return;
    }
    if (!rac1_level_running() || wanted_load != rac1_moby_generation()) {
        wanted = -1;
        return;
    }
    if (GREF(int, LEVEL_MODE) != MODE_PLAY || GREF(gaddr, HERO_MOBY) == 0
        || GREF(int32_t, HERO_HEALTH) <= 0) {
        return;
    }
    const int32_t scene = wanted;
    wanted = -1;
    func_L00_00299B68(scene);
    if (cutscene_playing()) {
        note_level_scene();
    }
}

int rac1_scene_actor(gaddr moby) {
    if (moby == 0 || !cutscene_playing()) {
        return 0;
    }
    const gaddr state = SCENE_STATE;
    int count = GREF(int16_t, state + SCENE_ACTOR_COUNT);
    if (count > SCENE_ACTORS_MAX) {
        count = SCENE_ACTORS_MAX;
    }
    for (int i = 0; i < count; ++i) {
        if (GREF(gaddr, state + SCENE_ACTORS + 4u * (gaddr)i) == moby) {
            return 1;
        }
    }
    return 0;
}
