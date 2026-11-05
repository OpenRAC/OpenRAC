/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Between a game built without the disc and the part of it built from the
 * disc later (tools/asm_part.py, port/tools/hostgen/asmext.py): the functions
 * the decompilation has only in assembly are called through slots, which the
 * part's openrac_asm_register fills (program/asm_part.cpp loads it).
 */
#ifndef OPENRAC_ASM_PART_H
#define OPENRAC_ASM_PART_H

#ifdef __cplusplus
extern "C" {
#endif

/* FN, a slot's function, for a call through it; a stop that says what is
 * missing when the part was never built or loaded (FN null). NAME is the
 * function's. Called by the generated trampolines. */
void* openrac_asm_external(void* fn, const char* name);

/* Where the runtime global NAME ("openrac_guest_base", guest.h
 * OPENRAC_PART_GLOBALS) is, for a part on Android, where those globals are the
 * game library's own; null for any other name. */
void* openrac_asm_global(const char* name);

#ifdef __cplusplus
}
#endif

#endif
