/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Native counterpart to Up Your Arsenal's loader and start-up.
 *
 * The disc boots SCES_524.56, a small loader linked at 0x00800000 (i5bootn's code) whose
 * read-only data holds the game itself, WAD-compressed at file offset 0x7080: a list of records
 * (load address, size, type, entry; then the bytes), the format level programs use
 * (editor/formats.py, overlay_sections). The loader unpacks it and jumps to its entry,
 * 0x0013ADA8, the payload's crt0 (hand-written, rac3-decomp asm/boot_elf/handwritten): it clears
 * core.bss, makes the main thread and the heap ($gp 0x001DC8B0, stack below 0x01FFC000, heap end
 * 0x01FF8000), calls _InitSys (0x00124B90) and FlushCache(0), runs _init (0x0012A940, empty),
 * registers the finaliser through atexit (0x00116FD0) and calls main (0x0013B330) with the
 * argument block at 0x0015C040.
 *
 * NTSC-U's loader (SCUS_973.53) carries its game at the same file offset, in the same records.
 *
 * The port's loader (main.cpp) placed SCES_524.56 as it is; this does the unpacking, then the
 * user-mode steps of crt0. The port owns the thread, the stack and the heap, and the payload's
 * bss records are zeros. LoadExecPS2 comes back here to start the multiplayer executable, or this
 * one again (exec.c).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exec.h"
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac3_version.h"

int func_00124B90(void);              /* _InitSys */
void func_0011F0A0(int a0);           /* FlushCache */
void func_0013B330(int a0, gaddr a1); /* main */
void rac3_install_tools(void);        /* tools.c */
void rac3_install_camera_latch(void); /* camera_latch.c */
void openrac_game_register_functions(void);
void rac3_program_set_overlay(int overlay); /* program.c */

/* Where the loader's payload is: file offset 0x7080 of an executable whose one segment is file
 * offset 0x1000 at 0x00800000 (exec.c reads it from the file itself). */
#define RAC3_PAYLOAD 0x00806080u
#define RAC3_ARGUMENTS 0x0015C040u /* crt0's argument block: argc, then argv */

static uint32_t word_at(const uint8_t* p) {
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

/* The loader's work, at the first boot: the payload's records copied to their addresses from the
 * loader the port's loader placed (main.cpp). */
static void unpack_payload(void) {
    if (rac3_exec_unpack_payload((const uint8_t*)G(RAC3_PAYLOAD)) == 0
        || word_at((const uint8_t*)G(0x0013ADA8u)) == 0) {
        fprintf(
            stderr,
            "rac3: the disc's loader holds no game at 0x%08x (not Up Your Arsenal's?)\n",
            RAC3_PAYLOAD
        );
        exit(1);
    }
}

/* Back to the boot program from the multiplayer one (its LoadExecPS2 of SCES_524.56): the
 * executable read from the disc again and unpacked over memory, its functions in place of the
 * multiplayer program's, the arguments it was started with in crt0's block. */
static void reload_boot_program(void) {
    uint32_t size = 0;
    uint8_t* file = rac3_exec_read_disc_file(RAC3_BOOT_FILE, &size);
    if (file == NULL) {
        fprintf(stderr, "rac3: %s cannot be read from the disc image\n", RAC3_BOOT_FILE);
        exit(1);
    }
    memset(G(0x00100000u), 0, 0x02000000u - 0x00100000u);
    openrac_guest_unregister_all();
    openrac_game_kernel_reset();
    openrac_snd_ring_reset();
    openrac_guest_clear_level_programs();
    openrac_game_register_functions();
    rac3_program_set_overlay(OPENRAC_OVERLAY_EXE);
    if (rac3_exec_unpack(file, size) == 0) {
        fprintf(stderr, "rac3: %s on the disc holds no game\n", RAC3_BOOT_FILE);
        exit(1);
    }
    free(file);
    rac3_exec_write_arguments(RAC3_ARGUMENTS);
}

/* InitSystemCallTableAddress (libkernl's initsys, called from _InitSys; rac2-ntsc's
 * func_0011F718): it patches two kernel routines through SetSyscall, then scans kernel memory with
 * FindAddress until two searches meet, to record where the kernel's table of system calls is. The
 * port has no kernel memory, so the scan would never end; there is nothing for it to find. */
void func_00124A80(void) {}

void rac3_menu_log_install(void); /* menu_log.c */
void rac3_hero_log_install(void); /* hero_log.c */

/* LoadExecPS2's wrapper (0x00124ED8: the argument block 0x00124DB8 builds, then syscall 6): the
 * multiplayer executable, or the boot program again (exec.c). */
void func_00124ED8(unsigned int path, int argc, unsigned int argv) {
    rac3_load_exec(path, argc, argv);
}

int func_0013ADA8(void) {
    static gaddr stack;
    uint32_t argc;
    rac3_install_tools();
    rac3_install_camera_latch();
    unpack_payload();
    rac3_menu_log_install();
    rac3_hero_log_install();
    openrac_game_draw(0);
    stack = openrac_guest_sp;
    /* LoadExecPS2 comes back here, the stack of the program that called it dropped. */
    if (setjmp(*rac3_exec_jump()) != 0) {
        openrac_guest_sp = stack;
        if (rac3_exec_program() == RAC3_EXEC_MULTIPLAYER) {
            rac3_mp_run();
            return 0;
        }
        reload_boot_program();
    }
    func_00124B90();  /* _InitSys */
    func_0011F0A0(0); /* FlushCache(0) */
    memcpy(&argc, G(RAC3_ARGUMENTS), sizeof argc);
    func_0013B330((int)argc, RAC3_ARGUMENTS + 4u);
    return 0;
}
