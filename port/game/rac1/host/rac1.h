/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank's host C, one copy for every version of the game (port/game/rac1/README.md).
 *
 * The files here are compiled into each version's program (hostgen.json, "host_shared") with that
 * version's translation (game_protos.h) and its table of names and addresses (rac1_version.h in
 * port/game/rac1-<version>/host). They are written with games/rac1's names for the game's functions
 * (func_001F8B6C, func_L00_002A8A20, func_0022C9A8__game) and its addresses inside RAC1_ADDR():
 * each version's table turns them into its own, so a fix written here holds for every disc.
 */
#ifndef OPENRAC_RAC1_H
#define OPENRAC_RAC1_H

#include "game_protos.h"
#include "openrac/game_host.h"

/* After the translation's prototypes: a version's table renames games/rac1's names to its own. */
#include "rac1_version.h"

#ifndef RAC1_ADDR
#error "rac1_version.h must define RAC1_ADDR"
#endif

/* Whether this version leaves out a function of the shared C: `#if !RAC1_OMIT(func_...)` around it,
 * and RAC1_OMIT_func_... 1 in the version's table with the reason (its decompilation has C of its
 * own for the function, or its catalogue places something else there, or its game never calls it). */
#define RAC1_OMIT(f) RAC1_OMIT_##f

/* A function's name in this version, as a string (for logs and switches that name functions). */
#define RAC1_NAME_(f) #f
#define RAC1_NAME(f) RAC1_NAME_(f)

/* hand.c: the game's printf conversions from host arguments into `out` (at most size - 1
 * characters, terminated); the count written. */
#include <stdarg.h>
#include <stddef.h>
int rac1_vformat(char *out, size_t size, const char *format, va_list args);

/* boot.c: a new game straight in `level` from the title, as --level N with OPENRAC_DIRECT. */
void rac1_start_level(int level);

/* options.c: once a frame, the game's options the port remembers between runs. */
void rac1_options_frame(void);

/* moby_pool.c: a WAD decompressed to `to` (hand.c's WadDecompress, after it wrote it): the gameplay
 * header's spare moby count raised while the level loader runs. */
void rac1_wad_decompressed(gaddr to);

/* scene_actors.c: hide the scene's missing actors during the flight update (pack), then
 * restore the list (unpack). */
void rac1_scene_actors_pack(void);
void rac1_scene_actors_unpack(void);

/* boot.c: the level programs ParseBin loaded since the start. */
uint32_t rac1_program_loads(void);

/* Whether a level runs: its program loaded and its loop not told to end (play.c). After the end
 * (a travel, the ship's flight out, Quit Game) the level's program is still there while the
 * flight between planets plays (DoSpaceTransition), with the space scene's mobys in place of the
 * level's: nothing of the level may be read or written then. */
int rac1_level_running(void);

/* front_end.c: what the window's menus need of the game (game_host.h), and the levels' names. */
extern const openrac_front_end rac1_front_end;
extern const char* const rac1_level_names[];

/* play.c: what the mods' play calls need of the game (game_host.h, interfaces 1.8 and 1.9), and a
 * moby put in a pose (a figure, or a moby of the world a mod drives). */
extern const openrac_game_play rac1_play;
void rac1_pose_moby(gaddr moby, const openrac_game_pose* pose);

/* world.c: the level's mobys for the mods (interface 1.9): a moby's slot in the array (-1 none),
 * the mods' gate around each update (UpdateMobys, crash_state.c: 1 before it skips it), the hero's
 * stand-in ended after the update, and the points besides the camera that keep mobys near them
 * running (moby_pool.c's run list). */
int32_t rac1_moby_slot(gaddr moby);
int rac1_moby_gate(gaddr moby, int phase);
void rac1_stand_in_end(void);
int rac1_moby_eyes(const float** xyz);
int game_rac1_expanded_ram(void);
void game_rac1_warn_moby_grid_full(void);
/* world.c's calls of play.c's table (game_host.h, openrac_game_play: the same names without
 * rac1_world_). */
int rac1_world_mobys(openrac_game_moby_state* out, int max);
int32_t rac1_world_moby_slot(int32_t key);
gaddr rac1_world_moby_at(int32_t slot);
void rac1_world_moby_kill(gaddr moby);
/* CreateMoby (func_0020D348) of a moby of the level's, also while the pause menu's mobys are the
 * current ones; 0 for none. */
gaddr rac1_world_create_moby(int32_t oclass);
void rac1_world_moby_hide(gaddr moby, int hidden);
int rac1_world_moby_deletion(int32_t slot, openrac_game_moby_deletion* out);
int rac1_world_camera_position(float out[3]);
int rac1_world_moby_run(int32_t slot, openrac_game_moby_run* out);
/* moby_pool.c's last run list of the level's mobys: how it treated `slot` (OPENRAC_MOBY_RUN_*, -1
 * not looked at, or none built this level load), how many were built and its camera; world.c's
 * whether the level's mobys are the current context (not the pause menu's). */
int rac1_moby_run(int32_t slot, uint32_t* runs, float camera[3]);
int rac1_level_mobys_current(void);
int32_t rac1_world_moby_spawn(int32_t oclass);
int32_t rac1_world_hero_slot(void);
void rac1_world_stand_in(const float position[3], gaddr moby);
void rac1_world_post_hit(
    gaddr victim,
    gaddr attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
);
void rac1_world_eyes(const float* xyz, int count);
int32_t rac1_world_bolts(int32_t add);
void rac1_world_keep_running(int on);

/* play.c: whether `moby` is a figure made in this level load. figure_items.c: what a figure wears
 * and holds and the shots it fires (interface 1.9; play.c's table), its pieces put on its joints
 * after the moby loop, taken off with it, and whether a moby is a figure's (its shots: they hit
 * nothing here). crash_state.c: the moby whose update runs now, 0 for none. */
int rac1_figure_alive(gaddr moby);
int rac1_hero_held(openrac_game_held* out);
void rac1_figure_held(gaddr figure, const openrac_game_held* held);
int rac1_hero_shots(openrac_game_shot* out, int max);
int rac1_figure_shot(gaddr figure, const openrac_game_shot* shot);
void rac1_figure_items_tick(void);
void rac1_figure_items_remove(gaddr figure);
int rac1_figure_owned(gaddr moby);
void rac1_figure_hit_dropped(const char* what);
gaddr rac1_updating_moby(void);

/* scenes.c: the level's in-game scenes for the mods (interface 1.9; play.c's table): the one
 * playing and a mod's to start, around the moby loop (crash_state.c: the game mode as it begins; a
 * scene its mobys started, and a mod's started once the player is in control, after it); and
 * whether a moby is an actor of the scene playing (world.c's listing). */
int32_t rac1_scene_playing(int32_t* frame);
int rac1_scene_start(int32_t scene);
void rac1_scenes_loop_begin(void);
void rac1_scenes_tick(void);
int rac1_scene_actor(gaddr moby);

#endif /* OPENRAC_RAC1_H */
