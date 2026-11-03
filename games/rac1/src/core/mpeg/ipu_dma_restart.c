#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/mpeg/ipu.h"

/*
 * core_text object 0x12CC70-0x12CC90. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Two libmpeg forwarders into the IPU DMA code of 0012CC90.c, each a bare
 * tail jump. Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), which
 * emits those tail calls itself for a void function that ends in a call.
 */

void mpeg_stop_ipu_dma(void *arg0) {
    sce_ipu_stop_dma((char *)*(void **)((char *)arg0 + 0x40) + 0x4C);
}

/* sce_ipu_restart_dma itself ends in a tail call (to set_d4_chcr): it returns
   nothing. */

/* Tail call with argument setup: retail is
   `lw $4,0x40($4)` / `j sce_ipu_restart_dma` / `addiu $4,$4,0x4C`. Void, so
   2.9-ee emits the tail jump (a `return` of the callee's value would
   keep a frame: 32 bytes against 12). */
void mpeg_restart_ipu_dma(char *a) {
    sce_ipu_restart_dma((char *)&((Wrapper *)a)->obj->handlers[8]);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012CC8C);
