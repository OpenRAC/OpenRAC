/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the level's in-game scenes for the mods (game_host.h, openrac_game_play's
 * scene_playing and scene_start; interface 1.9, multiplayer): the scene playing, and a scene
 * started as the level's own trigger starts it, so every game of a party plays the scenes the
 * host's plays.
 *
 * How the game starts one. StartCutscene (func_L00_00299B68, src/overlays/shared/
 * cutscene_and_movie_playback.c; the US disc's FUN_L00_00298840) takes the scene's number in the
 * level: it clears the scene's
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
#include <math.h>
#include <string.h>

#include "openrac/sound_engine.h"
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

/* Reward presentations do not grant items or write collection flags. */
static struct {
    int kind, ticks, gold_active, info_stage, info_movie;
    float info_time, info_angle, info_scale;
    uint32_t load;
    gaddr prop, held, hidden_hero;
    unsigned short held_flags, hero_flags;
    float home[3], yaw;
} pickup;

static gaddr infobot_camera_vectors;
#define PICKUP_COLLISION OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00173F40u))

static float infobot_clear_distance(const float from[3], const float to[3]) {
    const gaddr a = infobot_camera_vectors + 64, b = a + 16;
    memset(G(a), 0, 32);
    for (int axis = 0; axis < 3; ++axis) {
        GREF(float, a + 4 * axis) = from[axis];
        GREF(float, b + 4 * axis) = to[axis];
    }
    float length = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const float d = to[axis] - from[axis];
        length += d * d;
    }
    length = sqrtf(length);
    if (!func_L00_001EFFF0(a, b, 0x12, (int)GREF(gaddr, HERO_MOBY), 0)) {
        return length;
    }
    float hit = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const float d = GREF(float, PICKUP_COLLISION + 0x20 + axis * 4) - from[axis];
        hit += d * d;
    }
    return fminf(length, sqrtf(hit));
}

static void pickup_infobot_framing(void) {
    float facing = pickup.yaw, nearest = 100.0f;
    /* A vendor's hologram and display panels are not camera-collision geometry.
     * Frame from the player's side of the nearest vendor, outside those panels. */
    const gaddr pool = GREF(gaddr, OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00160098u)));
    for (gaddr m = pool; m && m < 0x02000000u - 0x100u; m += 0x100u) {
        if (GREF(uint8_t, m + 0x20) == 0xFF) {
            break;
        }
        if (GREF(int16_t, m + 0xA6) != 11 || GREF(uint8_t, m + 0x20) >= 0xFD) {
            continue;
        }
        const float dx = pickup.home[0] - GREF(float, m + 0x10),
                    dy = pickup.home[1] - GREF(float, m + 0x14);
        const float distance = dx * dx + dy * dy;
        if (distance < nearest && distance > 0.01f) {
            nearest = distance;
            facing = atan2f(dy, dx);
        }
    }
    pickup.info_scale = 1;
    float from[3] = {pickup.home[0], pickup.home[1], pickup.home[2] + 1.1f};
    for (int side = 0; side < 8; ++side) {
        const float angle = side * 0.7853982f;
        const float to[3] = {from[0] + 1.4f * cosf(angle), from[1] + 1.4f * sinf(angle), from[2]};
        pickup.info_scale =
            fminf(pickup.info_scale, (infobot_clear_distance(from, to) - 0.2f) / 1.4f);
    }
    const float top[3] = {from[0], from[1], pickup.home[2] + 3.8f};
    pickup.info_scale = fmaxf(
        0.45f, fminf(pickup.info_scale, (infobot_clear_distance(from, top) + 1.1f - 0.2f) / 3.8f)
    );
    const float offsets[] = {0, 0.7853982f, -0.7853982f, 1.5707964f, -1.5707964f, 3.1415927f};
    float best = -1;
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        const float angle = facing + offsets[i];
        const float target[3] =
            {pickup.home[0], pickup.home[1], pickup.home[2] + 1.1f * pickup.info_scale};
        const float eye[3] =
            {target[0] + 4.5f * pickup.info_scale * cosf(angle),
             target[1] + 4.5f * pickup.info_scale * sinf(angle),
             pickup.home[2] + 1.5f * pickup.info_scale};
        const float clearance = infobot_clear_distance(target, eye);
        if (clearance > best + 0.15f) {
            best = clearance;
            pickup.info_angle = angle;
        }
        if (clearance >= 4.5f * pickup.info_scale) {
            break;
        }
    }
}

static void pickup_infobot_camera(int reset) {
    if (!infobot_camera_vectors) {
        infobot_camera_vectors = openrac_guest_static(128);
    }
    const gaddr at = infobot_camera_vectors, rot = at + 16, eye = at + 32, look = at + 48;
    memset(G(at), 0, 64);
    for (int axis = 0; axis < 3; ++axis) {
        GREF(float, at + 4 * axis) = pickup.home[axis];
    }
    GREF(float, rot + 8) = pickup.yaw;
    if (reset && pickup.info_stage == 0) {
        pickup_infobot_framing();
    }
    if (reset) {
        func_L00_00217718(at, rot, 0x72, 0);
        if (pickup.kind == 3) {
            openrac_game_infobot_sound(pickup.info_stage == 0);
        }
    }
    if (pickup.hidden_hero) {
        GREF(uint16_t, pickup.hidden_hero + 0x34) |= 1;
    }
    const float t = fminf(pickup.info_time / 0.9f, 1);
    const float open = pickup.kind == 4         ? 0
                       : pickup.info_stage == 2 ? 1 - t * t * (3 - 2 * t)
                                                : t * t * (3 - 2 * t);
    const float distance = (4.5f + 0.5f * open) * pickup.info_scale;
    float target[3] =
        {pickup.home[0], pickup.home[1], pickup.home[2] + (1.0f + 0.3f * open) * pickup.info_scale};
    float wanted[3] =
        {target[0] + distance * cosf(pickup.info_angle),
         target[1] + distance * sinf(pickup.info_angle),
         pickup.home[2] + (1.0f + 0.5f * open) * pickup.info_scale};
    const float length =
        sqrtf(distance * distance + (wanted[2] - target[2]) * (wanted[2] - target[2]));
    const float safe = fmaxf(0.25f, infobot_clear_distance(target, wanted) - 0.3f);
    const float fraction = fminf(1, safe / fmaxf(length, 0.001f));
    for (int axis = 0; axis < 3; ++axis) {
        GREF(float, eye + axis * 4) = target[axis] + (wanted[axis] - target[axis]) * fraction;
    }
    GREF(float, look + 8) = pickup.info_angle + 3.1415927f;
    GREF(float, look + 4) = atan2f(wanted[2] - target[2], distance);
    if (reset) {
        func_L00_002EBF50(eye, look, 1, 0, 0);
    }
    func_L00_002EBE88(eye);
    func_L00_002EBEE0(look);
    GREF(float, OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F4FCu))) = 0;
}

int rac1_infobot_presentation(openrac_infobot_presentation* out) {
    if ((pickup.kind != 3 && pickup.kind != 4) || pickup.info_stage == 1 || !rac1_level_running()
        || pickup.load != rac1_moby_generation()) {
        return 0;
    }
    out->position[0] = pickup.home[0];
    out->position[1] = pickup.home[1];
    out->position[2] = pickup.home[2];
    out->scale = pickup.info_scale;
    out->special_item = pickup.kind == 4 ? pickup.info_movie : 0;
    out->time = pickup.info_time;
    out->yaw = pickup.yaw;
    if (pickup.kind == 4
        && (pickup.info_movie == 407 || pickup.info_movie == 37 || pickup.info_movie == 1002)) {
        out->yaw = pickup.info_angle;
    }
    const float t = fminf(pickup.info_time / 0.9f, 1);
    const float eased = t * t * (3 - 2 * t);
    out->openness = pickup.info_stage == 2 ? 1 - eased : eased;
    return 1;
}

int rac1_pickup_background(void) {
    return (pickup.kind == 3 || pickup.kind == 4) && pickup.info_stage != 1 && rac1_level_running()
           && pickup.load == rac1_moby_generation();
}

void func_L00_0029A300(void) {
    func_L00_0029A300__game();
    rac1_pickup_tick();
}

static void pickup_finish(void) {
    if (pickup.prop) {
        rac1_world_moby_kill(pickup.prop);
    }
    if (pickup.hidden_hero && GREF(gaddr, HERO_MOBY) == pickup.hidden_hero) {
        GREF(uint16_t, pickup.hidden_hero + 0x34) =
            (GREF(uint16_t, pickup.hidden_hero + 0x34) & ~1u) | (pickup.hero_flags & 1u);
    }
    if (pickup.held && GREF(gaddr, HERO + 0x1090u) == pickup.held) {
        GREF(unsigned short, pickup.held + 0x34) =
            (GREF(unsigned short, pickup.held + 0x34) & ~1u) | (pickup.held_flags & 1u);
    }
    if (pickup.kind == 1 || pickup.kind == 3 || pickup.kind == 4) {
        GREF(int32_t, OPENRAC_LDATA(1, RAC1_LADDR(01, 0x0015F504u))) = pickup.gold_active;
        func_L00_00217718(HERO + 0x80, HERO + 0x90, 0, 0);
        func_L00_002EC0C8(0);
    }
    if (GREF(int, LEVEL_MODE) == MODE_PLAY) {
        rac1_hero_place(pickup.home, pickup.yaw, 0);
    }
    func_0020BFC8(0, -1);
    openrac_music_hold(0);
    memset(&pickup, 0, sizeof pickup);
}

void rac1_pickup_tick(void) {
    if (pickup.kind && (!rac1_level_running() || pickup.load != rac1_moby_generation())) {
        openrac_music_hold(0);
        memset(&pickup, 0, sizeof pickup);
    }
    if (pickup.kind) {
        ++pickup.ticks;
        if (pickup.kind == 1) {
            const gaddr vars = GREF(gaddr, pickup.prop + 0x78);
            /* Let retail advance the prop, hero, particles and pickup camera.
             * Stop before its final accounting/save: AP already granted the reward. */
            if (GREF(int, HERO_HEALTH) <= 0 || pickup.ticks >= 900
                || (GREF(uint8_t, pickup.prop + 0x52) == 0 && GREF(int32_t, vars + 0x6C) <= 2)) {
                pickup_finish();
            }
        } else if (pickup.kind == 4 && GREF(int, LEVEL_MODE) == MODE_PLAY) {
            const float dt = GREF(float, RAC1_ADDR(0x0015EE6Cu));
            pickup.info_time += dt > 0 && dt < 0.25f ? dt : 1.0f / 60;
            pickup_infobot_camera(0);
            const float duration = pickup.info_movie == 1002   ? 4.8f
                                   : pickup.info_movie == 1428 ? 4.12f
                                   : pickup.info_movie == 407  ? 1.08f
                                   : pickup.info_movie == 37   ? .76f
                                   : pickup.info_movie == 645  ? 7.76f
                                                               : 7.8f;
            if (GREF(int, HERO_HEALTH) <= 0 || pickup.info_time >= duration) {
                pickup_finish();
            }
        } else if (pickup.kind == 3 && GREF(int, LEVEL_MODE) == MODE_PLAY) {
            if (GREF(int, HERO_HEALTH) <= 0) {
                pickup_finish();
                return;
            }
            if (pickup.info_stage == 1) {
                pickup.info_stage = 2;
                pickup.info_time = 0;
                pickup_infobot_camera(1);
            } else {
                const float dt = GREF(float, RAC1_ADDR(0x0015EE6Cu));
                pickup.info_time += dt > 0 && dt < 0.25f ? dt : 1.0f / 60;
                pickup_infobot_camera(0);
                if (pickup.info_stage == 0 && pickup.info_time >= 1.4f) {
                    pickup.info_stage = 1;
                    pickup.info_time = 0;
                    func_L00_0029A7D0(pickup.info_movie);
                } else if (pickup.info_stage == 2 && pickup.info_time >= 1.4f) {
                    pickup_finish();
                }
            }
        } else if (pickup.ticks > 1 && GREF(int, LEVEL_MODE) == MODE_PLAY) {
            pickup_finish();
        }
    }
}

int rac1_pickup_animation(int kind, int item) {
    if (kind < 0) {
        return pickup.kind != 0;
    }
    if (kind == 0) {
        if (rac1_level_running()) {
            func_0020BFC8(0, -1);
        }
        return 0;
    }
    if (kind < 1 || kind > 4) {
        return 0;
    }
    if (kind == 4 && item != 1002 && item != 1428 && item != 645 && item != 1433 && item != 407
        && item != 37) {
        return 0;
    }
    if (kind == 4 && !openrac_game_special_available(item)) {
        return 0;
    }
    if (pickup.kind || !rac1_level_running() || GREF(int, LEVEL_MODE) != MODE_PLAY) {
        return -1;
    }
    const gaddr vendor = OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001CA7C0u));
    if (kind == 2) {
        const gaddr demos = OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001CA320u));
        if (item < 2 || item >= 0x25 || GREF(int32_t, demos + 4 * item) < 0) {
            return 0;
        }
    }
    if (kind == 3 && (item < 2 || item > 18)) {
        return 0;
    }
    openrac_music_hold(1);
    pickup.kind = kind;
    pickup.load = rac1_moby_generation();
    for (int axis = 0; axis < 3; ++axis) {
        pickup.home[axis] = GREF(float, HERO + 0x80 + axis * 4);
    }
    pickup.yaw = GREF(float, HERO + 0x98);
    if (kind == 2) {
        /* The game's remote-vendor path initializes a transient vendor and its
         * HUD/resource lifetime. Go straight to the selected weapon's demo. */
        func_L00_0029BA20(0);
        const gaddr m = GREF(gaddr, vendor + 0x1C);
        /* This remote vendor is only the demo's transform anchor. Its normal
         * world update expects a placed vendor's beam resources and pvars. */
        GREF(gaddr, m + 0x74) = 0;
        GREF(float, m + 0x48) = pickup.yaw + 3.1415927f;
        func_L00_00251E30(m);
        const gaddr offset = OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001611B0u));
        for (int axis = 0; axis < 3; ++axis) {
            float local = 0;
            for (int col = 0; col < 3; ++col) {
                local +=
                    GREF(float, m + 0xC0 + col * 16 + axis * 4) * GREF(float, offset + col * 4);
            }
            GREF(float, m + 0x10 + axis * 4) = pickup.home[axis] - local;
        }
        GREF(int32_t, vendor + 0x48) = item;
        func_L00_0029C070();
    } else if (kind == 3 || kind == 4) {
        pickup.info_movie = item;
        pickup.hidden_hero = GREF(gaddr, HERO_MOBY);
        if (pickup.hidden_hero) {
            pickup.hero_flags = GREF(uint16_t, pickup.hidden_hero + 0x34);
        }
        pickup.gold_active = GREF(int32_t, OPENRAC_LDATA(1, RAC1_LADDR(01, 0x0015F504u)));
        GREF(int32_t, OPENRAC_LDATA(1, RAC1_LADDR(01, 0x0015F504u))) = 1;
        pickup.held = GREF(gaddr, HERO + 0x1090u);
        if (pickup.held) {
            pickup.held_flags = GREF(unsigned short, pickup.held + 0x34);
            GREF(unsigned short, pickup.held + 0x34) |= 1;
        }
        pickup_infobot_camera(1);
        if (kind == 4) {
            openrac_game_special_sound(item);
        }
    } else {
        pickup.prop = func_0020D348(1134);
        if (!pickup.prop) {
            openrac_music_hold(0);
            memset(&pickup, 0, sizeof pickup);
            return 0;
        }
        GREF(gaddr, pickup.prop + 0x74) = OPENRAC_LCODE(1, RAC1_LADDR(01, 0x00309078u));
        GREF(uint8_t, pickup.prop + 0x30) = 0xFF;
        GREF(int16_t, pickup.prop + 0x32) = 0x7FFF;
        const gaddr vars = GREF(gaddr, pickup.prop + 0x78);
        if (!vars) {
            pickup_finish();
            return 0;
        }
        memset(G(vars), 0, 0x70);
        /* A replacement pickup has no authored level camera path. Retail's
         * -1 sentinel selects the Gold Bolt orbit relative to Ratchet. */
        GREF(int32_t, vars + 4) = -1;
        GREF(int32_t, vars + 8) = -1;
        for (int axis = 0; axis < 3; ++axis) {
            GREF(float, pickup.prop + 0x10 + axis * 4) = pickup.home[axis];
        }
        GREF(float, pickup.prop + 0x48) = pickup.yaw;
        /* State zero only initializes the native prop. Borrow a valid index for
         * that check, restoring it immediately; the prop never owns a location. */
        const gaddr collected = RAC1_ADDR(0x0014BFC0u) + 4 * GREF(int32_t, RAC1_ADDR(0x0015EE84u));
        const uint8_t saved = GREF(uint8_t, collected);
        GREF(uint8_t, collected) = 0;
        GREF(uint8_t, pickup.prop + 0x20) = 0;
        func_L01_00309078__game(pickup.prop);
        GREF(uint8_t, collected) = saved;
        GREF(int32_t, vars) = -1;
        pickup.gold_active = GREF(int32_t, OPENRAC_LDATA(1, RAC1_LADDR(01, 0x0015F504u)));
        pickup.held = GREF(gaddr, HERO + 0x1090u);
        if (pickup.held) {
            pickup.held_flags = GREF(unsigned short, pickup.held + 0x34);
            GREF(unsigned short, pickup.held + 0x34) |= 1;
        }
        func_L01_00309078__game(pickup.prop);
    }
    return 1;
}
