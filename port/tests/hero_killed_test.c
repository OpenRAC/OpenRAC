/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Hero_OnKilled under a stand-in (rac1/host/hero_down.c, PAL's table): the player standing in died,
 * not this game's hero. */
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
#include "../game/rac1/host/hero_down.c"

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

gaddr openrac_guest_static(uint32_t size) {
    (void)size;
    return 0x3E0000u;
}

static int guarded;
static int noted;
static int killed;

int rac1_hero_guarded(void) {
    return guarded;
}

void rac1_hero_guard_killed(void) {
    ++noted;
}

int rac1_level_running(void) {
    return 1;
}

uint32_t rac1_moby_generation(void) {
    return 1;
}

void func_L00_00211908__game(void) {
    ++killed;
}

void func_L00_00217718(gaddr position, gaddr rotation, int state, int camera) {
    (void)position;
    (void)rotation;
    (void)state;
    (void)camera;
}

int main(void) {
#ifdef _WIN32
    openrac_guest_base = VirtualAlloc(NULL, 0x400000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    openrac_guest_base = mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
    func_L00_00211908();
    CHECK(killed == 1 && noted == 0);
    guarded = 1;
    func_L00_00211908();
    CHECK(killed == 1 && noted == 1);
    guarded = 0;
    func_L00_00211908();
    CHECK(killed == 2 && noted == 1);
    puts("ok");
    return 0;
}
