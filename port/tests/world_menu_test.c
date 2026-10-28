/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* The level's world under the pause menu (rac1/host/world.c, PAL's table): while the menu's moby
 * context is up (select_moby_context(1), its array in the level's pointers), a moby the mods make
 * (a co-op figure) is made among the level's mobys and one they delete is deleted there, its
 * deletion noted as a mod's; and the camera they are given is the level's, not the menu's at
 * 256,256,64. */
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
#include "../game/rac1/host/world.c"

/* Guest memory: the first 4 MB and the scratchpad, at their addresses. */
enum {
    RAM = 0x400000
};

uint8_t* openrac_guest_base;
const struct openrac_map_config* openrac_game_map;

static void map_guest(void) {
    const size_t span = GUEST_SCRATCHPAD + GUEST_SCRATCHPAD_SIZE;
#ifdef _WIN32
    uint8_t* p = VirtualAlloc(NULL, span, MEM_RESERVE, PAGE_NOACCESS);
    CHECK(p != NULL);
    CHECK(VirtualAlloc(p, RAM, MEM_COMMIT, PAGE_READWRITE) != NULL);
    CHECK(
        VirtualAlloc(p + GUEST_SCRATCHPAD, GUEST_SCRATCHPAD_SIZE, MEM_COMMIT, PAGE_READWRITE)
        != NULL
    );
#else
    uint8_t* p = mmap(NULL, span, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    CHECK(p != MAP_FAILED);
    CHECK(mprotect(p, RAM, PROT_READ | PROT_WRITE) == 0);
    CHECK(mprotect(p + GUEST_SCRATCHPAD, GUEST_SCRATCHPAD_SIZE, PROT_READ | PROT_WRITE) == 0);
#endif
    openrac_guest_base = p;
}

gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;
uint32_t openrac_guest_sp = 0x3F0000u;

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

gaddr openrac_guest_frame(uint32_t size) {
    openrac_guest_sp -= (size + 15u) & ~15u;
    return openrac_guest_sp;
}

/* The two moby arrays: the level's and the pause menu's own. */
#define LEVEL_ARRAY 0x200000u
#define MENU_ARRAY 0x300000u

/* select_moby_context: the menu's array put in the level's pointers, the level's in the alt_
 * ones; and back. */
void func_L00_00235FF8(int context) {
    if (context == GREF(int, CONTEXT)) {
        return;
    }
    const gaddr array = GREF(gaddr, MOBY_ARRAY);
    const gaddr end = GREF(gaddr, MOBY_PLACED_END);
    GREF(gaddr, MOBY_ARRAY) = GREF(gaddr, ALT_ARRAY);
    GREF(gaddr, MOBY_PLACED_END) = GREF(gaddr, ALT_PLACED_END);
    GREF(gaddr, ALT_ARRAY) = array;
    GREF(gaddr, ALT_PLACED_END) = end;
    GREF(int, CONTEXT) = context;
}

/* CreateMoby: the next slot of the current array; DeleteMoby: the state the game gives. */
static gaddr created_in;

gaddr func_0020D348(int oclass) {
    const gaddr base = GREF(gaddr, MOBY_ARRAY);
    created_in = base;
    const gaddr m = base + 300u * MOBY_SIZE;
    memset(G(m), 0, MOBY_SIZE);
    GREF(int16_t, m + MOBY_OCLASS) = (int16_t)oclass;
    return m;
}

static gaddr deleted_in;

void func_0020D678__game(gaddr moby) {
    deleted_in = GREF(gaddr, MOBY_ARRAY);
    GREF(uint8_t, moby + MOBY_STATE) = STATE_DELETED;
}

/* The menu's own update: what it does to the camera block (initialize_pause_menu's camera). */
static void put_menu_camera(void) {
    const float menu[3] = {256.0f, 256.0f, 64.0f};
    memcpy(G(OPENRAC_DATA(CAMERA_POSITION)), menu, sizeof menu);
}

void func_L00_00277A88__game(void) {
    put_menu_camera();
    func_L00_00235FF8(1);
}

void func_001FD3E8__game(void) {}

void func_00213C78(void) {}

void func_00218B10(void) {}

void func_002352C8(void) {}

void func_L00_0025AAC0__game(gaddr a0, gaddr a1) {
    (void)a0;
    (void)a1;
}

void func_L00_0025AC00__game(gaddr a0, float a1, int a2, int a3, gaddr a4, gaddr a5) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
}

void openrac_game_draw(unsigned layers) {
    (void)layers;
}

void openrac_game_world_under_menu(void) {}

/* No mod hooks the moby loop here. */
int (*openrac_game_moby_update)(int32_t slot, int phase);
int (*openrac_game_moby_hit)(
    int32_t victim,
    int32_t attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
);

void rac1_figure_hit_dropped(const char* what) {
    (void)what;
}

int rac1_figure_owned(gaddr moby) {
    (void)moby;
    return 0;
}

int rac1_level_running(void) {
    return 1;
}

int rac1_moby_run(int32_t slot, uint32_t* runs, float camera[3]) {
    (void)slot;
    *runs = 0;
    memset(camera, 0, 3 * sizeof(float));
    return -1;
}

uint32_t rac1_program_loads(void) {
    return 1;
}

int rac1_scene_actor(gaddr moby) {
    (void)moby;
    return 0;
}

gaddr rac1_updating_moby(void) {
    return 0;
}

int main(void) {
    map_guest();
    GREF(gaddr, MOBY_ARRAY) = LEVEL_ARRAY;
    GREF(gaddr, MOBY_PLACED_END) = LEVEL_ARRAY + 298u * MOBY_SIZE;
    GREF(gaddr, ALT_ARRAY) = MENU_ARRAY;
    GREF(gaddr, ALT_PLACED_END) = MENU_ARRAY + 4u * MOBY_SIZE;
    GREF(int, CONTEXT) = 0;
    const float level[3] = {132.3f, 112.4f, 34.6f};
    memcpy(G(OPENRAC_DATA(CAMERA_POSITION)), level, sizeof level);
    rac1_world_keep_running(1);

    /* In play: the level's camera. */
    float at[3];
    CHECK(rac1_world_camera_position(at));
    CHECK(memcmp(at, level, sizeof at) == 0);

    /* The menu's first tick saves the level's camera, then puts up its own and its mobys. */
    func_L00_00277A88();
    CHECK(GREF(int, CONTEXT) == 1);
    CHECK(GREF(float, OPENRAC_DATA(CAMERA_POSITION)) == 256.0f);
    CHECK(rac1_world_camera_position(at));
    CHECK(memcmp(at, level, sizeof at) == 0);

    /* Its later ticks (the level's camera swapped in for the world's run and back): still. */
    for (int tick = 0; tick < 264; ++tick) { /* 4.4 s */
        func_L00_00277A88();
        CHECK(GREF(int, CONTEXT) == 1);
        CHECK(GREF(float, OPENRAC_DATA(CAMERA_POSITION)) == 256.0f);
        CHECK(rac1_world_camera_position(at));
        CHECK(memcmp(at, level, sizeof at) == 0);
    }

    /* A figure made under the menu is the level's, the menu's context left up. */
    const gaddr figure = rac1_world_create_moby(0);
    CHECK(figure == LEVEL_ARRAY + 300u * MOBY_SIZE);
    CHECK(created_in == LEVEL_ARRAY);
    CHECK(GREF(int, CONTEXT) == 1 && GREF(gaddr, MOBY_ARRAY) == MENU_ARRAY);

    /* Deleted under the menu: among the level's mobys, noted as a mod's. */
    rac1_world_moby_kill(figure);
    CHECK(deleted_in == LEVEL_ARRAY);
    CHECK(GREF(uint8_t, figure + MOBY_STATE) == STATE_DELETED);
    CHECK(GREF(int, CONTEXT) == 1 && GREF(gaddr, MOBY_ARRAY) == MENU_ARRAY);
    openrac_game_moby_deletion d;
    CHECK(rac1_world_moby_deletion(300, &d));
    CHECK(d.by_mod == 1 && d.by == -1 && d.oclass == 0);

    /* Out of the menu: the camera block's own again. */
    func_L00_00235FF8(0);
    memcpy(G(OPENRAC_DATA(CAMERA_POSITION)), level, sizeof level);
    CHECK(rac1_world_camera_position(at));
    CHECK(memcmp(at, level, sizeof at) == 0);

    printf("world_menu ok\n");
    return 0;
}
