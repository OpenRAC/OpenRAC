/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * _start (crt0, func_0012D728): the program's entry point (the ELF's e_entry, hand-written
 * assembly the console's loader jumps to). The port never enters it: main.cpp sets the guest up
 * itself (the registers, the program's memory, one thread whose stack is outside the game's RAM)
 * and starts the game at main (hostgen.json "roots"). This is what crt0 does, in the port's
 * terms, for a start that went through it (libraries.tsv binds it; PAL only: the addresses are
 * this program's):
 *
 *   - every register cleared: the port's guest has no register file;
 *   - .bss cleared, a quadword at a time from D_00154100 while the address is below D_0015ED28
 *     (the store comes before the test, so the quadword at the first address past the end is
 *     cleared too: up to 0x0015ED40);
 *   - SetupThread (system call 0x3C: $gp 0x00166D00, the stack's top 0x01FFC000 and size 0x4000,
 *     the loader's argument block D_0015EB40, _exit func_0012D8F0 as the main thread's return) and
 *     SetupHeap (0x3D: 0x01FF8000, 0x4000): the port's kernel layer has its one thread and its
 *     heap limit (EndOfHeap) already;
 *   - _InitSys (func_0011D8A8), FlushCache(0), interrupts on (the port takes none);
 *   - main (func_0012D9D8) with argc and argv from the argument block, then Exit (func_0011DAA0)
 *     with main's result: main never returns (GameBootSequence, then each next program).
 */
#include <string.h>

#include "rac1.h"

#define CRT0_BSS_START 0x00154100u /* D_00154100 */
#define CRT0_BSS_END 0x0015EC28u   /* D_0015ED28 */

void func_0012D728(void) {
    gaddr p = CRT0_BSS_START;
    for (;;) {
        memset(G(p), 0, 16);
        if (!(p < CRT0_BSS_END)) {
            break;
        }
        p += 16;
    }
    func_0011D8A8();  /* _InitSys */
    func_00118A80(0); /* FlushCache(0) */
    func_0012D9D8();  /* main(GREF(int, 0x0015EB40), 0x0015EB44): the port's main takes none */
    func_0011DAA0(0); /* Exit */
}
