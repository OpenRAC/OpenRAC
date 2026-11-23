/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's two executables, and LoadExecPS2 between them.
 *
 * The disc has two programs the console runs: SCES_524.56, which SYSTEM.CNF boots (the single
 * player game and the main menu), and I5BOOTN.ELF, the multiplayer game. Both are the same loader
 * (i5bootn's code at 0x00800000, rac3-decomp's src/i5bootn) carrying a game WAD-compressed at file
 * offset 0x7080, as the records a level program is made of. The main menu's MULTIPLAYER row ends
 * the boot program with LoadExecPS2("cdrom0:\I5BOOTN.ELF;1", 2, {..., "BOPT=..."}) (0x003980A0,
 * 0x00124ED8), and leaving multiplayer starts SCES_524.56 again the same way.
 *
 * The port builds both into one program (the multiplayer one from hostgen.json beside mp/, its
 * C names starting with mp_) and does what the console does: LoadExecPS2 never returns, the stack
 * of the program that called it is dropped (a longjmp to func_0013ADA8, the port's entry), the
 * other program's records are copied over memory, its functions registered in place of the
 * first's, and its crt0's steps run with the arguments the kernel hands it (argv[0] the path,
 * then the caller's).
 *
 * The packed games are unpacked when the disc is set up, not here: the extractor writes each
 * loader's records (its WAD-compressed stream at file offset 0x7080, unpacked) to
 * programs/<file>.records beside the disc's files (tools/openrac_common/disc_files.py). The
 * port reads those, never the disc image or the loader's packed stream. */
#include "exec.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

static jmp_buf exec_jump;
static int exec_armed;
static int running = RAC3_EXEC_BOOT;
static char exec_path[128];
static int exec_argc;
static char exec_argv[8][128];

int rac3_exec_program(void) {
    return running;
}

void rac3_exec_set_program(int program) {
    running = program;
}

jmp_buf* rac3_exec_jump(void) {
    exec_armed = 1;
    return &exec_jump;
}

static uint32_t word_at(const uint8_t* p) {
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

/* The records of a program (programs/<file>.records, the extractor's unpacking of the loader's
 * packed game) copied to their addresses: (address, size, type, entry) and the bytes, type 1
 * code and data, type 8 zeros; the register-use records (0x70000006) load nowhere. The entry of
 * the last record, 0 when there is no program there. */
uint32_t rac3_exec_load_program(const char* file) {
    char path[160];
    uint32_t entry = 0, at = 0;
    snprintf(path, sizeof path, "programs/%s.records", file);
    const uint64_t length = openrac_disc_file_size(path);
    uint8_t* image =
        length <= 0x02000000u ? (uint8_t*)malloc(length > 0 ? (size_t)length : 1) : NULL;
    if (image == NULL || !openrac_disc_read_file(path, image, length)) {
        fprintf(stderr, "rac3: %s is not among the disc's files: set the game up again\n", path);
        free(image);
        return 0;
    }
    const uint32_t size = (uint32_t)length;
    while (size >= 16u && at <= size - 16u) {
        const uint32_t address = word_at(image + at), bytes = word_at(image + at + 4),
                       kind = word_at(image + at + 8);
        if (bytes > size - at - 16) {
            break;
        }
        entry = word_at(image + at + 12);
        if ((kind == 1u || kind == 8u) && address + bytes <= 0x02000000u) {
            memcpy(G(address), image + at + 16, bytes);
        }
        at += 16 + bytes;
    }
    free(image);
    return size >= 16u ? entry : 0;
}

/* The argument block a crt0 reads (its argc, then argv): the path the program was started by,
 * then the arguments LoadExecPS2 was given, the strings after the pointers. */
void rac3_exec_write_arguments(gaddr block) {
    const int argc = exec_path[0] != '\0' ? exec_argc + 1 : 0;
    gaddr strings = block + 4u + 4u * (gaddr)argc;
    memcpy(G(block), &argc, sizeof argc);
    for (int k = 0; k < argc; ++k) {
        const char* s = k == 0 ? exec_path : exec_argv[k - 1];
        const size_t n = strlen(s) + 1;
        memcpy(G(block + 4u + 4u * (gaddr)k), &strings, sizeof strings);
        memcpy(G(strings), s, n);
        strings = (gaddr)((strings + n + 3u) & ~3u);
    }
}

static void copy_string(char* out, size_t size, gaddr at) {
    size_t k = 0;
    if (at != 0 && at < 0x02000000u) {
        const char* s = (const char*)G(at);
        while (k + 1 < size && at + k < 0x02000000u && s[k] != '\0') {
            out[k] = s[k];
            ++k;
        }
    }
    out[k] = '\0';
}

/* LoadExecPS2(path, argc, argv): the program named starts in place of the one running, which does
 * not come back. */
void rac3_load_exec(gaddr path, int argc, gaddr argv) {
    copy_string(exec_path, sizeof exec_path, path);
    exec_argc = argc < 0 ? 0 : argc > 8 ? 8 : argc;
    for (int k = 0; k < exec_argc; ++k) {
        gaddr arg;
        memcpy(&arg, G(argv + 4u * (gaddr)k), sizeof arg);
        copy_string(exec_argv[k], sizeof exec_argv[k], arg);
    }
    const char* name = exec_path;
    for (const char* p = exec_path; *p != '\0'; ++p) {
        if (*p == '\\' || *p == '/' || *p == ':') {
            name = p + 1;
        }
    }
    int next = -1;
    if (strncmp(name, "I5BOOTN.ELF", 11) == 0) {
        next = RAC3_EXEC_MULTIPLAYER;
    } else if (strncmp(name, RAC3_BOOT_FILE, 11) == 0) {
        next = RAC3_EXEC_BOOT;
    }
    fprintf(
        stderr,
        "[info] rac3: LoadExecPS2(\"%s\", %d)%s%s%s\n",
        exec_path,
        exec_argc,
        exec_argc > 0 ? ": \"" : "",
        exec_argc > 0 ? exec_argv[exec_argc - 1] : "",
        exec_argc > 0 ? "\"" : ""
    );
    if (next < 0 || !exec_armed) {
        /* Another program of the disc (the Sly 2 demo, the network settings): not part of the
         * port, which goes on in the program that asked (as before LoadExecPS2 was answered). */
        fprintf(stderr, "[info] rac3: %s is not one of the game's programs: not started\n", name);
        return;
    }
    running = next;
    longjmp(exec_jump, 1);
}
