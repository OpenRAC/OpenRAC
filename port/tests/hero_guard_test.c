/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* What a moby's update does to the hero under a stand-in, and this game's hero held by a mod
 * (rac1/host/hero_grab.c, PAL's table), on made-up game memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "../game/rac1/host/hero_grab.c"

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE;
const struct openrac_map_config* openrac_game_map;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;
uint32_t openrac_guest_sp = 0x3F0000u;

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

static void map_guest(void) {
    const size_t span = 0x400000;
#ifdef _WIN32
    uint8_t* p = VirtualAlloc(NULL, span, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(p != NULL);
#else
    uint8_t* p = mmap(NULL, span, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    CHECK(p != MAP_FAILED);
#endif
    openrac_guest_base = p;
}

gaddr openrac_guest_static(uint32_t size) {
    (void)size;
    return 0x3E0000u;
}

/* The game: what it was asked. */
static int running = 1;
static uint32_t generation = 1;
static int set_state = -1;
static int set_calls;
static int reset_state = -1;
static int reset_camera = -1;
static int reset_calls;
static int animation = -1;
static int ride_calls;

int rac1_level_running(void) {
    return running;
}

uint32_t rac1_moby_generation(void) {
    return generation;
}

int func_L00_00222B80__game(int state, int animate) {
    (void)animate;
    set_state = state;
    ++set_calls;
    return 1;
}

#define OTHER_SETTER(name)                                                                         \
    int name(int state, int animate) {                                                             \
        (void)state;                                                                               \
        (void)animate;                                                                             \
        ++set_calls;                                                                               \
        return 1;                                                                                  \
    }

OTHER_SETTER(func_L01_0023D688__game)
OTHER_SETTER(func_L02_0022BE40__game)
OTHER_SETTER(func_L03_00216D38__game)
OTHER_SETTER(func_L05_0024D680__game)
OTHER_SETTER(func_L06_00235E08__game)
OTHER_SETTER(func_L08_00231228__game)
OTHER_SETTER(func_L11_0024E240__game)
OTHER_SETTER(func_L12_002407C0__game)
OTHER_SETTER(func_L14_002306C8__game)
OTHER_SETTER(func_L15_00217348__game)
OTHER_SETTER(func_L16_0021EA88__game)
OTHER_SETTER(func_L17_0021ED38__game)
OTHER_SETTER(func_L18_002284E0__game)

void func_L00_002110C0__game(int mode, int state, gaddr moby) {
    (void)mode;
    (void)state;
    (void)moby;
    ++ride_calls;
}

void func_L01_00231960__game(int mode, int state, int moby) {
    (void)mode;
    (void)state;
    (void)moby;
    ++ride_calls;
}

void func_L00_00217718(gaddr position, gaddr rotation, int state, int camera) {
    (void)position;
    (void)rotation;
    reset_state = state;
    reset_camera = camera;
    ++reset_calls;
    GREF(int32_t, HERO_STATE) = state;
}

void func_L00_00232C10(int seq, int blend, float speed) {
    (void)blend;
    (void)speed;
    animation = seq;
}

static void hero_at_start(void) {
    GREF(gaddr, HERO_MOBY) = 0x200000u;
    GREF(int32_t, HERO_STATE) = 2;
    GREF(int32_t, HERO_HEALTH) = 8;
    GREF(uint8_t, HERO_VEHICLE) = 0;
    GREF(uint8_t, HERO_CAMERA_STOPPED) = 0;
    GREF(uint8_t, HERO_FADE_PENDING) = 0;
    GREF(int16_t, HERO_FRAMES_OFF_GROUND) = 5;
    GREF(int16_t, HERO_AIR_FRAMES) = 5;
    GREF(float, HERO_ROTATION + 8u) = 1.0f;
    GREF(float, HERO_HAND) = 3.0f;
    GREF(float, HERO_POSITION) = 10.0f;
    GREF(int, LEVEL_MODE) = MODE_PLAY;
}

static void an_update_stays_off_this_games_hero(void) {
    hero_at_start();
    set_calls = 0;
    ride_calls = 0;

    /* Outside a stand-in, the game's own. */
    CHECK(func_L00_00222B80(7, 1) == 1 && set_calls == 1 && set_state == 7);
    func_L00_002110C0(2, 0x5A, 0);
    CHECK(ride_calls == 1);
    GREF(int32_t, HERO_STATE) = 2;

    /* It sees the player's, and writes what it likes. */
    rac1_hero_guard_begin();
    rac1_hero_guard_show(0, 5, STAND_IN_AIRBORNE);
    CHECK(GREF(int32_t, HERO_STATE) == 0 && GREF(int32_t, HERO_HEALTH) == 5);
    CHECK(GREF(int16_t, HERO_FRAMES_OFF_GROUND) == 1);
    rac1_hero_guard_show(0, 5, 0);
    CHECK(GREF(int16_t, HERO_FRAMES_OFF_GROUND) == 0);
    CHECK(func_L18_002284E0(0x78, 1) == 1);
    CHECK(func_L00_00222B80(0x3D, 1) == 1);
    CHECK(set_calls == 1 && GREF(int32_t, HERO_STATE) == 0x3D);
    func_L00_002110C0(2, 0x5A, 0);
    func_L01_00231960(2, 0x5A, 0);
    CHECK(ride_calls == 1);
    GREF(int32_t, HERO_HEALTH) = 0;
    GREF(uint8_t, HERO_CAMERA_STOPPED) = 1;
    GREF(uint8_t, HERO_FADE_PENDING) = 1;
    GREF(float, HERO_ROTATION + 8u) = 2.0f;
    GREF(float, HERO_HAND) = 9.0f;
    rac1_hero_guard_killed();

    /* What it did, for the mod. */
    float at[3];
    float yaw;
    int32_t state;
    int32_t health;
    uint32_t flags;
    CHECK(rac1_hero_guard_read(at, &yaw, &state, &health, &flags) == 1);
    CHECK(at[0] == 10.0f && yaw == 2.0f && state == 0x3D && health == 0 && flags == STAND_IN_KILLED);

    /* Nothing of it stays. */
    rac1_hero_guard_end(0);
    CHECK(!rac1_hero_guarded());
    CHECK(GREF(int32_t, HERO_STATE) == 2 && GREF(int32_t, HERO_HEALTH) == 8);
    CHECK(GREF(int16_t, HERO_FRAMES_OFF_GROUND) == 5 && GREF(int16_t, HERO_AIR_FRAMES) == 5);
    CHECK(GREF(uint8_t, HERO_CAMERA_STOPPED) == 0 && GREF(uint8_t, HERO_FADE_PENDING) == 0);
    CHECK(GREF(float, HERO_ROTATION + 8u) == 1.0f && GREF(float, HERO_HAND) == 3.0f);
    CHECK(rac1_hero_guard_read(at, &yaw, &state, &health, &flags) == 0);
}

static void a_fed_sharkigator_stops_carrying(void) {
    const gaddr moby = 0x210000u;
    const gaddr pvars = 0x220000u;
    GREF(gaddr, moby + MOBY_PVARS) = pvars;
    GREF(int16_t, moby + MOBY_CLASS_ID) = UMBRIS_SHARKIGATOR;
    GREF(float, pvars + UMBRIS_SHARKIGATOR_BLEND) = 1.0f;

    /* Its leap goes on; another moby's memory is not touched. */
    GREF(uint8_t, moby + MOBY_STATE) = 5;
    rac1_hero_guard_begin();
    rac1_hero_guard_end(moby);
    CHECK(GREF(float, pvars + UMBRIS_SHARKIGATOR_BLEND) == 1.0f);
    GREF(uint8_t, moby + MOBY_STATE) = UMBRIS_SHARKIGATOR_FED;
    GREF(int16_t, moby + MOBY_CLASS_ID) = 300;
    rac1_hero_guard_begin();
    rac1_hero_guard_end(moby);
    CHECK(GREF(float, pvars + UMBRIS_SHARKIGATOR_BLEND) == 1.0f);

    /* Fed: no respawn here will end the copy to the hero. */
    GREF(int16_t, moby + MOBY_CLASS_ID) = UMBRIS_SHARKIGATOR;
    rac1_hero_guard_begin();
    rac1_hero_guard_end(moby);
    CHECK(GREF(float, pvars + UMBRIS_SHARKIGATOR_BLEND) == 0.0f);
}

static void a_hold_in_this_game(void) {
    hero_at_start();
    reset_calls = 0;
    animation = -1;

    /* Asked in a menu: it waits. */
    GREF(int, LEVEL_MODE) = 1;
    CHECK(rac1_hero_grab(11, GRAB_SPIN) == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 0);

    /* In state 0x78 where he stands, no camera reset, with its animation. */
    GREF(int, LEVEL_MODE) = MODE_PLAY;
    rac1_hero_grab_tick();
    CHECK(reset_calls == 1 && reset_state == STATE_HELD && reset_camera == 0 && animation == 11);
    CHECK(GREF(int32_t, HERO_HEALTH) == 8);

    /* On his way, and spinning; the moved ground point and body centre with him. */
    const float wanted[3] = {20.0f, 0.0f, 0.0f};
    CHECK(rac1_hero_hold_at(wanted, 0.5f) == 1);
    const float x = GREF(float, HERO_POSITION);
    const float centre = GREF(float, HERO_BODY_CENTRE);
    const float yaw = GREF(float, HERO_ROTATION + 8u);
    rac1_hero_grab_tick();
    CHECK(GREF(float, HERO_POSITION) > x && GREF(float, HERO_POSITION) < 20.0f);
    CHECK(GREF(float, HERO_BODY_CENTRE) - centre == GREF(float, HERO_POSITION) - x);
    CHECK(GREF(float, HERO_ROTATION + 8u) != yaw && GREF(float, HERO_ROTATION + 8u) != 0.5f);

    /* Let go: standing, where he is. */
    CHECK(rac1_hero_let_go() == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 2 && reset_state == 0 && reset_camera == 0);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 2);
    CHECK(rac1_hero_let_go() == 0);
}

static void the_games_own_end_of_a_hold(void) {
    /* Another state: he is the game's again, and letting go stands nobody up. */
    hero_at_start();
    reset_calls = 0;
    CHECK(rac1_hero_grab(-1, 0) == 1);
    rac1_hero_grab_tick();
    GREF(int32_t, HERO_STATE) = 0x3D;
    rac1_hero_grab_tick();
    CHECK(rac1_hero_let_go() == 0 && reset_calls == 1);

    /* At no health a let-go stands nobody up. */
    hero_at_start();
    reset_calls = 0;
    CHECK(rac1_hero_grab(-1, 0) == 1);
    rac1_hero_grab_tick();
    GREF(int32_t, HERO_HEALTH) = 0;
    CHECK(rac1_hero_let_go() == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 1);

    /* The level made again takes the hold with it. */
    hero_at_start();
    reset_calls = 0;
    CHECK(rac1_hero_grab(-1, 0) == 1);
    rac1_hero_grab_tick();
    generation = 2;
    rac1_hero_grab_tick();
    CHECK(rac1_hero_let_go() == 0);

    /* On a ride a grab waits, and is dropped when asked back. */
    hero_at_start();
    reset_calls = 0;
    GREF(uint8_t, HERO_VEHICLE) = 1;
    CHECK(rac1_hero_grab(-1, 0) == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 0 && rac1_hero_let_go() == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 0);

    /* No level: refused. */
    running = 0;
    CHECK(rac1_hero_grab(-1, 0) == 0);
    running = 1;
}

static void a_fatal_grab_in_this_game(void) {
    hero_at_start();
    reset_calls = 0;
    animation = -1;
    CHECK(rac1_hero_grab(-1, GRAB_FATAL) == 1);
    rac1_hero_grab_tick();
    CHECK(reset_state == STATE_DYING && GREF(int32_t, HERO_HEALTH) == 0);
    CHECK(GREF(uint8_t, HERO_CAMERA_STOPPED) == 1);
    CHECK(GREF(int32_t, GREF(gaddr, HERO_MOBY) + MOBY_KILL_WORD) == -1 && animation == -1);

    /* Pinned to the holder, facing as it says; let go, the dead stay down. */
    const float mouth[3] = {10.5f, 0.0f, 0.0f};
    CHECK(rac1_hero_hold_at(mouth, 0.25f) == 1);
    rac1_hero_grab_tick();
    CHECK(GREF(float, HERO_ROTATION + 8u) == 0.25f);
    CHECK(rac1_hero_let_go() == 1);
    rac1_hero_grab_tick();
    CHECK(reset_calls == 1);
}

int main(void) {
    map_guest();
    an_update_stays_off_this_games_hero();
    a_fed_sharkigator_stops_carrying();
    a_hold_in_this_game();
    the_games_own_end_of_a_hold();
    a_fatal_grab_in_this_game();
    puts("ok");
    return 0;
}
