#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/cdvd/read.h"
#include "core/kernel/libkernl_glue.h"

/*
 * core_text object 0x1224B0-0x122630. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph008.o (sceGsPutDrawEnv) and graph011.o
 * (sceGsSyncV), back to back. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libgraph.a.
 */

extern void scePrintf();

extern char gs_put_draw_env_dma_timeout_msg[];

/* Kicks a DMA transfer on the GIF channel (D2 at 0x1000A000), waiting
   for the channel with a timeout. Every register access is volatile. */
int sceGsPutDrawEnv(void *arg0) {
    int p = (int)arg0;
    unsigned int i;

    i = 0;
    while (*(volatile int *)0x1000A000 & 0x100) {
        if (i++ > 0x1000000) {
            scePrintf(gs_put_draw_env_dma_timeout_msg);
            return -1;
        }
    }
    *(volatile int *)0x1000A020 = (int)(*(long *)arg0 & 0x7FFF) + 1;
    if ((p & 0x70000000) == 0x70000000) {
        *(volatile int *)0x1000A010 = (p & 0x0FFFFFFF) | 0x80000000;
    } else {
        *(volatile int *)0x1000A010 = p & 0x0FFFFFFF;
    }
    *(volatile int *)0x1000A000 = 0x101;
    return 0;
}

/* libgraph's sceGsGParam, as far as this reads it. */
typedef struct {
    short interlace; /* 0x00 */
    short omode;     /* 0x02 */
    short ffmode;    /* 0x04 */
    short version;   /* 0x06 */
    int unk_08;      /* 0x08 */
} GsGParam;

/* Returns the current field, bit 13 of the GS CSR (read directly at
   0x12001000, or through wait_vblank_csr), when the display is
   interlaced, and 1 otherwise. In the second arm the bit is taken from
   the call's result before the interlace test, as retail does. */
int sce_gs_sync_v(void) {
    GsGParam *gp = sceGsGetGParam();
    long csr;

    if (gp->unk_08 == 0) {
        wait_vblank_start();
        if (gp->interlace != 1) {
            return 1;
        }
        return (*(volatile unsigned long *)0x12001000 >> 13) & 1;
    }
    csr = (wait_vblank_csr() >> 13) & 1;
    if (gp->interlace != 1) {
        return 1;
    }
    return csr;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012262C);
