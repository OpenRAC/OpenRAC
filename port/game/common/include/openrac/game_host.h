/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Between a game's program (main.cpp), the library replacements every game
 * shares (lib/) and what hostgen generates for each game (game_info.c,
 * libraries.c, port/tools/hostgen).
 *
 * The replacements are C, written against guest.h like the translated game:
 * their arguments are the game's values and addresses. They implement the
 * libraries' APIs as the games use them (libraries.tsv lists each one), from
 * what the game's own code shows and what is publicly known of each API;
 * nothing of Sony's is in them. Below them, the port never models the
 * console: a read is a read of the extracted disc image, a memory card is a
 * folder, a frame ends where the game waits for vsync, and the game's display
 * list goes to the port's renderer as data.
 */
#ifndef OPENRAC_GAME_HOST_H
#define OPENRAC_GAME_HOST_H

#include "openrac/guest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The game, from its hostgen.json (generated game_info.c) ---- */

typedef struct openrac_game_info {
    const char* id;     /* "rac1-pal": port/game/<id>, the program openrac-<id> */
    const char* title;  /* "Ratchet & Clank (PAL)" */
    const char* game;   /* "rac1": the extractor's and the launcher's name for it */
    const char* serial; /* "SCES_509.16": its executable on the disc */
    int frame_rate;     /* 50 (PAL) or 60 */
    const char* entry;  /* the game's main, or NULL while it is not known */
} openrac_game_info;

extern const openrac_game_info openrac_game;

/* Runs the game's entry (its main). Returns false if the game has none yet. */
int openrac_game_run(void);

/* The level program that is loaded, or OPENRAC_OVERLAY_EXE: which overlay a
 * code address means. Generated as "the executable" unless the game's
 * hostgen.json says the game provides it ("overlay_hook", in host/). */
int openrac_game_loaded_overlay(void);

/* Sets the level program back to `overlay` when a warm save-state is restored (mods.cpp). The game
 * with "overlay_hook" provides it beside openrac_game_loaded_overlay; otherwise it is generated as
 * doing nothing. */
void openrac_game_restore_overlay(int overlay);
size_t openrac_game_snapshot_extra_size(void);
int openrac_game_snapshot_extra_save(void* out, size_t capacity);
int openrac_game_snapshot_extra_load(const void* in, size_t size);
void openrac_game_clear_level_programs(void);

/* Every function by code address (generated functions.c). */
void openrac_game_register_functions(void);

/* ---- Set by the program before the game starts ---- */

/* The player's disc image, as the extractor kept it (iso_data/<game>/disc.iso).
 * The game reads it by sector. */
extern const char* openrac_game_disc_image;

/* A folder of edited levels (--mods), or NULL: its patches.tsv lists the disc
 * sectors that are read from files beside it instead (editor/pack.py writes
 * both). The disc image itself is never changed. */
extern const char* openrac_game_mods_dir;

/* An enabled map mod, or NULL for the campaign. The runtime checked map.json against this disc.

 * The map uses `level`'s program with its own data. Spawn is in game units, heading in radians.

 * Map sessions start a fresh game with memory cards disabled. */
struct openrac_map_config {
    const char* name;
    int level;
    float spawn[3];
    float heading;
    int skip_movies;
    int skip_cutscenes;
};
extern const struct openrac_map_config* openrac_game_map;

/* A registered ship destination, separate from the active map and its guest template ID. */
extern const struct openrac_map_config* openrac_game_destinations;
extern unsigned openrac_game_destination_count;
extern int
    openrac_game_active_destination; /* -1: the campaign; otherwise an index in that registry */
extern unsigned openrac_game_mods_generation;
void openrac_game_select_destination(int index);

/* The first memory card's folder (the second is beside it, <folder>.slot2). */
extern const char* openrac_game_card_dir;

/* The mods loaded for this run, as a JSON array for the save mark:
 * [{"id":"lock-strafe","version":"1.0.0"}]. "[]" when none loaded and in purist mode (--no-mods).
 * The mod loader sets it once, before the game runs (game/common/mods.cpp); never null after that,
 * and it does not change while the game runs. lib/mc.c writes it beside each save the game
 * writes, so the launcher and the game's own memory card page can say a save was played with mods
 * (launcher/core/src/saves.rs). */
extern const char* openrac_game_loaded_mods;

/* The console's language setting (sceScfGetLanguage): 1 English, 2 French,
 * 3 Spanish, 4 German, 5 Italian. */
extern int openrac_game_language;

/* An option the game keeps in its own memory, which a console forgets at power off (rac1: the
 * language and the Sound page), remembered between runs in the port's settings (key game_*): its
 * value, or `fallback` while none is remembered; and remember one (the file is written when it
 * changed). "game_language" is the console's language above, which the port starts with. */
int openrac_game_option(const char* key, int fallback);
void openrac_game_set_option(const char* key, int value);

/* ---- Provided by the program, called by the replacements ---- */

/* The game finished a frame and waits for vsync (sceGsSyncV): present what
 * the renderer has, pump input, keep the frame rate. Returns the field the
 * console would report (0 or 1, alternating). */
int openrac_game_vsync(void);

/* Called by the window's frame loop with the frame number, after each vertical blank, when a game
 * installs it (rac1: the direct start into a level, boot.c). */
extern void (*openrac_game_on_frame)(unsigned frame);

/* The player's bolts, when a game installs it (rac1/pal: boot.c): logged while a level plays for
 * the launcher's Discord presence. */
extern int (*openrac_game_bolts)(void);

/* The buttons a pressed Start also presses while Skip cutscenes is on (settings skip_cutscenes), when
 * a game installs it: what the game's own skip of the scene playing now takes, as the window's pad
 * bits (0x4000 Cross, 0x0F00 the four shoulders), or 0 while nothing skippable plays (rac1: boot.c,
 * the shoulders in a cutscene, Cross in the ship's landing and take-off). */
extern int (*openrac_game_skip_buttons)(void);

/* For crash reports (port/common/crash.h), sampled once a frame, when a game installs them (rac1:
 * boot.c): the game's mode word, where the hero is and where the camera is. Each returns 0 when it
 * has no answer now (no level loaded). */
extern int (*openrac_game_mode)(uint32_t* mode);
extern int (*openrac_game_hero_position)(float xyz[3]);
extern int (*openrac_game_camera_position)(float xyz[3]);

/* For crash reports: the moby whose update was running when the program stopped. */
typedef struct openrac_game_moby {
    uint32_t address; /* in game memory */
    int32_t oclass;   /* its class */
    float position[3];
    int32_t uid; /* its id in the level's placements, or -1 */
} openrac_game_moby;

/* Asked while the report is written, not each frame: from a signal handler, so it only reads game
 * memory it has checked is there, and allocates nothing. 1 and `out` filled when a moby's update
 * was running (rac1: boot.c's func_00213C78, the moby update loop), 0 otherwise. */
extern int (*openrac_game_updating_moby)(openrac_game_moby* out);

/* For OPENRAC_TRACE (frontend.cpp), when a game installs it (rac1: boot.c): the hero's position,
 * rotation (game axes; z the yaw) and state id. 0 when there is none now (no level loaded). */
extern int (*openrac_game_hero)(float position[3], float rotation[3], int32_t* state);

/* For the picture's late camera latch (settings responsive_controls, frontend/camera_latch.cpp),
 * when a game installs it (rac1: camera_latch.c): the turn its follow camera would make over the
 * next `ticks` ticks (a fraction too) with the right stick held from now on at `right_x` (the pad's
 * byte, 128 centred), in radians about `axis` (game axes, the way the game turns the camera's
 * offset from the hero). `reads` counts the camera's stick reads so far, so that a tick without
 * one shows. Returns 0, with no turn, while that camera does not steer the view, its stick turn
 * is locked or the stick is centred. It only reads the game's memory. */
extern int (*openrac_game_camera_turn)(
    uint8_t right_x, float ticks, float* radians, float axis[3], unsigned* reads
);

/* A trap the game raised on purpose, or another reason the game cannot go on: writes a crash
 * report (<data>/crashes) and ends the program. */
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void openrac_game_crash(const char* reason);

/* A warning from the game's host C, as one line of the port's log (and so of the session's file a
 * player sends): something the port did instead of what the console would have (a crash the
 * console shares, skipped). */
void openrac_game_warn(const char* message);

/* Called while Start is held with skip_cutscenes on, when a game installs it: skips what the game
 * lets the port skip besides its own cutscenes (rac1/pal: the flight between planets, boot.c). */
extern void (*openrac_game_skip)(void);

/* What the window's own menus and drawing need of a game's front end, when a game installs it
 * (rac1: port/game/rac1/host/front_end.c, with each disc's addresses). NULL: the window adds
 * nothing to the game's menus. */
typedef struct openrac_front_end {
    /* The developer menu (debug_menu.cpp): the levels, by the game's own numbers, and a new game
     * started straight in one of them, as --level N with OPENRAC_DIRECT does. */
    int levels;
    const char* const* level_names;
    void (*start_level)(int level);
    /* The menus' sound for a widget (rac1: the confirm sound, 0x11, from the widget's moby). */
    void (*menu_sound)(int sound, gaddr moby);
    /* The front end's page menu, data in the boot program (rac1: debug_menu.cpp describes it). */
    gaddr page_menu;      /* +4 the page shown */
    gaddr options_list;   /* the Options list widget: +0x34 its items, 12 bytes each */
    gaddr options_items;  /* the items the game gives it, ended by one without a label */
    gaddr video_item;     /* the item that opens the Video page, added where the game's lack it */
    gaddr language_page, language_title, language_list; /* what the Planets page is copied from */
    int16_t planets_text;                                /* "Planets" */
    const int16_t* planet_texts;                         /* each level's planet name, by level */
    gaddr video_page, video_list, video_rows; /* the Video options page, its list, its own rows */
    gaddr sound_hint;                         /* the Sound options page's hint widget */
    int32_t sound_list_seq, sound_hint_seq;   /* that page's frame animations */
    gaddr pad;        /* the first pad: +0x1C4 the buttons pressed this tick, +0x100 the sticks */
    gaddr video_mode; /* a byte: the game's own 50/60 Hz choice, kept at 60 Hz; 0 for none */
    /* The window (frontend.cpp): words of the boot program and the executable. */
    gaddr game_mode;  /* the game mode; 6 in the boot program is the flight between planets */
    gaddr light_bank; /* the directional light bank (relocated to the loaded program's copy) */
    gaddr level;      /* the level being played, or -1 */
    gaddr dialog;     /* the front end's dialog: +0 its kind, +0x1C its step (logged) */
    gaddr title_exit; /* the title loop's exit (logged) */
    gaddr ship_state; /* the ship's sequence word: 4 the flight through space (the flight world),
                         others drawn on the planet (0 take-off, 8 landing, 3 and 7 leaving it);
                         0 for a game without one */
    /* Writes the hero body first, then currently attached equipment mobys, up to capacity.
     * Returns the count written; zero or NULL means no readable shadow rig this frame. */
    unsigned (*hero_shadow_casters)(gaddr* addresses, unsigned capacity);
    /* A word: the game's language (rac1: 0 English, 2 French, 3 German, 4 Spanish, 5 Italian), for
     * the port's own texts in its menus (options_menu.cpp); 0 for none. */
    gaddr language;
    /* The mods' pages of the game's own menus (mod_ui.cpp, interface 1.7): the title's main menu
     * page, the Options page (whose frames a first-level page has), the memory card's prompt
     * widget (its drawing, for a question), and the 14 frame mobys' slots. */
    gaddr main_page, options_page, prompt, panel_slots;
    /* One of the menus' sounds on a widget's moby (rac1: 0 confirm, 1 cursor, 2 denied, 3 open,
     * 4 page change, the frame moby class's table). */
    void (*menu_cue)(int cue, gaddr moby);
    /* Frame moby `slot` cut to frame 0 of its seq `seq` (`last`: its last frame), then played at
     * `speed` frames a tick (1 on, -1 back), as the page menu's switch does. */
    void (*panel_animation)(int slot, int seq, int last, float speed);
    /* The fonts' glyph tables, normal, small and large (rac1: 4 bytes a character, its width the
     * signed byte 3, which the game's string width sums), for the width of a mod's page; 0 for
     * none. */
    gaddr fonts[3];
} openrac_front_end;

extern const openrac_front_end* openrac_game_front_end;

/* What the mods' play calls need of a game (sdk/include/openrac/openrac_mod.h, interface 1.8:
 * multiplayer), when a game installs it (rac1: port/game/rac1/host/play.c, with each disc's
 * addresses). NULL: the calls answer that the game has none. Everything on the game's thread. */

/* What the game is doing: openrac_mod.h's OPENRAC_PLAY_* values (mods.cpp checks they agree). */
#define OPENRAC_GAME_PHASE_NONE 0
#define OPENRAC_GAME_PHASE_FRONT_END 1
#define OPENRAC_GAME_PHASE_LOADING 2
#define OPENRAC_GAME_PHASE_PLAYING 3
#define OPENRAC_GAME_PHASE_SCENE 4
#define OPENRAC_GAME_PHASE_MENU 5

/* A shared progress section: openrac_mod.h's OPENRAC_SHARED_* values (mod_play.cpp checks they agree). */
#define OPENRAC_GAME_SHARED_GOLD_WEAPONS 1
#define OPENRAC_GAME_SHARED_SKILL_POINTS 2
#define OPENRAC_GAME_SHARED_LEVEL_FLAGS 3
#define OPENRAC_GAME_SHARED_SPECIAL_ITEMS 4

/* A pose of the hero or of a figure: openrac_pose's fields after its size, the same meanings. */
typedef struct openrac_game_pose {
    int32_t level;
    float position[3];
    float rotation[3];
    float scale;
    float velocity[3];
    int32_t state;
    int32_t health;
    int32_t item;
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
    uint32_t flags;
} openrac_game_pose;

/* A moby of the level as the world calls see it: openrac_moby's fields after its size. */
typedef struct openrac_game_moby_state {
    int32_t slot;
    int32_t key;
    int32_t oclass;
    uint32_t flags;
    uint32_t state;
    float position[3];
    float rotation[3];
    float scale;
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
} openrac_game_moby_state;

/* How a moby of the level was deleted (interface 1.9): openrac_moby_deletion's fields after its
 * size. */
typedef struct openrac_game_moby_deletion {
    int32_t by;
    int32_t by_class;
    int32_t by_mod;
    int32_t oclass;
    uint32_t loads;
} openrac_game_moby_deletion;

/* How the game ran the level's mobys in the last tick, and a moby in it (interface 1.9):
 * openrac_moby_run's fields after its size. */
typedef struct openrac_game_moby_run {
    uint32_t runs;
    float camera[3];
    uint32_t menu_ticks;
    int32_t menu;
    float level_camera[3];
    int32_t level_mode;
    int32_t context;
    int32_t update_dist;
    int32_t draw_dist;
    int32_t drawn;
    int32_t ran;
    float distance;
} openrac_game_moby_run;

/* What a body wears and holds, and a shot (interface 1.9): openrac_held's and openrac_shot's
 * fields after their size. */
#define OPENRAC_GAME_HELD_MAX 8

typedef struct openrac_game_held_piece {
    int32_t slot;
    int32_t part;
    int32_t item;
    int32_t oclass;
    int32_t joint;
    uint32_t flags;
    float position[3];
    float rotation[3];
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
} openrac_game_held_piece;

typedef struct openrac_game_held {
    int32_t count;
    openrac_game_held_piece pieces[OPENRAC_GAME_HELD_MAX];
} openrac_game_held;

typedef struct openrac_game_shot {
    int32_t item;
    float position[3];
    float velocity[3];
    float aim[4];
} openrac_game_shot;

/* A shot with more to say (openrac_shot2, after its size). A weapon that is no single shot is a
 * track: the flags say where in it this one is, aux[0] which of the player's tracks it is. */
#define OPENRAC_GAME_SHOT_START 0x1u
#define OPENRAC_GAME_SHOT_MOVE 0x2u
#define OPENRAC_GAME_SHOT_STOP 0x4u
#define OPENRAC_GAME_SHOT_BURST 0x8u

typedef struct openrac_game_shot2 {
    int32_t item;
    float position[3];
    float velocity[3];
    float aim[4];
    uint32_t flags;
    int32_t target_key;
    float velocity2[3];
    float aux[4];
} openrac_game_shot2;

typedef struct openrac_game_play {
    /* The levels, by the game's own numbers, and their names. */
    int levels;
    const char* const* level_names;
    /* The phase now (OPENRAC_GAME_PHASE_*), the level played or loading (-1 none), and the level
     * programs loaded since the start (a new count: the last level's mobys are gone). */
    int (*phase)(int32_t* level);
    uint32_t (*loads)(void);
    /* The hero's pose as he is drawn: 1 and `out` filled, 0 with no hero now. */
    int (*hero_pose)(openrac_game_pose* out);
    /* A figure: a moby that looks like the hero and runs none of his logic, made in the level
     * loaded (0: no room); whether `moby` is still the one made (the level's program not loaded
     * again since, the moby not reused); put in a pose; taken out. */
    gaddr (*figure_spawn)(void);
    int (*figure_alive)(gaddr moby);
    void (*figure_pose)(gaddr moby, const openrac_game_pose* pose);
    void (*figure_remove)(gaddr moby);
    /* From the front end: a new game in `level` (`save` null), or the `size` bytes of a save
     * restored and continued in its level (or in `level` when 0 or more). 1 started, 0 refused (a
     * save the game does not take). */
    int (*start)(int level, const uint8_t* save, uint32_t size);
    /* In a level: leaves it for `level`. 1 on the way, 0 refused. */
    int (*travel)(int level);
    /* The progress now as a save (the memory card file's bytes) into `out` when it has `size`
     * bytes; returns the save's size, 0 for none. */
    uint32_t (*save_capture)(uint8_t* out, uint32_t size);
    /* The memory card's saves: the folder in the card's (openrac_game_card_dir) and the file name
     * of save `slot` in it (printf's %d), how many slots there are, and what a save's bytes hold:
     * 1 and the fields for one the game would load, 0 otherwise. */
    const char* save_folder;
    const char* save_file;
    int save_slots;
    int (*save_info)(
        const uint8_t* save,
        uint32_t size,
        int32_t* level,
        int32_t* bolts,
        int32_t* gold_bolts,
        int32_t* seconds
    );

    /* The world (interface 1.9): the level's mobys by slot (their index in its array). */
    /* The live ones into `out` (at most `max`), how many there are; the slot of a placed moby by
     * its key (its record in the level's file: the same on every machine), -1 for none. */
    int (*mobys)(openrac_game_moby_state* out, int max);
    int32_t (*moby_slot)(int32_t key);
    /* A live moby's address by its slot, 0 for none; one put in a pose, deleted, made (of a class
     * the level has: its slot, -1 for none). */
    gaddr (*moby_at)(int32_t slot);
    int32_t (*slot_of)(gaddr moby);
    void (*moby_pose)(gaddr moby, const openrac_game_pose* pose);
    void (*moby_kill)(gaddr moby);
    int32_t (*moby_spawn)(int32_t oclass);
    /* The hero's moby's slot, -1 for none. */
    int32_t (*hero_slot)(void);
    /* While a moby's update runs (a gate's call before it): the hero is at `position` for it, his
     * moby the one at `moby` (0: his own); undone after the update. */
    void (*stand_in)(const float position[3], gaddr moby);
    /* A hit posted to `victim` (the game's message: MobyGetHitMessage reads it), from `attacker`
     * (0: none), of the weapon or attack `mask`, `damage`, at `position` moving at `velocity`. */
    void (*post_hit)(
        gaddr victim,
        gaddr attacker,
        uint32_t mask,
        float damage,
        const float position[3],
        const float velocity[3]
    );
    /* Up to 8 points besides the camera that keep the mobys near them running (each moby's update
     * distance), until set again. */
    void (*eyes)(const float* xyz, int count);
    /* The player's bolts after adding `add`. */
    int32_t (*bolts)(int32_t add);
    /* The level's mobys kept running under the game's menus (1) or not (0). */
    void (*keep_running)(int on);
    /* What the hero wears and holds (1 and `out` filled, 0 with no hero); a figure made to wear and
     * hold `held` (each tick it is posed: what is left out comes off); the shots the hero fired
     * since the last call (at most `max` into `out`, how many); a figure firing `shot`, hitting
     * nothing (1, or 0 for a weapon it cannot fire). NULL for a game without them. */
    int (*hero_held)(openrac_game_held* out);
    void (*figure_held)(gaddr figure, const openrac_game_held* held);
    int (*hero_shots)(openrac_game_shot* out, int max);
    int (*figure_shot)(gaddr figure, const openrac_game_shot* shot);
    /* The level's scenes (interface 1.9: openrac_api.scene_playing, play_scene): the one playing
     * (its number in the level, -1 for none) and its frames played into `frame`; a scene to start
     * as the level's own trigger does, once the player is in control (-1 drops one waiting): 1
     * asked, 0 refused (no level). NULL for a game without them. */
    int32_t (*scene_playing)(int32_t* frame);
    int (*scene_start)(int32_t scene);
    /* A moby hidden from drawing (1, as the game hides one) or shown (0) (interface 1.9:
     * openrac_api.moby_hide); the last deletion the game made of the moby in `slot` this level
     * load into `out` (1, or 0 for none seen: openrac_api.moby_deletion). NULL for a game without
     * them. */
    void (*moby_hide)(gaddr moby, int hidden);
    int (*moby_deletion)(int32_t slot, openrac_game_moby_deletion* out);
    /* Where the game's camera is into `out` (1, or 0 outside a level: openrac_api.camera_position).
     * NULL for a game without it. */
    int (*camera_position)(float out[3]);
    /* How the last tick ran the level's mobys and the moby in `slot` (-1 none) into `out` (1, or 0
     * outside a level: openrac_api.moby_run). NULL for a game without it. */
    int (*moby_run)(int32_t slot, openrac_game_moby_run* out);
    /* hero_shots and figure_shot with more to say (openrac_api.hero_shots2, figure_shot2). NULL for
     * a game without them. */
    int (*hero_shots2)(openrac_game_shot2* out, int max);
    int (*figure_shot2)(gaddr figure, const openrac_game_shot2* shot);
    /* The world this game's discs belong to (openrac_api.world_family): two with the same text
     * can share a game. NULL for a game that does not say. */
    const char* world_family;
    /* The sim digest of `level` as loaded (openrac_api.sim_digest): two games with the same one
     * can share that level; 0 for a level not loaded or not known. NULL for a game without it. */
    uint32_t (*sim_digest)(int level);
    /* A party's game saves as a game loaded from the card does, whatever the title did (`on` 1,
     * asked each frame while it lasts), and the card's state is put back after (`on` 0: 1 when
     * done, 0 while a write still runs: ask again). NULL for a game without it. */
    int (*card_session)(int on);
    /* Whether a save's bytes can be handed to the game, whole, for saves from elsewhere: save_info
     * reads only the head (for rac1 the SDK's openrac/games/rac1_save.h). NULL for a game without
     * it. */
    int (*save_check)(const uint8_t* save, uint32_t size);
    /* The card filter, for the mods' calls: openrac_game_card_filter and its fellows (lib/mc.c).
     * NULL for a game without it. */
    int (*card_filter)(int mode, const char* folder);
    void (*card_slot)(int slot);
    int (*card_write)(int slot, const uint8_t* bytes, uint32_t size);
    uint32_t (*card_saves)(void);
    /* Whether the world is drawn under the pause menu while it keeps running (1, the start) or the
     * picture stands (0): openrac_api.play_live_picture. NULL for a game without it. */
    void (*live_picture)(int on);
    /* The hero put at `position` facing `yaw` with OPENRAC_PLACE_* `flags` once the player is in
     * control (openrac_api.hero_place): 1 asked, 0 refused (no level, no hero). NULL for a game
     * without it. */
    int (*hero_place)(const float position[3], float yaw, uint32_t flags);
    /* The shared progress (openrac_api.shared_read, shared_give, mission_state_read and write;
     * rac1/host/shared.c): a section's size or -1, 1 merged or 0, 1 read or 0, 1 merged or 0. NULL
     * for a game without it. */
    int (*shared_read)(int tag, void* out, uint32_t size);
    int (*shared_give)(int tag, const void* data, uint32_t size);
    int (*mission_state_read)(int level, uint8_t* out);
    int (*mission_state_write)(int level, const uint8_t* in);
    /* A dead hero kept down, brought back or sent to his checkpoint (openrac_api.hero_revive and
     * its OPENRAC_REVIVE_* flags): 1 asked, 0 refused. NULL for a game without it. */
    int (*hero_revive)(const float position[3], uint32_t flags);
    /* The mission a moby of the level goes with, -1 for none. NULL for a game without it. */
    int32_t (*moby_mission)(int32_t slot);
    /* The game's camera scripts (openrac_api.on_camera_script and its fellows; rac1/host/
     * camera_script.c): the next start or end not yet told (1, or 0 for none), the view of the one
     * running (1, or 0 for none), one shown here (id >= 0; OPENRAC_GAME_CAMERA_END_EASED or _CUT
     * ends it; 1 asked, 0 refused) and its view (1 while it is asked or up). NULL for a game without
     * them. */
    int (*camera_script_next)(int32_t* id, int32_t* started);
    int (*camera_script_view)(float eye[3], float rotation[3], float where[3], int32_t* stand_in);
    int (*camera_script_play)(int32_t id);
    int (*camera_script_show)(const float eye[3], const float rotation[3]);
    /* stand_in, and the update also sees the player's hero `state`, his `health` and
     * OPENRAC_STAND_IN_* `flags` (openrac_api.hero_stand_in_as). NULL for a game without it. */
    void (*stand_in_as)(
        const float position[3], gaddr moby, int32_t state, int32_t health, uint32_t flags
    );
    /* In the gate's call after an update under a stand-in, what it did to the hero: 1, or 0 with no
     * stand-in on. NULL for a game without it. */
    int (*stand_in_read)(
        float position[3], float* yaw, int32_t* state, int32_t* health, uint32_t* flags
    );
    /* The hero held as a grab holds him, on his way to the place asked, and let go (openrac_api
     * hero_grab, hero_hold_at, hero_release): 1 asked, 0 refused. NULL for a game without it. */
    int (*hero_grab)(int animation, uint32_t flags);
    int (*hero_hold_at)(const float position[3], float yaw);
    int (*hero_let_go)(void);
} openrac_game_play;

#define OPENRAC_GAME_CAMERA_END_EASED (-1)
#define OPENRAC_GAME_CAMERA_END_CUT (-2)

extern const openrac_game_play* openrac_game_play_host;

/* Set by the mods' loader (interface 1.9), called by the game: before (phase 0) and after (phase
 * 1) a moby's update in the moby loop, by its slot; 1 before skips the update. And a hit posted to a
 * moby (victim and attacker by slot, -1 for none): 1 takes it back, as if never posted. NULL: no
 * mod asked. */
extern int (*openrac_game_moby_update)(int32_t slot, int phase);
/* The metal detector is about to give a buried moby's bolts (rac1/host/metal_detect.c): 0 holds it
 * back. NULL while no mod asked. */
extern int (*openrac_game_metal_detect)(int32_t slot, int32_t key);
extern int (*openrac_game_moby_hit)(
    int32_t victim,
    int32_t attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
);

/* Host rendering and menus use the game's descriptor, not a particular disc's level count.
 * These are program/template IDs; a named map still runs one of those programs. */
static inline int openrac_front_end_has_level(const openrac_front_end* front_end, int level) {
    return front_end != NULL && level >= 0 && level < front_end->levels;
}

/* Whether the running game's descriptor contains this template program. */
static inline int openrac_game_has_level(int level) {
    return openrac_front_end_has_level(openrac_game_front_end, level);
}

/* The active map's name, or the descriptor's label; no duplicate host name table. */
static inline const char* openrac_game_level_name(int level) {
    if (!openrac_game_has_level(level)) {
        return "boot";
    }
    if (openrac_game_map != NULL && level == openrac_game_map->level) {
        return openrac_game_map->name;
    }
    return openrac_game_front_end->level_names != NULL ? openrac_game_front_end->level_names[level]
                                                       : "unnamed level";
}

/* The game sends a DMA chain (sceDmaSend): channel is the game address of
 * the channel's registers (0x10009000 is VIF1, the display list; 0x1000A000
 * is the GIF), tag the address of the chain's first tag. The renderer reads
 * it as data, the way OpenGOAL reads Jak's DMA chains. */
void openrac_game_dma_send(gaddr channel, gaddr tag);

/* The display and drawing environment, decoded from the GS register values
 * the game puts (sceGsPutDispEnv, sceGsPutDrawEnv). */
typedef struct openrac_game_display {
    int width, height; /* the frame buffer shown, in pixels */
    int frame_base;    /* its address in the GS's memory, in 2048-word pages */
    int frame_width;   /* in 64-pixel units */
    int psm;           /* its pixel format */
    int x, y;          /* where the console put it on the screen */
} openrac_game_display;

void openrac_game_set_display(const openrac_game_display* display);
void openrac_game_set_video_mode(int interlace, int mode, int field_mode);

/* An image the game uploads to the GS's memory (sceGsExecLoadImage): its
 * destination and the game address of its pixels. The renderer turns it
 * into a texture (port/renderer, TexturePool); nothing models GS memory. */
typedef struct openrac_game_image {
    int base, width_units,
        psm; /* destination: base (64-word blocks), width (64-pixel units), format */
    int x, y, width, height;
    gaddr pixels;
} openrac_game_image;

void openrac_game_load_image(const openrac_game_image* image);

/* The game reading a rectangle of the GS's memory back (sceGsExecStoreImage):
 * the source, and the game address the pixels go to. The renderer answers
 * reads of the frame buffer with the frame it drew. */
void openrac_game_store_image(const openrac_game_image* image);

/* The game's own renderers were asked to draw these layers this frame (bits: 1 sky, 2 terrain,
 * 4 ties, 8 shrubs, 16 mobys; renderer::Bucket's order). Its renderer entry points call this in
 * the port, where they hand over to the port's renderers: the window draws a layer only in a
 * frame the game drew it (no world behind a loading card or a menu over black). A game that never
 * calls it gets its world drawn whenever it has a camera. */
#define OPENRAC_DRAW_SKY 1u
#define OPENRAC_DRAW_TERRAIN 2u
#define OPENRAC_DRAW_TIES 4u
#define OPENRAC_DRAW_SHRUBS 8u
#define OPENRAC_DRAW_MOBYS 16u
void openrac_game_draw(unsigned layers);
/* The world drawn next this frame (its terrain, openrac_game_draw) is a level's whose mobys a menu's
 * own moby context hides (Ratchet & Clank's pause menu while a party plays): the window takes the
 * mobys listed when the terrain is drawn for the frame's world, and draws the menu's picture of the
 * frame (sceGsExecStoreImage, read back once as it opens) from that live world. All of the menu's
 * 2D goes over that world, and the mobys it draws with its own camera (its frame objects) are drawn
 * on the screen at their place in its 2D, with that camera, after the world's depth is cleared. */
void openrac_game_world_under_menu(void);
/* Rendering notification only: an explicit gameplay write to a moby's RGB. */
void openrac_game_moby_colour(gaddr object, unsigned colour);

/* The moby at `moby`'s current pose, for the game's own use: each joint marked in `marks` (128
 * bytes: nonzero for a joint to evaluate, byte 0x7F the joint count) gets its pose matrix, four rows
 * of four floats, in the scratchpad at 0x70000000 + 0x40 * joint, as the game's hand-written chain
 * evaluator leaves it. Nothing without the window's evaluator. */
void openrac_game_moby_chain(gaddr moby, gaddr marks);

/* The game's moby renderer was asked to draw `count` mobys from `first` (0x100 bytes each; count
 * -1: the whole list, the world's draw). A list drawn on its own (the page menu's frame objects)
 * may be drawn with a camera of its own: the window draws those mobys as that camera saw them.
 * Nonzero: the caller puts a moby marker in the 2D list where the game's packets would have gone
 * (the renderer's kMobyMarker), so that mobys drawn into a texture are drawn there in order. */
int openrac_game_mobys_drawn(gaddr first, int count);

/* A textured quad the game draws in the world (rac1: func_001F7EF8, the draw callbacks' quad
 * routine): its corners in world space (x, y, z, w), the strip 0 1 2, 1 2 3; RGBA bytes and
 * normalised ST per corner; the GS registers CLAMP_1, TEX0_1, TEX1_1 and ALPHA_1 it draws with;
 * and, for an effect texture, where its pixels are.
 * The window draws the frame's quads after the mobys (renderer/effects.h). */
typedef struct openrac_game_quad {
    float corner[4][4];
    uint32_t rgba[4];
    float st[4][2];
    uint64_t clamp, tex0, tex1, alpha;
    /* Where the texture's pixels (PSMT8) and CLUT (16 x 16 PSMCT32) are in the game's memory, as
     * its effect texture paging sends them to TEX0's blocks; 0 when the texture is already in the
     * GS's memory some other way. */
    uint32_t pixels, clut;
    /* Where those two were found: 1 this frame's paging records, 2 the sources the same TEX0 had
     * when it was last paged in, 0 nowhere. */
    uint32_t source;
} openrac_game_quad;

void openrac_game_effect_quad(const openrac_game_quad* quad);

/* A sky shell the game drew this frame (rac1: SkyDrawShell) and the matrix it was drawn with: 16
 * floats, the game's rows (x, y, z axes, then the translation, in the sky's own units), each row
 * one column of the window's column-major matrix. */
void openrac_game_sky_shell(int shell, const float* matrix);

/* A textured quad of the 2D layer (rac1: DrawTexturedQuad, the HUD's sprite routines): four corners
 * in strip order (0-1-2, 1-2-3) in the chip's 12.4 screen coordinates as the game computes them,
 * their texel coordinates (12.4 both ways), the depth, the colour (RGBAQ's low word) and the
 * texture (TEX0). The window draws it natively: the index to put in the display list's marker, or
 * -1 to send the game's own packet (no window, or the 2D layer replayed). */
typedef struct openrac_game_ui_rect {
    uint32_t x[4], y[4];
    uint32_t u[4], v[4];
    uint32_t z;
    uint32_t rgba;
    uint64_t tex0;
    uint32_t flags; /* 1: a glyph of text; 2: sent as a sprite (corners 0 and 3) */
} openrac_game_ui_rect;

int openrac_game_ui_quad(const openrac_game_ui_rect* quad);

/* How far, in the game's screen pixels (512 across its 4:3 picture), the HUD's elements anchored to
 * a side of the screen are moved out past that side: the width a wider picture gained there when
 * the player's HUD setting is "Edges", else 0. */
int openrac_game_hud_side(void);

/* How many times wider than the game's 4:3 the picture is (the player's aspect setting): its
 * horizontal tangent is the game's times this. 1 for a 4:3 picture, and without a window. */
float openrac_game_picture_wider(void);

/* A game fix's flag, by its setting key (port/game/<game>/differences.json, kind "fix";
 * game/common/fixes.h): nonzero while the port's fix is on, 0 for the console's behaviour. The
 * pointer stays good for the program's life and follows the player's setting, so the code a fix
 * changes asks once and reads it from then on:
 *
 *     static const unsigned char* fix;
 *     if (!fix) fix = openrac_game_fix("fix_wide_liquids");
 *     if (*fix) ...
 *
 * A key this disc's list does not have reads as always on (and is logged). */
const unsigned char* openrac_game_fix(const char* key);

/* A live particle the game's particle renderer would draw (rac1: PartProc): its 0x40-byte record
 * (+0x01 bits 0-1 the render kind: 0 camera-facing sprite, 1 flat quad, 2 line, 3 ribbon; +0x03
 * ALPHA_1's low byte; +0x04 RGBA; +0x08 rotation, 256 steps a turn; +0x09 near (low nibble, 1/4
 * units) and far (high nibble, 32 units); +0x0C size in 1/210000 units, or the second colour of a
 * line or ribbon; +0x10 position, +0x1C a ribbon's half width; +0x20 a line or ribbon's other end,
 * +0x2C its width factor) and where its texture is in the game's memory (32 x 32 PSMT8 pixels,
 * 16 x 16 PSMCT32 CLUT; 0 for none). The window draws the frame's particles after the effect quads,
 * back to front (renderer/effects.h). */
void openrac_game_particle(const uint8_t* record, uint32_t pixels, uint32_t clut, int log2_side);

/* A sky sprite (rac1: SkySpriteProc, the stars and glows between the sky shells): its 0x20-byte
 * record (+0x03 ALPHA_1's low byte, +0x04 RGBA, +0x08 rotation in radians, +0x10 position around
 * the eye, +0x1C size) and the TEX0 of its sky texture. The window draws them behind the world. */
void openrac_game_sky_sprite(const uint8_t* record, uint64_t tex0);

/* Plays a PSS movie from the disc in the window, blocking as the game's own player does: `bytes`
 * bytes at sector `lsn`, the ADPCM channel `channel` (the language; channel 0 when the file has no
 * such channel). Start skips it when `start_skips` (the console's readMpeg rule for the caller).
 * Returns 1 if skipped, 0 if played to the end or when there is no window to play it in. */
int openrac_game_play_movie(uint32_t lsn, uint32_t bytes, int channel, int start_skips);

/* The buttons of a pad, in the console's bit order (active low, as the pad
 * library reports them) and its analog values; false if none is connected. */
/* Ratchet & Clank's blend snapshot: moby `moby`'s current pose (its keys A and B and the blend
 * between them) written at `dst` as a keyframe, as the game's VU0 routine leaves it in a blend slot.
 * Nonzero when written. */
int openrac_game_moby_snapshot(gaddr moby, gaddr dst);

/* Nonzero: no memory card is inserted in either port (--no-card). */
extern int openrac_game_no_card;

/* Where the game's memory card library keeps the command sceMcSync reports (a word, 0 while none has
 * finished) and its result (a word), for the port's (lib/mc.c): its commands finish at once and leave
 * both there, as the console's library leaves them for its sceMcSync, so that the game's memory holds
 * the library's state as on the console. Called by the game's host C at start-up (rac1/pal: boot.c,
 * the addresses in rac1_version.h); without a call they are lib/mc.c's own. */
void openrac_game_mc_state(gaddr command, gaddr result);

/* The card filter (lib/mc.c says how it works): what the game's writes to its saves do while a
 * party plays: 0 reach the card, 1 are dropped, 2 go to the card folder `folder`, into the slot
 * openrac_game_card_slot named (-1: the game's). The game is told they worked. 0 for a mode or
 * folder that cannot be. */
int openrac_game_card_filter(int mode, const char* folder);
void openrac_game_card_slot(int slot);
/* Writes a whole save as `slot` of the folder redirected to; 1 when it is in place. */
int openrac_game_card_write(int slot, const uint8_t* bytes, uint32_t size);
/* How many of the game's own saves have reached that folder. */
uint32_t openrac_game_card_saves(void);

/* --level N: the level a new game starts in (the game's own number), or -1 for the game's own
 * first level. For getting to a level directly, as the behaviour checker's level write does. */
extern int openrac_game_start_level;

int openrac_game_pad(int port, uint16_t* buttons, uint8_t analog[4]);

/* The mods' pad callbacks (game/common/mods.h, docs/port/MODS.md): each report scePad2Read fills,
 * `size` bytes at `report`, before the game reads it. Null while no mod asked for one. */
extern void (*openrac_game_pad_report_hook)(int port, uint8_t* report, int size);
/* After the mods changed a report (openrac_game_pad_report_hook): what the port does with the
 * buttons as they leave it (Start's skip), on the report's buttons and pressures. */
void openrac_game_pad_after_mods(int port, uint8_t* report, int size);
/* openrac_game_pad, then through the mods' callbacks as a report and back: for the port's own
 * readers of the pad (the movie player). */
int openrac_game_pad_through_mods(int port, uint16_t* buttons, uint8_t analog[4]);

/* The game's text lookup, asked for a text id: the game address of a string of the port's own (the
 * rows it adds to the game's options), or 0 when the id is the game's. */
gaddr openrac_game_text(int id);

/* Nonzero only while the enhanced Novalis water replacement is ready and enabled. */
int openrac_game_enhanced_water(void);

/* ---- Developer tools ---- */

/* What the game's host C keeps for its profile's diagnostics (GameProfile::diagnostics, which the
 * window hands it), or NULL while it keeps nothing. rac1: host/collision_notes.h's watch. */
extern void* openrac_game_diagnostics;

/* ---- Inside the replacements ---- */

/* Interrupt handlers the game installed (AddIntcHandler, sceGsSyncVCallback,
 * AddDmacHandler): called by the replacements where the console would raise
 * the interrupt. */
void openrac_game_run_vsync_handlers(void);
/* After those handlers: the mods' tick boundary (openrac_api.on_tick_boundary), where a warm
 * save-state may be taken or restored. */
void openrac_game_tick_boundary(void);
void openrac_game_run_dmac_handlers(int channel);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAME_HOST_H */
