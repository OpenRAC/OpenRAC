#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "core/sif/reset_iop.h"

/*
 * core_text object 0x11DD68-0x11DDD0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), exit.o: kCopy (the byte
 * version), a TLB re-init stub and Exit. Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libkernl.a, which emits
 * both of its tail calls natively.
 */

/*
 * exit.o's kCopy: a byte copy loop returning 0. Exact under 2.9-ee, which
 * pads the short loop itself and leaves the branch's delay slot empty,
 * with `dst++` before the branch as retail has it. (Under 2.95.3 reorg
 * sinks `dst++` into the slot, 44 bytes against 48: the old revert.)
 */
int exit_kCopy(char *dst, char *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++) {
        *dst++ = *src++;
    }
    return 0;
}

/* Tail call: retail is `j InitTLB` + nop, with no frame at all.
   2.9-ee emits it natively for a void function that ends in a call. */
void exit_reinit_tlb(void) {
    InitTLB();
}

/*
 * Exit(status): re-init the TLB (exit_reinit_tlb), then the _Exit syscall
 * (kernel_exit) with the status, as a bare tail jump. Exact under
 * 2.9-ee, which emits "call, then tail call" itself; under 2.95.3 it was
 * 4 bytes over (2.95.3 has no sibling calls). The status lives in $16
 * across the first call.
 */

void Exit(int status) {
    exit_reinit_tlb();
    kernel_exit(status);
}

LINKER_REMNANT("asm/remnants/core_text", func_0011DDC8);
