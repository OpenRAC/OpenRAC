/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The card filter (lib/mc.c) against the calls memcard_update_state makes, one for one, on a real
 * folder: while a party plays no write reaches a player's own saves, whatever the game does. Each
 * scenario runs with the filter off first: the same calls change the save, so a pass means the
 * filter stopped them, not that the calls missed. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#define make_dir(p) _mkdir(p)
#else
#include <unistd.h>
#define make_dir(p) mkdir((p), 0755)
#endif

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

static int failures;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c);                                \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

/* What the port's main has, and the host's say of a save: a save here is SAVE_SIZE bytes whose
 * first byte is 'S'. */
#define SAVE_SIZE 3000
#define GAME_PART 1000
#define LEVEL_PART 200
const char* openrac_game_card_dir;
int openrac_game_no_card;
const char* openrac_game_loaded_mods = "[{\"id\": \"multiplayer\", \"version\": \"1\"}]";
static int host_save_info(
    const uint8_t* save, uint32_t size, int32_t* level, int32_t* bolts, int32_t* gold, int32_t* secs
) {
    *level = *bolts = *gold = *secs = 0;
    return size == SAVE_SIZE && save[0] == 'S';
}
static const openrac_game_play host = {
    .save_file = "save%d.bin", .save_slots = 5, .save_info = host_save_info
};
const openrac_game_play* openrac_game_play_host = &host;

/* The guest memory: names and buffers the game hands the library. */
static uint8_t guest[0x10000];
uint8_t* openrac_guest_base = guest;
#define NAME 0x100
#define BUFFER 0x1000

static char root[512], folder_path[600], save_path[640], coop_path[640];

static void name_is(const char* name) {
    snprintf((char*)guest + NAME, 64, "%s", name);
}

/* The result of the command just issued, as sceMcSync gives it. */
static int result(void) {
    GREF(int, 0x40) = GREF(int, 0x44) = 0;
    CHECK(openrac_lib_sceMcSync(1, 0x40, 0x44) == 1);
    return GREF(int, 0x44);
}

static int mc_open(const char* name, int mode) {
    name_is(name);
    openrac_lib_sceMcOpen(0, 0, NAME, mode);
    return result();
}

static void mc_write(int fd, uint8_t value, int size) {
    memset(guest + BUFFER, value, (size_t)size);
    openrac_lib_sceMcWrite(fd, BUFFER, size);
    CHECK(result() == size);
}

static void mc_seek(int fd, int offset, int whence) {
    openrac_lib_sceMcSeek(fd, offset, whence);
    CHECK(result() >= 0);
}

static void mc_close(int fd) {
    openrac_lib_sceMcClose(fd);
    CHECK(result() == 0);
}

/* MEMCARD_UPDATE_SAVE: the game's own save: the two parts, in place. */
static void game_saves(const char* name, uint8_t value, int record, int close_it) {
    int fd = mc_open(name, 2);
    CHECK(fd >= 0);
    mc_seek(fd, 8, 0);
    mc_write(fd, value, GAME_PART);
    if (record != 0) {
        mc_seek(fd, record * LEVEL_PART, 1);
    }
    mc_write(fd, value, LEVEL_PART);
    if (close_it) {
        mc_close(fd);
    }
}

/* MEMCARD_WRITE_FULL_SAVE: the pause menu's Save. */
static void game_saves_whole(const char* name, uint8_t value) {
    int fd = mc_open(name, 2);
    CHECK(fd >= 0);
    memset(guest + BUFFER, value, SAVE_SIZE);
    guest[BUFFER] = 'S';
    openrac_lib_sceMcWrite(fd, BUFFER, SAVE_SIZE);
    CHECK(result() == SAVE_SIZE);
    mc_close(fd);
}

/* MEMCARD_CREATE_SAVE_DIRECTORY's files, over what is there. */
static void game_makes_folder(const char* name, uint8_t value) {
    int fd = mc_open(name, 0x203);
    CHECK(fd >= 0);
    mc_write(fd, value, 64);
    mc_close(fd);
}

/* MEMCARD_DELETE_SAVE_DIRECTORY's delete. */
static void game_deletes(const char* name) {
    name_is(name);
    openrac_lib_sceMcDelete(0, 0, NAME);
    CHECK(result() == 0);
}

static void write_file(const char* path, uint8_t value, size_t size) {
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL);
    for (size_t i = 0; i < size; i++) {
        fputc(i == 0 ? 'S' : value, f);
    }
    fclose(f);
}

/* The bytes of a file; 0 for none, -1 for one not all `value` after its 'S'. */
static long file_holds(const char* path, uint8_t value) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    long n = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (n > 0 && c != value) {
            n = -1;
            break;
        }
        n++;
    }
    fclose(f);
    return n;
}

static int exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static void remove_all(const char* path) {
    remove(path);
}

static void clean(void) {
    char p[800];
    const char* files[] = {"", ".tmp", ".bak", ".mods"};
    for (int i = 0; i < 4; i++) {
        snprintf(p, sizeof p, "%s%s", coop_path, files[i]);
        remove_all(p);
        snprintf(p, sizeof p, "%s%s", save_path, files[i]);
        remove_all(p);
    }
    snprintf(p, sizeof p, "%s/icon.sys", folder_path);
    remove_all(p);
}

/* A fresh card: the player's save 0, all 0x11. */
static void fresh(void) {
    clean();
    openrac_game_card_filter(0, "");
    write_file(save_path, 0x11, SAVE_SIZE);
}

static void a_write_reaches_the_card_with_the_filter_off(void) {
    fresh();
    game_saves("save0.bin", 0x22, 2, 1);
    CHECK(file_holds(save_path, 0x11) != SAVE_SIZE);
    fresh();
    game_saves_whole("save0.bin", 0x22);
    CHECK(file_holds(save_path, 0x22) == SAVE_SIZE);
    fresh();
    game_makes_folder("save0.bin", 0x22);
    CHECK(file_holds(save_path, 0x11) != SAVE_SIZE);
    fresh();
    game_deletes("save0.bin");
    CHECK(!exists(save_path));
}

static void the_party_never_writes_the_players_save(void) {
    for (int mode = 1; mode <= 2; mode++) {
        fresh();
        CHECK(openrac_game_card_filter(mode, "GAME-coop"));
        game_saves("save0.bin", 0x22, 2, 1);
        game_saves_whole("save0.bin", 0x33);
        game_makes_folder("save0.bin", 0x44);
        game_deletes("save0.bin");
        game_deletes(".");
        CHECK(file_holds(save_path, 0x11) == SAVE_SIZE);
        openrac_game_card_filter(0, "");
    }
}

static void a_save_of_the_games_lands_on_the_party_copy(void) {
    fresh();
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    openrac_game_card_slot(3);
    memset(guest + BUFFER, 0x55, SAVE_SIZE);
    guest[BUFFER] = 'S';
    CHECK(openrac_game_card_write(3, guest + BUFFER, SAVE_SIZE));
    CHECK(file_holds(coop_path, 0x55) == SAVE_SIZE);
    const uint32_t before = openrac_game_card_saves();

    game_saves("save0.bin", 0x66, 2, 1);

    /* Only the parts the game wrote changed: the file stays whole under them. */
    FILE* f = fopen(coop_path, "rb");
    uint8_t back[SAVE_SIZE];
    CHECK(f != NULL && fread(back, 1, SAVE_SIZE, f) == SAVE_SIZE);
    fclose(f);
    CHECK(back[0] == 'S' && back[7] == 0x55 && back[8] == 0x66);
    CHECK(back[8 + GAME_PART - 1] == 0x66 && back[8 + GAME_PART] == 0x55);
    CHECK(back[8 + GAME_PART + 2 * LEVEL_PART] == 0x66);
    CHECK(back[SAVE_SIZE - 1] == 0x55);
    CHECK(openrac_game_card_saves() == before + 1);

    /* The one before kept, a mark beside it, nothing left over. */
    char p[800];
    snprintf(p, sizeof p, "%s.bak", coop_path);
    CHECK(file_holds(p, 0x55) == SAVE_SIZE);
    snprintf(p, sizeof p, "%s.mods", coop_path);
    CHECK(exists(p));
    snprintf(p, sizeof p, "%s.tmp", coop_path);
    CHECK(!exists(p));
    snprintf(p, sizeof p, "%s.mods", save_path);
    CHECK(!exists(p));
    CHECK(file_holds(save_path, 0x11) == SAVE_SIZE);
    openrac_game_card_filter(0, "");
}

static void a_part_of_a_save_with_no_save_under_it_comes_to_nothing(void) {
    fresh();
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    openrac_game_card_slot(1);
    char path[800], temp[800];
    snprintf(path, sizeof path, "%s/GAME-coop/save1.bin", root);
    snprintf(temp, sizeof temp, "%s.tmp", path);
    remove(path);
    game_saves("save0.bin", 0x66, 0, 1);
    CHECK(!exists(path) && !exists(temp));
    /* A whole save makes one. */
    game_saves_whole("save0.bin", 0x77);
    CHECK(file_holds(path, 0x77) == SAVE_SIZE);
    remove(path);
    openrac_game_card_filter(0, "");
}

static void a_write_cut_short_leaves_the_save_as_it_was(void) {
    fresh();
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    openrac_game_card_slot(3);
    memset(guest + BUFFER, 0x55, SAVE_SIZE);
    guest[BUFFER] = 'S';
    CHECK(openrac_game_card_write(3, guest + BUFFER, SAVE_SIZE));

    /* The game stops in the middle (the same as a crash: the file is never closed). */
    game_saves("save0.bin", 0x66, 2, 0);
    CHECK(file_holds(coop_path, 0x55) == SAVE_SIZE);

    /* The party goes meanwhile: the write stays the party's when it ends. */
    openrac_game_card_filter(0, "");
    openrac_lib_sceMcClose(0);
    CHECK(result() == 0);
    CHECK(file_holds(save_path, 0x11) == SAVE_SIZE);

    /* A copy left by a crash is written over by the next save. */
    char temp[800];
    snprintf(temp, sizeof temp, "%s.tmp", coop_path);
    write_file(temp, 0x99, 10);
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    openrac_game_card_slot(3);
    game_saves("save0.bin", 0x67, 0, 1);
    CHECK(!exists(temp));
    FILE* f = fopen(coop_path, "rb");
    uint8_t back[64];
    CHECK(f != NULL && fread(back, 1, sizeof back, f) == sizeof back);
    fclose(f);
    CHECK(back[8] == 0x67);
    openrac_game_card_filter(0, "");
}

static void a_save_the_game_would_not_load_never_replaces_one(void) {
    fresh();
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    memset(guest + BUFFER, 0x55, SAVE_SIZE);
    guest[BUFFER] = 'S';
    CHECK(openrac_game_card_write(3, guest + BUFFER, SAVE_SIZE));
    CHECK(!openrac_game_card_write(3, guest + BUFFER, 12));
    CHECK(file_holds(coop_path, 0x55) == SAVE_SIZE);
    openrac_game_card_filter(0, "");
    CHECK(!openrac_game_card_write(3, guest + BUFFER, SAVE_SIZE));
}

static void a_new_cards_folder_still_gets_made(void) {
    fresh();
    CHECK(openrac_game_card_filter(2, "GAME-coop"));
    char icon[800];
    snprintf(icon, sizeof icon, "%s/icon.sys", folder_path);
    game_makes_folder("icon.sys", 0x22);
    CHECK(exists(icon));
    /* Once there it stays as it was. */
    game_makes_folder("icon.sys", 0x33);
    FILE* f = fopen(icon, "rb");
    CHECK(f != NULL && fgetc(f) == 0x22);
    fclose(f);
    openrac_game_card_filter(0, "");
}

static void refused(void) {
    CHECK(!openrac_game_card_filter(9, "GAME-coop"));
    CHECK(!openrac_game_card_filter(2, ""));
    CHECK(!openrac_game_card_filter(2, NULL));
}

int main(int argc, char** argv) {
    snprintf(root, sizeof root, "%s", argc > 1 ? argv[1] : "mc_filter_card");
    make_dir(root);
    openrac_game_card_dir = root;
    snprintf(folder_path, sizeof folder_path, "%s/GAME", root);
    make_dir(folder_path);
    snprintf(save_path, sizeof save_path, "%s/save0.bin", folder_path);
    snprintf(coop_path, sizeof coop_path, "%s/GAME-coop/save3.bin", root);
    name_is("/GAME");
    openrac_lib_sceMcChdir(0, 0, NAME, 0);
    CHECK(result() == 0);

    a_write_reaches_the_card_with_the_filter_off();
    the_party_never_writes_the_players_save();
    a_save_of_the_games_lands_on_the_party_copy();
    a_part_of_a_save_with_no_save_under_it_comes_to_nothing();
    a_write_cut_short_leaves_the_save_as_it_was();
    a_save_the_game_would_not_load_never_replaces_one();
    a_new_cards_folder_still_gets_made();
    refused();

    clean();
    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
    }
    return failures != 0;
}
