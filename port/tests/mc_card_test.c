/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The memory card library (game/common/lib/mc.c) on a scratch card folder, with no game: a save the
 * game writes is on disk when the write returns, so a program killed before it closes the file
 * (a crash, a kill, a closed window) still leaves it; and the save folder's listing holds only the
 * card's own files, not what file browsers leave in it, which Going Commando counts. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#if !defined(_WIN32)
#include <unistd.h>

#include <sys/wait.h>
#endif

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "memory card test failed at line %d: %s\n", __LINE__, #condition);     \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

/* The game's memory: a small fixture at guest addresses 0 up. */
static uint8_t memory[0x10000];

static void* test_guest(gaddr address) {
    CHECK(address < sizeof memory);
    return memory + address;
}

#undef G
#define G(address) test_guest((gaddr)(address))
#include "../game/common/lib/mc.c"

#if defined(_WIN32)
#define remove_dir(p) _rmdir(p)
#else
#define remove_dir(p) rmdir(p)
#endif

const char* openrac_game_card_dir;
const char* openrac_game_loaded_mods;
const openrac_game_play* openrac_game_play_host;
int openrac_game_no_card;

enum {
    NAME = 0x100,
    CMD = 0x200,
    RESULT = 0x204,
    TABLE = 0x1000,
    DATA = 0x8000
};

/* The finished command's result, as the game's sceMcSync loop reads it. */
static int result(int expected_cmd) {
    CHECK(openrac_lib_sceMcSync(0, CMD, RESULT) == 1);
    CHECK(GREF(int, CMD) == expected_cmd);
    return GREF(int, RESULT);
}

static gaddr name(const char* text) {
    snprintf((char*)G(NAME), 0x100, "%s", text);
    return NAME;
}

static void write_file(const char* path, const uint8_t* bytes, int size, int close) {
    CHECK(openrac_lib_sceMcOpen(0, 0, name(path), 0x203) == 0);
    const int fd = result(MC_OPEN);
    CHECK(fd >= 0);
    memcpy(G(DATA), bytes, (size_t)size);
    CHECK(openrac_lib_sceMcWrite(fd, DATA, size) == 0);
    CHECK(result(MC_WRITE) == size);
    if (close) {
        CHECK(openrac_lib_sceMcClose(fd) == 0);
        CHECK(result(MC_CLOSE) == 0);
    }
}

static long file_size(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 ? (long)st.st_size : -1;
}

int main(void) {
    char card[512], path[700];
    const char* tmp = getenv("TMPDIR");
#if defined(_WIN32)
    if (tmp == NULL) {
        tmp = getenv("TEMP");
    }
#endif
    /* A folder no earlier run left behind. */
    struct stat st;
    for (int n = 0;; n++) {
        snprintf(
            card,
            sizeof card,
            "%s/openrac_mc_test_%ld_%d",
            tmp != NULL ? tmp : ".",
            (long)time(NULL),
            n
        );
        if (stat(card, &st) != 0) {
            break;
        }
    }
    openrac_game_card_dir = card;

    /* The game's folder and its files, as Going Commando makes them on a new card. */
    CHECK(openrac_lib_sceMcGetInfo(0, 0, 0, 0, 0) == 0);
    CHECK(result(MC_GET_INFO) == 0);
    CHECK(openrac_lib_sceMcMkdir(0, 0, name("/BASCUS-97268RATCHET")) == 0);
    CHECK(result(MC_MKDIR) == 0);
    static uint8_t save[4096];
    for (size_t i = 0; i < sizeof save; i++) {
        save[i] = (uint8_t)(i * 7 + 1);
    }
    const char* card_files[] =
        {"icon.sys",
         "BASCUS-97268RATCHET",
         "static.ico",
         "save0.bin",
         "save1.bin",
         "save2.bin",
         "save3.bin"};
    for (size_t i = 0; i < sizeof card_files / sizeof card_files[0]; i++) {
        snprintf(path, sizeof path, "/BASCUS-97268RATCHET/%s", card_files[i]);
        write_file(path, save, (int)sizeof save, 1);
    }

    /* What Finder, Explorer, the port and the launcher put beside them. */
    const char* clutter[] =
        {".DS_Store", "._save0.bin", "Thumbs.db", "DESKTOP.INI", "save0.bin.mods"};
    for (size_t i = 0; i < sizeof clutter / sizeof clutter[0]; i++) {
        snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/%s", card, clutter[i]);
        FILE* f = fopen(path, "wb");
        CHECK(f != NULL);
        fputs("not the card's", f);
        fclose(f);
    }
    snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/.backup_1", card);
    CHECK(make_dir(path) == 0);

    /* The listing the game checks: ".", ".." and its seven files, nothing else. */
    CHECK(openrac_lib_sceMcGetDir(0, 0, name("/BASCUS-97268RATCHET/*"), 0, 11, TABLE) == 0);
    const int count = result(MC_GET_DIR);
    CHECK(count == 9);
    int seen_files = 0;
    for (int i = 0; i < count; i++) {
        const mc_entry* e = (const mc_entry*)G(TABLE + (gaddr)i * sizeof(mc_entry));
        CHECK(e->name[0] != '.' || strcmp(e->name, ".") == 0 || strcmp(e->name, "..") == 0);
        CHECK(strcmp(e->name, "Thumbs.db") != 0 && strcmp(e->name, "DESKTOP.INI") != 0);
        if (e->attributes == MC_ATTR_FILE) {
            CHECK(e->size == sizeof save);
            seen_files++;
        }
    }
    CHECK(seen_files == 7);

#if !defined(_WIN32)
    /* A save written by a program that is then killed before it closes the file: on disk whole. */
    static uint8_t later[4096];
    for (size_t i = 0; i < sizeof later; i++) {
        later[i] = (uint8_t)(255 - i);
    }
    const pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        write_file("/BASCUS-97268RATCHET/save2.bin", later, (int)sizeof later, 0);
        _exit(0); /* no close, no exit handlers: as a kill would leave it */
    }
    int status = 0;
    CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);

    /* Read back through the library, as Load does. */
    CHECK(openrac_lib_sceMcOpen(0, 0, name("/BASCUS-97268RATCHET/save2.bin"), 1) == 0);
    const int fd = result(MC_OPEN);
    CHECK(fd >= 0);
    memset(G(DATA), 0, sizeof later);
    CHECK(openrac_lib_sceMcRead(fd, DATA, (int)sizeof later) == 0);
    CHECK(result(MC_READ) == (int)sizeof later);
    CHECK(memcmp(G(DATA), later, sizeof later) == 0);
    CHECK(openrac_lib_sceMcClose(fd) == 0);
    CHECK(result(MC_CLOSE) == 0);
#endif

    /* The other slots are untouched. */
    snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/save1.bin", card);
    CHECK(file_size(path) == (long)sizeof save);

    /* Clean up the scratch card. */
    for (size_t i = 0; i < sizeof card_files / sizeof card_files[0]; i++) {
        snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/%s", card, card_files[i]);
        remove(path);
    }
    for (size_t i = 0; i < sizeof clutter / sizeof clutter[0]; i++) {
        snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/%s", card, clutter[i]);
        remove(path);
    }
    snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET/.backup_1", card);
    remove_dir(path);
    snprintf(path, sizeof path, "%s/BASCUS-97268RATCHET", card);
    remove_dir(path);
    remove_dir(card);
    printf("memory card: listing and killed-writer save OK\n");
    return 0;
}
