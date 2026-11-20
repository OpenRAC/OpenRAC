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
 * The port's loader (main.cpp) placed SCES_524.56 as it is; this does the unpacking, then the
 * user-mode steps of crt0. The port owns the thread, the stack and the heap, and the payload's
 * bss records are zeros.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

uint32_t openrac_lib_wad_decompress(const uint8_t* src, uint8_t* dst, uint32_t capacity);

int func_00124B90(void);              /* _InitSys */
void func_0011F0A0(int a0);           /* FlushCache */
void func_0013B330(int a0, gaddr a1); /* main */
void rac3_install_tools(void);        /* tools.c */
void rac3_install_camera_latch(void); /* camera_latch.c */

/* Where the loader's payload is: file offset 0x7080 of an executable whose one segment is file
 * offset 0x1000 at 0x00800000. */
#define RAC3_PAYLOAD 0x00806080u
#define RAC3_PAYLOAD_MAX 0x00400000u

static uint32_t word_at(const uint8_t* p) {
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

/* The loader's work: the payload's records copied to their addresses. */
static void unpack_payload(void) {
    uint8_t* image = (uint8_t*)malloc(RAC3_PAYLOAD_MAX);
    if (image == NULL) {
        fprintf(stderr, "rac3: no memory to unpack the game\n");
        exit(1);
    }
    const uint32_t size =
        openrac_lib_wad_decompress((const uint8_t*)G(RAC3_PAYLOAD), image, RAC3_PAYLOAD_MAX);
    if (size < 16) {
        fprintf(
            stderr,
            "rac3: SCES_524.56 holds no game at 0x%08x (not the PAL disc's loader?)\n",
            RAC3_PAYLOAD
        );
        exit(1);
    }
    uint32_t at = 0;
    while (at + 16 <= size) {
        const uint32_t address = word_at(image + at), bytes = word_at(image + at + 4),
                       kind = word_at(image + at + 8);
        if (bytes > size - at - 16) {
            break;
        }
        /* Code and data (1) and zeros (8); the register-use records (0x70000006) load nowhere. */
        if ((kind == 1u || kind == 8u) && address + bytes <= 0x02000000u) {
            memcpy(G(address), image + at + 16, bytes);
        }
        at += 16 + bytes;
    }
    free(image);
}

/* InitSystemCallTableAddress (libkernl's initsys, called from _InitSys; rac2-ntsc's
 * func_0011F718): it patches two kernel routines through SetSyscall, then scans kernel memory with
 * FindAddress until two searches meet, to record where the kernel's table of system calls is. The
 * port has no kernel memory, so the scan would never end; there is nothing for it to find. */
void func_00124A80(void) {}

void rac3_menu_log_install(void); /* menu_log.c */

int func_0013ADA8(void) {
    uint32_t argc;
    rac3_install_tools();
    rac3_install_camera_latch();
    unpack_payload();
    rac3_menu_log_install();
    openrac_game_draw(0);
    func_00124B90();  /* _InitSys */
    func_0011F0A0(0); /* FlushCache(0) */
    memcpy(&argc, G(0x0015C040u), sizeof argc);
    func_0013B330((int)argc, 0x0015C044u);
    return 0;
}
