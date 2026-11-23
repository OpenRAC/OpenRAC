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
/* A file of the disc's root ("I5BOOTN.ELF"), malloc'd; NULL when it is not there. */
uint8_t* rac3_exec_read_disc_file(const char* name, uint32_t* size);
/* The loader's unpacking of a loader file's packed game into memory; its entry, 0 for none. */
uint32_t rac3_exec_unpack(const uint8_t* file, uint32_t size);
/* The same from the packed stream itself (the loader the port's loader placed: 0x00806080). */
uint32_t rac3_exec_unpack_payload(const uint8_t* payload);
/* The crt0's argument block at `block`: argc, argv, the strings. */
void rac3_exec_write_arguments(gaddr block);

/* The multiplayer executable (mp.c): its functions registered, its crt0's steps and main. */
void rac3_mp_run(void);

#endif
