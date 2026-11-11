/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The other games' saves on the first card (game/common/lib/mc.c, "The other games' saves"): a
 * folder of card folders as the launcher keeps them, one per disc serial, with Going Commando's
 * card and Ratchet & Clank's beside it. Going Commando lists the card, enters and reads Ratchet &
 * Clank's saves (the Barlow vendor's free weapons, func_00298F00 states 24 to 27), and nothing it
 * does can write, create or delete anything in another game's folder. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "shared card test failed at line %d: %s\n", __LINE__, #condition);     \
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
    DATA = 0x4000
};

#define RAC1_READ 0x2800 /* what Going Commando reads of each Ratchet & Clank save */

static int result(int expected_cmd) {
    CHECK(openrac_lib_sceMcSync(0, CMD, RESULT) == 1);
    CHECK(GREF(int, CMD) == expected_cmd);
    return GREF(int, RESULT);
}

static gaddr name(const char* text) {
    snprintf((char*)G(NAME), 0x100, "%s", text);
    return NAME;
}

static char root[512];

/* A path under the scratch folder. */
static const char* at(const char* relative) {
    static char path[4][900];
    static int next;
    char* out = path[next++ & 3];
    snprintf(out, sizeof path[0], "%s/%s", root, relative);
    return out;
}

static void make_file(const char* relative, uint8_t seed, long size) {
    FILE* f = fopen(at(relative), "wb");
    CHECK(f != NULL);
    for (long i = 0; i < size; i++) {
        fputc((int)(uint8_t)(seed + i * 13), f);
    }
    fclose(f);
}

/* Every name under a folder, each file's size and a sum of its bytes: what must not change. */
static void fingerprint_into(const char* dir, const char* name, void* data);

static void fingerprint(const char* dir, char* out) {
    out[0] = '\0';
    CHECK(each_name(dir, fingerprint_into, out));
}

static void fingerprint_into(const char* dir, const char* name, void* data) {
    char* out = (char*)data;
    char path[1200], line[400];
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return;
    }
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (is_dir(path)) {
        snprintf(line, sizeof line, "[%s]", name);
        strcat(out, line);
        each_name(path, fingerprint_into, out);
        return;
    }
    FILE* f = fopen(path, "rb");
    CHECK(f != NULL);
    unsigned long sum = 0, length = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        sum = sum * 31 + (unsigned long)c;
        length++;
    }
    fclose(f);
    snprintf(line, sizeof line, "%s:%lu:%lx;", name, length, sum);
    CHECK(strlen(out) + strlen(line) < 8000);
    strcat(out, line);
}

/* Removes the scratch folder and everything in it. */
static void remove_tree_into(const char* dir, const char* name, void* data) {
    (void)data;
    char path[1200];
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return;
    }
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (is_dir(path)) {
        each_name(path, remove_tree_into, NULL);
        remove_dir(path);
    } else {
        remove(path);
    }
}

/* The names a listing of `spec` gives, one after another with a ';' after each, in order. */
static void listing_of(int port, const char* spec, char* out, size_t size) {
    CHECK(openrac_lib_sceMcGetDir(port, 0, name(spec), 0, 40, TABLE) == 0);
    const int n = result(MC_GET_DIR);
    CHECK(n >= 0);
    out[0] = '\0';
    for (int i = 0; i < n; i++) {
        const mc_entry* e = (const mc_entry*)G(TABLE + (gaddr)i * sizeof(mc_entry));
        CHECK(strlen(out) + strlen(e->name) + 2 < size);
        strcat(out, e->name);
        strcat(out, ";");
    }
}

static int has(const char* list, const char* entry) {
    char wanted[80];
    snprintf(wanted, sizeof wanted, "%s;", entry);
    for (const char* p = list; (p = strstr(p, wanted)) != NULL; p++) {
        if (p == list || p[-1] == ';') {
            return 1;
        }
    }
    return 0;
}

static const mc_entry* entry_named(int count, const char* entry) {
    for (int i = 0; i < count; i++) {
        const mc_entry* e = (const mc_entry*)G(TABLE + (gaddr)i * sizeof(mc_entry));
        if (strcmp(e->name, entry) == 0) {
            return e;
        }
    }
    return NULL;
}

static int open_file(const char* path, int mode) {
    CHECK(openrac_lib_sceMcOpen(0, 0, name(path), mode) == 0);
    return result(MC_OPEN);
}

/* Reads what Going Commando reads of a save and checks it is the file's, byte for byte. */
static void read_like_rac2(const char* card_path_name, uint8_t seed) {
    const int fd = open_file(card_path_name, 1);
    CHECK(fd >= 0);
    memset(G(DATA), 0, RAC1_READ);
    CHECK(openrac_lib_sceMcRead(fd, DATA, RAC1_READ) == 0);
    CHECK(result(MC_READ) == RAC1_READ);
    for (int i = 0; i < RAC1_READ; i++) {
        CHECK(((uint8_t*)G(DATA))[i] == (uint8_t)(seed + i * 13));
    }
    CHECK(openrac_lib_sceMcClose(fd) == 0);
    CHECK(result(MC_CLOSE) == MC_OK);
}

int main(void) {
    const char* tmp = getenv("TMPDIR");
#if defined(_WIN32)
    if (tmp == NULL) {
        tmp = getenv("TEMP");
    }
#endif
    struct stat st;
    for (int n = 0;; n++) {
        snprintf(
            root,
            sizeof root,
            "%s/openrac_shared_card_%ld_%d",
            tmp != NULL ? tmp : ".",
            (long)time(NULL),
            n
        );
        if (stat(root, &st) != 0) {
            break;
        }
    }
    CHECK(make_dir(root) == 0);

    /* The serial folders, as the launcher keeps them: Going Commando's own (NTSC-U) with its save
     * folder, Ratchet & Clank's NTSC-U and PAL cards with theirs, a folder that is no serial's,
     * and the second card's folder. */
    const char* folders[] = {
        "SCUS_972.68",
        "SCUS_972.68/BASCUS-97268RATCHET",
        "SCUS_971.99",
        "SCUS_971.99/BASCUS-97199RATCHET",
        "SCES_509.16",
        "SCES_509.16/BESCES-50916RATCHET",
        "SCPS_150.37",
        "backups",
        "backups/BISCPS-15037RATCHET",
        "SCUS_972.68.slot2",
    };
    for (size_t i = 0; i < sizeof folders / sizeof folders[0]; i++) {
        CHECK(make_dir(at(folders[i])) == 0);
    }
    make_file("SCUS_972.68/BASCUS-97268RATCHET/save0.bin", 1, 4000);
    make_file("SCUS_971.99/BASCUS-97199RATCHET/save0.bin", 7, 59912);
    make_file("SCUS_971.99/BASCUS-97199RATCHET/save1.bin", 9, 59912);
    make_file("SCUS_971.99/BASCUS-97199RATCHET/icon.sys", 3, 964);
    make_file("SCES_509.16/BESCES-50916RATCHET/save0.bin", 11, 59912);
    make_file("backups/BISCPS-15037RATCHET/save0.bin", 5, 59912);

    char before_us[8192], before_pal[8192], after[8192];
    fingerprint(at("SCUS_971.99"), before_us);
    fingerprint(at("SCES_509.16"), before_pal);

    static char card[700];
    snprintf(card, sizeof card, "%s", at("SCUS_972.68"));
    openrac_game_card_dir = card;
    CHECK(openrac_lib_sceMcGetInfo(0, 0, 0, 0, 0) == 0);
    CHECK(result(MC_GET_INFO) == MC_OK);

    /* The card's root: its own folder, then the other cards' save folders; nothing from a folder
     * that is no card's, and none of the cards' own folders. */
    char list_text[2048];
    listing_of(0, "/*", list_text, sizeof list_text);
    CHECK(strstr(list_text, "BASCUS-97268RATCHET;") < strstr(list_text, "BASCUS-97199RATCHET;"));
    CHECK(has(list_text, "BASCUS-97199RATCHET"));
    CHECK(has(list_text, "BESCES-50916RATCHET"));
    CHECK(!has(list_text, "BISCPS-15037RATCHET"));
    CHECK(!has(list_text, "SCUS_971.99"));
    CHECK(!has(list_text, "SCUS_972.68.slot2"));
    CHECK(openrac_lib_sceMcGetDir(0, 0, name("/*"), 0, 40, TABLE) == 0);
    const int top_count = result(MC_GET_DIR);
    const mc_entry* rac1_top = entry_named(top_count, "BASCUS-97199RATCHET");
    CHECK(rac1_top != NULL && rac1_top->attributes == MC_ATTR_DIR);
    /* A folder asked for by name, from the root. */
    listing_of(0, "/BESCES-50916RATCHET", list_text, sizeof list_text);
    CHECK(strcmp(list_text, "BESCES-50916RATCHET;") == 0);

    /* Inside Ratchet & Clank's folder: its files and their sizes. */
    listing_of(0, "/BASCUS-97199RATCHET/*", list_text, sizeof list_text);
    CHECK(has(list_text, "save0.bin") && has(list_text, "save1.bin") && has(list_text, "icon.sys"));
    CHECK(openrac_lib_sceMcGetDir(0, 0, name("/BASCUS-97199RATCHET/*"), 0, 40, TABLE) == 0);
    const int rac1_count = result(MC_GET_DIR);
    const mc_entry* save = entry_named(rac1_count, "save0.bin");
    CHECK(save != NULL && save->size == 59912 && save->attributes == MC_ATTR_FILE);

    /* Going Commando's reads, by the names it uses (no leading '/', from the root), each region. */
    read_like_rac2("BASCUS-97199RATCHET/save0.bin", 7);
    read_like_rac2("BASCUS-97199RATCHET/save1.bin", 9);
    read_like_rac2("BESCES-50916RATCHET/save0.bin", 11);
    /* A save that is not there, and a region no card has: not found, as on a console. */
    CHECK(open_file("BASCUS-97199RATCHET/save4.bin", 1) == MC_NOT_FOUND);
    CHECK(open_file("BISCPS-15037RATCHET/save0.bin", 1) == MC_NOT_FOUND);
    /* The game's own saves are its own card's. */
    const int own = open_file("/BASCUS-97268RATCHET/save0.bin", 1);
    CHECK(own >= 0 && !file_foreign[own]);
    CHECK(openrac_lib_sceMcClose(own) == 0);
    CHECK(result(MC_CLOSE) == MC_OK);

    /* Entering the folder works too, and a read inside it is the same file. */
    CHECK(openrac_lib_sceMcChdir(0, 0, name("/BASCUS-97199RATCHET"), 0) == 0);
    CHECK(result(MC_CHDIR) == MC_OK);
    read_like_rac2("save0.bin", 7);
    CHECK(openrac_lib_sceMcChdir(0, 0, name("/"), 0) == 0);
    CHECK(result(MC_CHDIR) == MC_OK);

    /* Nothing can change another game's folder: writing, creating, deleting, making folders. */
    CHECK(open_file("BASCUS-97199RATCHET/save0.bin", 2) == MC_FAILED);
    CHECK(open_file("BASCUS-97199RATCHET/save0.bin", 3) == MC_FAILED);
    CHECK(open_file("BASCUS-97199RATCHET/save0.bin", 0x203) == MC_FAILED);
    CHECK(open_file("/BASCUS-97199RATCHET/save9.bin", 0x203) == MC_FAILED);
    CHECK(open_file("BESCES-50916RATCHET/new.bin", 0x202) == MC_FAILED);
    CHECK(openrac_lib_sceMcDelete(0, 0, name("/BASCUS-97199RATCHET/save1.bin")) == 0);
    CHECK(result(MC_DELETE) == MC_FAILED);
    CHECK(openrac_lib_sceMcDelete(0, 0, name("/BESCES-50916RATCHET")) == 0);
    CHECK(result(MC_DELETE) == MC_FAILED);
    CHECK(openrac_lib_sceMcMkdir(0, 0, name("/BASCUS-97199RATCHET")) == 0);
    CHECK(result(MC_MKDIR) == MC_EXISTS);
    CHECK(openrac_lib_sceMcMkdir(0, 0, name("/BASCUS-97199RATCHET/extra")) == 0);
    CHECK(result(MC_MKDIR) == MC_FAILED);
    /* A file opened to read is not written either. */
    const int fd = open_file("BASCUS-97199RATCHET/save0.bin", 1);
    CHECK(fd >= 0);
    memset(G(DATA), 0xEE, 64);
    CHECK(openrac_lib_sceMcWrite(fd, DATA, 64) == 0);
    CHECK(result(MC_WRITE) == MC_FAILED);
    CHECK(openrac_lib_sceMcClose(fd) == 0);
    CHECK(result(MC_CLOSE) == MC_OK);
    /* A path climbing out of the folder is no other game's: it stays on the game's own card. */
    CHECK(open_file("/BASCUS-97199RATCHET/../BASCUS-97199RATCHET/save0.bin", 0x203) < 0);

    /* While a party plays the filter is on; the other games' saves are read the same and still
     * never written. */
    CHECK(openrac_game_card_filter(CARD_DISCARD, NULL));
    read_like_rac2("BASCUS-97199RATCHET/save0.bin", 7);
    CHECK(open_file("BASCUS-97199RATCHET/save0.bin", 0x203) == MC_FAILED);
    CHECK(openrac_game_card_filter(CARD_PASS, NULL));

    fingerprint(at("SCUS_971.99"), after);
    CHECK(strcmp(before_us, after) == 0);
    fingerprint(at("SCES_509.16"), after);
    CHECK(strcmp(before_pal, after) == 0);

    /* The second card is a card of its own: none of the other games' folders. */
    listing_of(1, "/*", list_text, sizeof list_text);
    CHECK(!has(list_text, "BASCUS-97199RATCHET") && !has(list_text, "BESCES-50916RATCHET"));
    CHECK(openrac_lib_sceMcOpen(1, 0, name("BASCUS-97199RATCHET/save0.bin"), 1) == 0);
    CHECK(result(MC_OPEN) == MC_NOT_FOUND);

    /* A folder of the game's own card with another game's folder name comes first. */
    CHECK(make_dir(at("SCUS_972.68/BESCES-50916RATCHET")) == 0);
    make_file("SCUS_972.68/BESCES-50916RATCHET/save0.bin", 21, RAC1_READ);
    read_like_rac2("BESCES-50916RATCHET/save0.bin", 21);
    listing_of(0, "/*", list_text, sizeof list_text);
    {
        int seen = 0;
        for (const char* p = list_text; (p = strstr(p, "BESCES-50916RATCHET;")) != NULL; p++) {
            seen++;
        }
        CHECK(seen == 1);
    }

    each_name(root, remove_tree_into, NULL);
    remove_dir(root);
    CHECK(stat(root, &st) != 0);
    printf("shared card test passed\n");
    return 0;
}
