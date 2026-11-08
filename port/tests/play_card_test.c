/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* A game started from code (rac1/host/play.c's start, PAL's table) after the title's Load and Quit:
 * the card record the Load page left must not make the game's own save path write the player's
 * save<N>.bin. memcard_Save (func_0020BFC8, games/rac1/decomp/src/game/system/memcard.c) saves only when
 * the record names a card in use (+0xC8 not -1) and a save number (+0x14 not below 0), and keeps
 * a save pending in +0xFC; its check is copied here, as the game makes it. */
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
#define RAC1_PLAY_STATICS_ONLY
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "../game/rac1/host/play.c"

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = GUEST_RAM_SIZE;
const struct openrac_map_config* openrac_game_map;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;
uint32_t openrac_guest_sp = 0x3F0000u;
int openrac_game_start_level = -1;
int openrac_game_save_in_use = -1;
int32_t openrac_game_save_in_use_seconds = 0;

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

const char* openrac_env_string(const char* name) {
    (void)name;
    return NULL;
}

int openrac_game_loaded_overlay(void) {
    return -1; /* the title: no level loaded */
}

int openrac_game_card_filter(int mode, const char* folder) {
    (void)mode;
    (void)folder;
    return 1;
}

static int started = -1;

void rac1_start_level(int level) {
    started = level;
}

/* What play.c calls of the game and the rest of the host, which this test never reaches. */
void func_001F7B40(void) {}

void func_001FA218(gaddr a0, gaddr a1) {
    (void)a0;
    (void)a1;
}

void func_00209CE8(int a0) {
    (void)a0;
}

void func_00209DC0(void) {}

void func_0020BA00(gaddr a0) {
    (void)a0;
}

void func_00213D28(gaddr a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
}

void func_00213DE0(gaddr a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
}

void func_0022EAB0(int a0) {
    (void)a0;
}

int func_0022ED80(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

void func_0022F4A0(int a0) {
    (void)a0;
}

void func_L00_002056D8(int a0) {
    (void)a0;
}

void func_L00_00205CC0(void) {}

void func_L00_00207220(void) {}

void func_L00_00208E98(void) {}

void func_L00_002098B8(int a0) {
    (void)a0;
}

void func_L00_00210FE0(void) {}

void func_L00_00232C10(int a0, int a1, float a2) {
    (void)a0;
    (void)a1;
    (void)a2;
}

void func_L00_00232EF0(void) {}

void rac1_figure_items_remove(gaddr figure) {
    (void)figure;
}

int rac1_hero_steady(void) {
    return 0;
}

uint32_t rac1_moby_generation(void) {
    return 1;
}

gaddr rac1_world_create_moby(int32_t oclass) {
    (void)oclass;
    return 0;
}

void rac1_world_moby_kill(gaddr moby) {
    (void)moby;
}

/* memcard_Save's own test of the record: whether the game's save would reach the card. */
static int game_would_save(void) {
    const int active = CARD_WORD(RAC1_CARD_ACTIVE);
    return active != -1 && GREF(int, RAC1_CARD_STATE + active * 0xC0 + CARD_NUMBER) >= 0;
}

int main(void) {
#ifdef _WIN32
    openrac_guest_base =
        VirtualAlloc(NULL, GUEST_RAM_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    openrac_guest_base =
        mmap(NULL, GUEST_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
    /* Boot: no card in use. */
    CARD_WORD(RAC1_CARD_ACTIVE) = -1;
    CHECK(!game_would_save());
    /* The title's Load: card 0 in use, save 3 chosen, saving on. Then Quit Game: back at the
     * title, the record as the Load left it. */
    CARD_WORD(RAC1_CARD_ACTIVE) = 0;
    CARD_WORD(CARD_NUMBER) = 2;
    CARD_WORD(RAC1_CARD_SAVING) = 1;
    CHECK(game_would_save());
    /* A game started from code: none of the player's saves is its own. */
    CHECK(start(4, NULL, 0) == 1 && started == 4);
    CHECK(!game_would_save());
    CHECK(CARD_WORD(RAC1_CARD_SAVING) == 0);
    puts("ok");
    return 0;
}
