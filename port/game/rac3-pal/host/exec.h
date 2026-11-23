/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's two executables and LoadExecPS2 between them (exec.c). */
#ifndef RAC3_EXEC_H
#define RAC3_EXEC_H

#include <setjmp.h>
#include <stdint.h>

#include "openrac/guest.h"

enum {
    RAC3_EXEC_BOOT = 0,        /* SCES_524.56: the single player game and the main menu */
    RAC3_EXEC_MULTIPLAYER = 1, /* I5BOOTN.ELF: the multiplayer game (mp/hostgen.json) */
};

/* The executable running. */
int rac3_exec_program(void);
void rac3_exec_set_program(int program);
/* Where LoadExecPS2 goes back to (func_0013ADA8's setjmp), armed by asking for it. */
jmp_buf* rac3_exec_jump(void);
/* LoadExecPS2(path, argc, argv) of either executable: never returns for one of the game's. */
void rac3_load_exec(gaddr path, int argc, gaddr argv);
/* The program a loader file carries ("I5BOOTN.ELF", RAC3_BOOT_FILE) copied into memory from the
 * records the extractor unpacked (programs/<file>.records); its entry, 0 for none. */
uint32_t rac3_exec_load_program(const char* file);
/* The crt0's argument block at `block`: argc, argv, the strings. */
void rac3_exec_write_arguments(gaddr block);

/* The multiplayer executable (mp.c): its functions registered, its crt0's steps and main. */
void rac3_mp_run(void);

#endif
