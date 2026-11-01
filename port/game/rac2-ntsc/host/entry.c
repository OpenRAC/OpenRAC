/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native counterpart to Going Commando's ELF start-up at 0x00131AE8 (crt0).
 *
 * The retail start-up clears .bss, calls the kernel's InitMainThread and
 * SetupHeap system calls ($gp 0x001AEFF0, stack below 0x01FFC000, heap end
 * 0x01FF8000), then _InitSys (0x0011F828), FlushCache(0) and main(argc,
 * argv) with the argument block at 0x00146B40, and exit with main's result.
 * The port owns the thread, the stack and the memory, and the loader has
 * zeroed .bss, so only the user-mode steps are kept here.
 */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

void func_0011F828(void);
void func_0011AEA0(int cache);
int func_00131D98(int argc, int argv);
void rac2_install_tools(void); /* tools.c */

/* InitSystemCallTableAddress (libkernl's initsys, called from _InitSys): it patches
 * two kernel routines through SetSyscall, then scans kernel memory with
 * FindAddress until two searches meet, to record where the kernel's table of
 * system calls is. The port has no kernel memory (FindAddress finds nothing),
 * so the scan would never end; there is nothing for it to find. */
void func_0011F718(void) {
}

/* The Deci2 console set-up of _InitSys (libkernl, rac1-decomp's func_001195A0):
 * it creates a semaphore and starts a thread that serves the debugger's TTY
 * protocol through Deci2Call. There is no debugger station and threads do not
 * run in the port; the retail routine returns -1 when the thread cannot be
 * made, and _InitSys ignores the result. */
int func_0011B800(void) {
    return -1;
}

/* A widget's draw names the function at 0x002DC908: the catalogue starts it at 0x002DC900, on the
 * end of the code before it. */
int func_002DC900(gaddr box);

static const openrac_fn_entry entries[] = {
    {0x002DC908u, (openrac_host_fn)func_002DC900, "func_002DC900"},
};

int func_00131AE8(void) {
    openrac_guest_register(OPENRAC_OVERLAY_EXE, entries, sizeof entries / sizeof entries[0]);
    /* The game reports what it draws (host/render.c), so the window draws no world until a pass
     * of DrawWorld runs: none at the title and the main menu, which are the 2D path alone. */
    openrac_game_draw(0);
    uint32_t argc;

    rac2_install_tools();
    func_0011F828(); /* _InitSys */
    func_0011AEA0(0); /* FlushCache(0) */
    memcpy(&argc, G(0x00146B40u), sizeof argc);
    return func_00131D98((int)argc, 0x00146B44);
}
