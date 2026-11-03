#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/cdvd/read.h"

/*
 * core_text object 0x122140-0x122200. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph004.o (sceGsPutDispEnv). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
 */

typedef struct {
    unsigned long f0;
    unsigned long f8;
    unsigned long f10;
    unsigned long f18;
    unsigned long f20;
} GsDispEnv122140;

/* Push a GS display-environment struct's quadword fields to the GS
 * privileged registers, picking the register set (and whether f8 is
 * used at all) by the field/mode flag at offset 6 of CoreDataTable
 * (sceGsGetGParam's return). */
void sce_gs_put_disp_env(GsDispEnv122140 *env) {
    void *info = sceGsGetGParam();

    if (*(short *)((char *)info + 6) == 1) {
        *(volatile unsigned long *)0x12000000 = env->f0;
        *(volatile unsigned long *)0x12000070 = env->f10;
        *(volatile unsigned long *)0x12000080 = env->f18;
        *(volatile unsigned long *)0x120000C0 = env->f20;
    } else {
        *(volatile unsigned long *)0x12000000 = env->f0;
        *(volatile unsigned long *)0x12000020 = env->f8;
        *(volatile unsigned long *)0x12000090 = env->f10;
        *(volatile unsigned long *)0x120000A0 = env->f18;
        *(volatile unsigned long *)0x120000E0 = env->f20;
    }
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001221FC);
