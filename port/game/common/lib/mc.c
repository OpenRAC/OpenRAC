/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The memory card library (libmc). A card is a folder (card_dir for the
 * first port, card_dir.slot2 for the second, which counts as inserted only if the
 * folder exists), and the game's save folder and files are ordinary files in
 * it, so saves can be copied to and from other tools.
 *
 * The library's calls start a command and sceMcSync reports its result; here
 * every command finishes when it is issued, and sceMcSync hands back its
 * result the first time it is polled; the command and its result stay where
 * the game's own library kept them, when its host C says where
 * (openrac_game_mc_state). The structures and codes are the
 * library's public interface as the game uses them
 * (docs/port/RAC1_PAL_SURVEY.md, section 7).
 *
 * While a party plays, a filter stands between the game and its saves (the end of "The save mark"
 * below says how): whatever the game believes it writes, a player's own saves stay as they were. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#define make_dir(p) _mkdir(p)
#else
#include <dirent.h>
#include <unistd.h>
#define make_dir(p) mkdir((p), 0755)
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

/* The command numbers sceMcSync reports. */
enum {
    MC_GET_INFO = 0x01,
    MC_OPEN = 0x02,
    MC_CLOSE = 0x03,
    MC_SEEK = 0x04,
    MC_READ = 0x05,
    MC_WRITE = 0x06,
    MC_MKDIR = 0x0B,
    MC_CHDIR = 0x0C,
    MC_GET_DIR = 0x0D,
    MC_DELETE = 0x0F,
    MC_FORMAT = 0x10,
    MC_UNFORMAT = 0x11,
};

/* Results. */
#define MC_OK 0
#define MC_NO_CARD (-10)
#define MC_NOT_FOUND (-4)
#define MC_EXISTS (-4)
#define MC_FAILED (-5)
/* A missing or empty name: the library refuses it before it starts a command
 * (sceMcOpen func_001238B0, and through it sceMcMkdir, sceMcGetDir
 * func_001241F0, sceMcDelete func_00124410 test it once they hold the
 * library's semaphore), so sceMcSync has nothing new to report. */
#define MC_NO_NAME (-210)

/* Directory entry attributes. */
#define MC_ATTR_FILE 0x8497
#define MC_ATTR_DIR 0x8427

/* The command that has finished and its result, for sceMcSync: in the game's memory where the game
 * says the console's library kept them (openrac_game_mc_state), else here. */
static int pending_cmd;
static int pending_result;
static int have_pending;
static gaddr state_command, state_result;

void openrac_game_mc_state(gaddr command, gaddr result) {
    state_command = command;
    state_result = result;
}

static int no_name(gaddr name) {
    return name == 0 || *(const char*)G(name) == 0;
}

static int finish(int cmd, int result) {
    if (state_command != 0) {
        GREF(int, state_command) = cmd;
        GREF(int, state_result) = result;
        return 0;
    }
    pending_cmd = cmd;
    pending_result = result;
    have_pending = 1;
    return 0; /* the command was started */
}

/* Each card's current folder (sceMcChdir), relative to its root without
 * slashes at either end: "" is the root. A name that does not start with '/'
 * is in it. */
static char current_dir[2][512];

/* The first port's card is card_dir itself, the folder the launcher's save
 * manager shows (launcher/core/src/saves.rs); the second port's is beside it,
 * card_dir.slot2, and counts as inserted only if it exists. */
static int card_path(int port, const char* name, char* out, size_t size) {
    const char* root = openrac_game_card_dir != NULL ? openrac_game_card_dir : "memcard";
    const char* dir = *name == '/' ? "" : current_dir[port != 0];
    while (*name == '/') {
        name++;
    }
    return snprintf(
               out,
               size,
               "%s%s%s%s%s%s",
               root,
               port == 0 ? "" : ".slot2",
               *dir ? "/" : "",
               dir,
               *name ? "/" : "",
               name
           )
           < (int)size;
}

static int card_present(int port) {
    char path[1024];
    if (openrac_game_no_card) {
        return 0;
    }
    struct stat st;
    if (!card_path(port, "", path, sizeof path)) {
        return 0;
    }
    if (port == 0) {
        make_dir(path); /* the first card is always inserted */
    }
    return stat(path, &st) == 0;
}

/* sceMcInit */
int openrac_lib_sceMcInit(void) {
    return 0;
}

/* sceMcGetInfo(port, slot, type, free, format): a formatted 8 MB card. */
int openrac_lib_sceMcGetInfo(int port, int slot, gaddr type, gaddr free_clusters, gaddr format) {
    (void)slot;
    if (!card_present(port)) {
        return finish(MC_GET_INFO, MC_NO_CARD);
    }
    if (type != 0) {
        GREF(int, type) = 2; /* a PS2 memory card */
    }
    if (free_clusters != 0) {
        GREF(int, free_clusters) = 7000;
    }
    if (format != 0) {
        GREF(int, format) = 1;
    }
    return finish(MC_GET_INFO, MC_OK);
}

/* ---- Files ---- */

#define MAX_FILES 8
static FILE* files[MAX_FILES];
/* Each open file's path and whether the game wrote to it, for the save mark below. */
static char file_paths[MAX_FILES][1024];
static int file_written[MAX_FILES];
/* What the card filter made of the open (below): 0 as asked. */
static int file_fate[MAX_FILES];

/* ---- The save mark ---- */

/* A save the game wrote keeps a small file of its own beside it, `<save>.mods`, saying which
 * mods were loaded while it was played (openrac_game_loaded_mods). The save file itself is never
 * touched: its 8-byte header has no room and every section carries a CRC (assets/disc/save_game.h),
 * and a real memory card must still take the file. Playing without mods deletes the mark, so an
 * overwritten save never keeps the one before it. The launcher reads it (core/src/saves.rs) and
 * so does the game's own memory card page. */

/* Which of the game's save files `name` is (a file name without its folder), -1 for none. The host
 * says what they are called (openrac_game_play_host->save_file, "save%d.bin"). */
static int save_slot_of(const char* name) {
    if (openrac_game_play_host == NULL || openrac_game_play_host->save_file == NULL) {
        return -1;
    }
    for (int slot = 0; slot < openrac_game_play_host->save_slots; slot++) {
        char expected[64];
        if (snprintf(expected, sizeof expected, openrac_game_play_host->save_file, slot)
                < (int)sizeof expected
            && strcmp(expected, name) == 0) {
            return slot;
        }
    }
    return -1;
}

static int is_save_file(const char* name) {
    return save_slot_of(name) >= 0;
}

/* The mark's own name for a save's path, "" when it does not fit. */
static void mark_path(const char* save_path, char* out, size_t size) {
    if (snprintf(out, size, "%s.mods", save_path) >= (int)size) {
        *out = '\0';
    }
}

/* Whether the file at `path` holds a game the player saved, rather than one of the blank slots the
 * game writes when it makes its folder on a fresh card. The host answers it: save_info reads the
 * save and says whether it is one the game would load (game_host.h, rac1/host/play.c). Without a
 * host answer nothing is marked, which leaves a save plain rather than marking every slot. */
static int is_a_players_save(const char* path) {
    if (openrac_game_play_host == NULL || openrac_game_play_host->save_info == NULL) {
        return 0;
    }
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    const long size = ftell(f);
    if (size <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    uint8_t* bytes = (uint8_t*)malloc((size_t)size);
    if (bytes == NULL) {
        fclose(f);
        return 0;
    }
    const size_t read = fread(bytes, 1, (size_t)size, f);
    fclose(f);
    int32_t level = 0, bolts = 0, gold_bolts = 0, seconds = 0;
    const int real = read == (size_t)size
                     && openrac_game_play_host->save_info(
                            bytes, (uint32_t)size, &level, &bolts, &gold_bolts, &seconds
                        ) != 0;
    free(bytes);
    return real;
}

/* Writes or removes the mark beside the save at `path`, which the game has just written. An empty
 * slot never keeps one: the game writes all five files when it makes its folder on a fresh card,
 * and only the ones the player saved into are marked. */
static void mark_save(const char* path) {
    const char* mods = openrac_game_loaded_mods != NULL ? openrac_game_loaded_mods : "[]";
    char mark[1100];
    mark_path(path, mark, sizeof mark);
    if (*mark == '\0') {
        return;
    }
    if (strcmp(mods, "[]") == 0 || !is_a_players_save(path)) {
        /* Played without mods, or a slot with no game in it: the save is plain. */
        remove(mark);
        return;
    }
    char when[32];
    const time_t now = time(NULL);
    struct tm parts;
#if defined(_WIN32)
    const int ok = localtime_s(&parts, &now) == 0;
#else
    const int ok = localtime_r(&now, &parts) != NULL;
#endif
    if (!ok || strftime(when, sizeof when, "%Y-%m-%dT%H:%M:%S", &parts) == 0) {
        snprintf(when, sizeof when, "%s", "");
    }
    FILE* f = fopen(mark, "wb");
    if (f == NULL) {
        return;
    }
    fprintf(f, "{\"format\": 1, \"saved\": \"%s\", \"mods\": %s}\n", when, mods);
    fclose(f);
}


/* ---- The card filter ---- */

/* A party's game must never reach a player's own saves. The game writes its saves two ways, and a
 * filter has to take both as they are (memcard_update_state):
 *   - a save of the game's own (a gold bolt, a story moment, leaving by ship) opens save<N>.bin for
 *     writing without creating it and writes only the game part and its level's part, in place;
 *   - the pause menu's Save opens it the same way and writes the whole file.
 * So a redirected write needs the whole save underneath. It goes to a private copy, `<save>.tmp`,
 * of the folder's save (the party's, which the mod wrote there first: openrac_game_card_write),
 * and only a close that leaves a save the game would load puts it in place. A crash or a lost
 * party in the middle of a write leaves the save as it was: the copy is only ever replaced, the
 * previous save stays as `<save>.bak` (read_save in common/mod_play.cpp falls back to it), and a
 * write without a save under it (a new game's, before the first copy) comes to nothing.
 *
 * What a write becomes is settled at the open and stays with its file, so the filter going off in
 * the middle of a write cannot send the rest of it to the player's save. The game is always told
 * it worked: its state machine and its menus go on as they do with a card.
 *
 * Besides a save: making the folder on a fresh card creates files that are not there, which takes
 * nothing from anyone and passes (the game finds its folder later); a file that is there is left
 * alone, and sceMcDelete (the game clearing a folder it finds damaged) deletes nothing. */

#define CARD_PASS 0
#define CARD_DISCARD 1
#define CARD_REDIRECT 2

#define FATE_ASKED 0
#define FATE_DROPPED 1
#define FATE_REDIRECTED 2

static int card_mode = CARD_PASS;
static char card_folder[256];
static int card_slot = -1;
static uint32_t card_saves;

int openrac_game_card_filter(int mode, const char* folder) {
    if (mode != CARD_PASS && mode != CARD_DISCARD && mode != CARD_REDIRECT) {
        return 0;
    }
    if (mode == CARD_REDIRECT
        && (folder == NULL || *folder == '\0' || strlen(folder) >= sizeof card_folder)) {
        return 0;
    }
    card_mode = mode;
    snprintf(card_folder, sizeof card_folder, "%s", mode == CARD_REDIRECT ? folder : "");
    if (mode == CARD_PASS) {
        card_slot = -1;
    }
    return 1;
}

void openrac_game_card_slot(int slot) {
    card_slot = slot;
}

uint32_t openrac_game_card_saves(void) {
    return card_saves;
}

/* The file of `slot` in the redirect folder, the folder made; 0 when it does not fit or there is
 * no host to name it. */
static int redirect_path(int slot, char* out, size_t size) {
    const char* root = openrac_game_card_dir != NULL ? openrac_game_card_dir : "memcard";
    char name[64];
    if (openrac_game_play_host == NULL || openrac_game_play_host->save_file == NULL
        || snprintf(name, sizeof name, openrac_game_play_host->save_file, slot)
               >= (int)sizeof name) {
        return 0;
    }
    if (snprintf(out, size, "%s/%s", root, card_folder) >= (int)size) {
        return 0;
    }
    make_dir(out);
    return snprintf(out, size, "%s/%s/%s", root, card_folder, name) < (int)size;
}

/* A sibling of `path`: `path` with `suffix` after it. */
static int sibling(const char* path, const char* suffix, char* out, size_t size) {
    return snprintf(out, size, "%s%s", path, suffix) < (int)size;
}

/* Copies the file at `from` to `to`; 0 when it cannot be read or written. */
static int copy_file(const char* from, const char* to) {
    FILE* in = fopen(from, "rb");
    if (in == NULL) {
        return 0;
    }
    FILE* out = fopen(to, "wb");
    if (out == NULL) {
        fclose(in);
        return 0;
    }
    char block[4096];
    size_t n;
    int ok = 1;
    while ((n = fread(block, 1, sizeof block, in)) > 0) {
        if (fwrite(block, 1, n, out) != n) {
            ok = 0;
        }
    }
    if (ferror(in) || fclose(out) != 0) {
        ok = 0;
    }
    fclose(in);
    return ok;
}

/* The bytes on their way to the disk before anything counts on them. */
static void settle(FILE* f) {
    fflush(f);
#if defined(_WIN32)
    _commit(_fileno(f));
#else
    fsync(fileno(f));
#endif
}

/* Puts the finished copy `temp` in place of the save `target`, which stays as `<target>.bak`; 0
 * (and the save as it was) for a copy the game would not load. */
static int put_in_place(const char* temp, const char* target) {
    char previous[1100];
    if (!sibling(target, ".bak", previous, sizeof previous) || !is_a_players_save(temp)) {
        remove(temp);
        return 0;
    }
    FILE* there = fopen(target, "rb");
    if (there != NULL) {
        fclose(there);
        remove(previous);
        rename(target, previous);
    }
    if (rename(temp, target) != 0) {
        rename(previous, target);
        remove(temp);
        return 0;
    }
    return 1;
}

/* Writes a whole save as `slot` of the redirect folder (coop_save_write), in place at once. */
int openrac_game_card_write(int slot, const uint8_t* bytes, uint32_t size) {
    char target[1024], temp[1100];
    if (card_mode != CARD_REDIRECT || bytes == NULL || size == 0
        || !redirect_path(slot, target, sizeof target)
        || !sibling(target, ".tmp", temp, sizeof temp)) {
        return 0;
    }
    FILE* f = fopen(temp, "wb");
    if (f == NULL) {
        return 0;
    }
    const int written = fwrite(bytes, 1, size, f) == size;
    settle(f);
    fclose(f);
    if (!written || !put_in_place(temp, target)) {
        remove(temp);
        return 0;
    }
    mark_save(target);
    return 1;
}

/* Starts the private copy of the redirect folder's save the game's write goes to; the game's own
 * `slot` unless the mod named one. Fills `target` and `temp`; 0 when there can be no copy. */
static int start_redirect(int slot, char* target, size_t target_size, char* temp, size_t temp_size) {
    char previous[1100];
    if (!redirect_path(card_slot >= 0 ? card_slot : slot, target, target_size)
        || !sibling(target, ".tmp", temp, temp_size)
        || !sibling(target, ".bak", previous, sizeof previous)) {
        return 0;
    }
    if (copy_file(target, temp) || copy_file(previous, temp)) {
        return 1;
    }
    /* Nothing yet: a whole save written over it is a save, a part of one comes to nothing. */
    FILE* f = fopen(temp, "wb");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/* Whether a file is at `path`. */
static int is_there(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/* What the filter makes of opening `path` with `mode`. */
static int fate_of(const char* path, int mode) {
    if (card_mode == CARD_PASS || (mode & (2 | 0x200)) == 0) {
        return FATE_ASKED;
    }
    if ((mode & 0x200) != 0) {
        return is_there(path) ? FATE_DROPPED : FATE_ASKED;
    }
    const char* slash = strrchr(path, '/');
    if (save_slot_of(slash != NULL ? slash + 1 : path) < 0) {
        return is_there(path) ? FATE_DROPPED : FATE_ASKED;
    }
    return card_mode == CARD_REDIRECT ? FATE_REDIRECTED : FATE_DROPPED;
}

/* sceMcOpen(port, slot, name, mode): mode 1 read, 2 write, 3 both, 0x200 create. */
int openrac_lib_sceMcOpen(int port, int slot, gaddr name, int mode) {
    (void)slot;
    char path[1024];
    if (no_name(name)) {
        return MC_NO_NAME;
    }
    if (!card_present(port) || !card_path(port, (const char*)G(name), path, sizeof path)) {
        return finish(MC_OPEN, MC_NO_CARD);
    }
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i] == NULL) {
            fd = i;
            break;
        }
    }
    if (fd < 0) {
        return finish(MC_OPEN, -7);
    }
    FILE* f = NULL;
    int fate = fate_of(path, mode);
    if (fate == FATE_REDIRECTED) {
        const char* slash = strrchr(path, '/');
        char temp[1100];
        if (start_redirect(
                save_slot_of(slash != NULL ? slash + 1 : path), path, sizeof path, temp, sizeof temp
            )) {
            f = fopen(temp, "r+b");
        }
        if (f == NULL) {
            /* No copy can be made (a full disk): the write comes to nothing, never to the save. */
            fate = FATE_DROPPED;
            f = fopen(path, "rb");
        }
    } else if (fate == FATE_DROPPED) {
        f = fopen(path, "rb");
    } else if (mode & 0x200) {
        f = fopen(path, "r+b");
        if (f == NULL) {
            f = fopen(path, "w+b");
        }
    } else if (mode & 2) {
        f = fopen(path, "r+b");
    } else {
        f = fopen(path, "rb");
    }
    if (f == NULL) {
        return finish(MC_OPEN, MC_NOT_FOUND);
    }
    files[fd] = f;
    snprintf(file_paths[fd], sizeof file_paths[fd], "%s", path);
    file_written[fd] = 0;
    file_fate[fd] = fate;
    return finish(MC_OPEN, fd);
}

/* sceMcClose(fd) */
int openrac_lib_sceMcClose(int fd) {
    if (fd < 0 || fd >= MAX_FILES || files[fd] == NULL) {
        return finish(MC_CLOSE, MC_FAILED);
    }
    if (file_fate[fd] == FATE_REDIRECTED) {
        settle(files[fd]);
    }
    fclose(files[fd]);
    files[fd] = NULL;
    /* The party's save just written: in place now, whole, or not at all. */
    if (file_fate[fd] == FATE_REDIRECTED) {
        char temp[1100];
        if (sibling(file_paths[fd], ".tmp", temp, sizeof temp)) {
            if (!file_written[fd]) {
                remove(temp);
            } else if (put_in_place(temp, file_paths[fd])) {
                mark_save(file_paths[fd]);
                card_saves++;
            }
        }
        file_written[fd] = 0;
        file_fate[fd] = FATE_ASKED;
        return finish(MC_CLOSE, MC_OK);
    }
    /* A save just written: its mark is set or removed now, never half way through the write. */
    if (file_written[fd]) {
        const char* slash = strrchr(file_paths[fd], '/');
        if (is_save_file(slash != NULL ? slash + 1 : file_paths[fd])) {
            mark_save(file_paths[fd]);
        }
    }
    file_written[fd] = 0;
    return finish(MC_CLOSE, MC_OK);
}

/* sceMcSeek(fd, offset, whence) */
int openrac_lib_sceMcSeek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FILES || files[fd] == NULL || fseek(files[fd], offset, whence) != 0) {
        return finish(MC_SEEK, MC_FAILED);
    }
    return finish(MC_SEEK, (int)ftell(files[fd]));
}

/* sceMcRead(fd, buffer, size) */
int openrac_lib_sceMcRead(int fd, gaddr buffer, int size) {
    if (fd < 0 || fd >= MAX_FILES || files[fd] == NULL) {
        return finish(MC_READ, MC_FAILED);
    }
    return finish(MC_READ, (int)fread(G(buffer), 1, (size_t)size, files[fd]));
}

/* sceMcWrite(fd, buffer, size) */
int openrac_lib_sceMcWrite(int fd, gaddr buffer, int size) {
    if (fd < 0 || fd >= MAX_FILES || files[fd] == NULL) {
        return finish(MC_WRITE, MC_FAILED);
    }
    if (file_fate[fd] == FATE_DROPPED) {
        fseek(files[fd], size, SEEK_CUR);
        return finish(MC_WRITE, size);
    }
    const size_t n = fwrite(G(buffer), 1, (size_t)size, files[fd]);
    fflush(files[fd]);
    file_written[fd] = 1;
    return finish(MC_WRITE, (int)n);
}

/* sceMcMkdir(port, slot, name) */
int openrac_lib_sceMcMkdir(int port, int slot, gaddr name) {
    (void)slot;
    char path[1024];
    struct stat st;
    if (no_name(name)) {
        return MC_NO_NAME;
    }
    if (!card_present(port) || !card_path(port, (const char*)G(name), path, sizeof path)) {
        return finish(MC_MKDIR, MC_NO_CARD);
    }
    if (stat(path, &st) == 0) {
        return finish(MC_MKDIR, MC_EXISTS);
    }
    return finish(MC_MKDIR, make_dir(path) == 0 ? MC_OK : MC_FAILED);
}

/* sceMcDelete(port, slot, name): a file, or an empty folder. */
int openrac_lib_sceMcDelete(int port, int slot, gaddr name) {
    (void)slot;
    char path[1024];
    if (no_name(name)) {
        return MC_NO_NAME;
    }
    if (!card_present(port) || !card_path(port, (const char*)G(name), path, sizeof path)) {
        return finish(MC_DELETE, MC_NO_CARD);
    }
    if (card_mode != CARD_PASS) {
        return finish(MC_DELETE, MC_OK);
    }
    if (remove(path) == 0) {
        return finish(MC_DELETE, MC_OK);
    }
#if defined(_WIN32)
    return finish(MC_DELETE, _rmdir(path) == 0 ? MC_OK : MC_NOT_FOUND);
#else
    return finish(MC_DELETE, rmdir(path) == 0 ? MC_OK : MC_NOT_FOUND);
#endif
}

/* sceMcFormat, sceMcUnformat: a folder is always formatted. */
int openrac_lib_sceMcFormat(int port, int slot) {
    (void)slot;
    return finish(MC_FORMAT, card_present(port) ? MC_OK : MC_NO_CARD);
}

int openrac_lib_sceMcUnformat(int port, int slot) {
    (void)slot;
    return finish(MC_UNFORMAT, card_present(port) ? MC_OK : MC_NO_CARD);
}

/* ---- Directories ---- */

/* One entry of sceMcGetDir's table, 64 bytes. */
typedef struct {
    uint8_t created[8]; /* reserved, second, minute, hour, day, month, year (16 bits) */
    uint8_t modified[8];
    uint32_t size;
    uint16_t attributes;
    uint16_t reserved1;
    uint32_t reserved2;
    uint32_t pda;
    char name[32];
} mc_entry;

#define MAX_ENTRIES 64
static mc_entry listing[MAX_ENTRIES];
static int listing_count, listing_next;

static void stamp(uint8_t out[8], time_t t) {
    struct tm local;
#if defined(_WIN32)
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    out[0] = 0;
    out[1] = (uint8_t)local.tm_sec;
    out[2] = (uint8_t)local.tm_min;
    out[3] = (uint8_t)local.tm_hour;
    out[4] = (uint8_t)local.tm_mday;
    out[5] = (uint8_t)(local.tm_mon + 1);
    out[6] = (uint8_t)((local.tm_year + 1900) & 0xFF);
    out[7] = (uint8_t)((local.tm_year + 1900) >> 8);
}

static void add_entry(const char* dir, const char* name) {
    char path[1200];
    struct stat st;
    if (listing_count >= MAX_ENTRIES) {
        return;
    }
    snprintf(path, sizeof path, "%s/%s", dir, name);
    mc_entry* e = &listing[listing_count++];
    memset(e, 0, sizeof *e);
    if (stat(path, &st) == 0) {
        stamp(e->created, st.st_mtime);
        stamp(e->modified, st.st_mtime);
        e->size = S_ISDIR(st.st_mode) ? 0 : (uint32_t)st.st_size;
        e->attributes = S_ISDIR(st.st_mode) ? MC_ATTR_DIR : MC_ATTR_FILE;
    }
    snprintf(e->name, sizeof e->name, "%s", name);
}

static int matches(const char* pattern, const char* name) {
    /* The save marks are the port's own, not the card's: the game never sees them. */
    const size_t length = strlen(name);
    if (length > 5 && strcmp(name + length - 5, ".mods") == 0) {
        return 0;
    }
    if (strcmp(pattern, "*") == 0) {
        return 1;
    }
    return strcmp(pattern, name) == 0;
}

static void list(int port, const char* spec) {
    char dir_part[512], pattern[64], path[1024];
    const char* slash = strrchr(spec, '/');
    listing_count = listing_next = 0;
    if (slash == NULL) {
        snprintf(dir_part, sizeof dir_part, "%s", "");
        snprintf(pattern, sizeof pattern, "%s", spec);
    } else {
        snprintf(dir_part, sizeof dir_part, "%.*s", (int)(slash - spec), spec);
        snprintf(pattern, sizeof pattern, "%s", slash + 1);
    }
    if (!card_path(port, dir_part, path, sizeof path)) {
        return;
    }
#if defined(_WIN32)
    char query[1100];
    struct _finddata_t found;
    snprintf(query, sizeof query, "%s/*", path);
    intptr_t h = _findfirst(query, &found);
    if (h == -1) {
        return;
    }
    do {
        if (matches(pattern, found.name)) {
            add_entry(path, found.name);
        }
    } while (_findnext(h, &found) == 0);
    _findclose(h);
#else
    DIR* d = opendir(path);
    if (d == NULL) {
        return;
    }
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (matches(pattern, ent->d_name)) {
            add_entry(path, ent->d_name);
        }
    }
    closedir(d);
#endif
}

/* sceMcGetDir(port, slot, name, mode, max entries, table): mode 0 starts a
 * listing, otherwise it continues one. */
int openrac_lib_sceMcGetDir(
    int port, int slot, gaddr name, unsigned int mode, int max, gaddr table
) {
    (void)slot;
    if (no_name(name)) {
        return MC_NO_NAME;
    }
    if (!card_present(port)) {
        return finish(MC_GET_DIR, MC_NO_CARD);
    }
    if (mode == 0) {
        list(port, (const char*)G(name));
    }
    int n = 0;
    while (n < max && listing_next < listing_count) {
        memcpy(G(table + (gaddr)n * sizeof(mc_entry)), &listing[listing_next++], sizeof(mc_entry));
        n++;
    }
    return finish(MC_GET_DIR, n);
}

/* sceMcChdir(port, slot, path, old path): enters a folder of the card, from
 * its root when the path starts with '/'; "." and ".." work as usual. A folder
 * that does not exist leaves the current one and reports not found. The old
 * path, when asked for, is written as an absolute one. */
int openrac_lib_sceMcChdir(int port, int slot, gaddr path, gaddr old_path) {
    (void)slot;
    if (!card_present(port)) {
        return finish(MC_CHDIR, MC_NO_CARD);
    }
    char* cur = current_dir[port != 0];
    if (old_path != 0) {
        snprintf((char*)G(old_path), 1024, "/%s", cur);
    }
    const char* p = (const char*)G(path);
    char next[512];
    snprintf(next, sizeof next, "%s", *p == '/' ? "" : cur);
    while (*p) {
        while (*p == '/') {
            p++;
        }
        size_t len = strcspn(p, "/");
        if (len == 0 || (len == 1 && p[0] == '.')) {
            /* nothing */
        } else if (len == 2 && p[0] == '.' && p[1] == '.') {
            char* slash = strrchr(next, '/');
            if (slash != NULL) {
                *slash = '\0';
            } else {
                next[0] = '\0';
            }
        } else {
            size_t used = strlen(next);
            if (used + len + 2 > sizeof next) {
                return finish(MC_CHDIR, MC_FAILED);
            }
            snprintf(next + used, sizeof next - used, "%s%.*s", used ? "/" : "", (int)len, p);
        }
        p += len;
    }
    char full[1024];
    struct stat st;
    char saved[512];
    snprintf(saved, sizeof saved, "%s", cur);
    snprintf(cur, sizeof current_dir[0], "%s", next);
    if (!card_path(port, "", full, sizeof full) || stat(full, &st) != 0 || !S_ISDIR(st.st_mode)) {
        snprintf(cur, sizeof current_dir[0], "%s", saved);
        return finish(MC_CHDIR, MC_NOT_FOUND);
    }
    return finish(MC_CHDIR, MC_OK);
}

/* sceMcSync(mode, command, result): 1 when a command has finished (always,
 * once), -1 when none is running. With the library's state in the game's
 * memory, as the console's sceMcSync once the IOP has answered: the command
 * word handed out and cleared, the result word handed out. */
int openrac_lib_sceMcSync(int mode, gaddr cmd, gaddr result) {
    (void)mode;
    if (state_command != 0) {
        if (GREF(int, state_command) == 0) {
            return -1;
        }
        if (cmd != 0) {
            GREF(int, cmd) = GREF(int, state_command);
        }
        GREF(int, state_command) = 0;
        if (result != 0) {
            GREF(int, result) = GREF(int, state_result);
        }
        return 1;
    }
    if (!have_pending) {
        return -1;
    }
    have_pending = 0;
    if (cmd != 0) {
        GREF(int, cmd) = pending_cmd;
    }
    if (result != 0) {
        GREF(int, result) = pending_result;
    }
    return 1;
}
