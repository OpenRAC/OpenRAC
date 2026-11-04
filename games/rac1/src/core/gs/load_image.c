#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x122818-0x122AD8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph016.o (sceGsSetDefStoreImage) and graph017.o
 * (sceGsExecLoadImage), back to back. Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
 */


extern void scePrintf();
typedef int u128_sd __attribute__((mode(TI)));
typedef struct {
    unsigned long NLOOP : 15;
    unsigned long EOP : 1;
    unsigned long pad16 : 30;
    unsigned long PRE : 1;
    unsigned long PRIM : 11;
    unsigned long FLG : 2;
    unsigned long NREG : 4;
    unsigned long REGS0 : 4;
    unsigned long REGS1 : 4;
    unsigned long REGS2 : 4;
    unsigned long REGS3 : 4;
    unsigned long REGS4 : 4;
    unsigned long REGS5 : 4;
    unsigned long REGS6 : 4;
    unsigned long REGS7 : 4;
    unsigned long REGS8 : 4;
    unsigned long REGS9 : 4;
    unsigned long REGS10 : 4;
    unsigned long REGS11 : 4;
    unsigned long REGS12 : 4;
    unsigned long REGS13 : 4;
    unsigned long REGS14 : 4;
    unsigned long REGS15 : 4;
} SdGifTag __attribute__((aligned(16)));
typedef struct {
    unsigned long SBP : 14;
    unsigned long pad14 : 2;
    unsigned long SBW : 6;
    unsigned long pad22 : 2;
    unsigned long SPSM : 6;
    unsigned long pad30 : 2;
    unsigned long DBP : 14;
    unsigned long pad46 : 2;
    unsigned long DBW : 6;
    unsigned long pad54 : 2;
    unsigned long DPSM : 6;
    unsigned long pad62 : 2;
} SdBitbltbuf;
typedef struct {
    unsigned long SSAX : 11;
    unsigned long pad11 : 5;
    unsigned long SSAY : 11;
    unsigned long pad27 : 5;
    unsigned long DSAX : 11;
    unsigned long pad43 : 5;
    unsigned long DSAY : 11;
    unsigned long DIR : 2;
    unsigned long pad61 : 3;
} SdTrxpos;
typedef struct {
    unsigned long RRW : 12;
    unsigned long pad12 : 20;
    unsigned long RRH : 12;
    unsigned long pad44 : 20;
} SdTrxreg;
typedef struct {
    unsigned long pad00;
} SdFinish;
typedef struct {
    unsigned long XDR : 2;
    unsigned long pad02 : 62;
} SdTrxdir;
typedef struct {
    unsigned int vifcode[4];
    SdGifTag giftag;
    SdBitbltbuf bitbltbuf;
    long bitbltbufaddr;
    SdTrxpos trxpos;
    long trxposaddr;
    SdTrxreg trxreg;
    long trxregaddr;
    SdFinish finish;
    long finishaddr;
    SdTrxdir trxdir;
    long trxdiraddr;
} SdStoreImage __attribute__((aligned(16)));

int sceGsSetDefStoreImage(SdStoreImage *sp, short sbp, short sbw, short spsm, short x, short y, short w, short h) {
    sp->vifcode[0] = 0;
    *(unsigned long *)&sp->finish = 0;
    sp->vifcode[1] = 0x06008000;
    sp->vifcode[2] = 0x13000000;
    sp->vifcode[3] = 0x50000006;
    *(u128_sd *)&sp->giftag = (u128_sd)0;
    sp->giftag.NLOOP = 5;
    sp->giftag.EOP = 1;
    sp->giftag.NREG = 1;
    sp->giftag.REGS0 = 0xE;
    *(unsigned long *)&sp->bitbltbuf = (unsigned long)sbp | ((unsigned long)sbw << 16) | ((unsigned long)spsm << 24);
    sp->bitbltbufaddr = 0x50;
    *(unsigned long *)&sp->trxpos = (unsigned long)x | ((unsigned long)y << 16);
    sp->trxposaddr = 0x51;
    *(unsigned long *)&sp->trxreg = (unsigned long)w | ((unsigned long)h << 32);
    sp->trxregaddr = 0x52;
    sp->finishaddr = 0x61;
    *(unsigned long *)&sp->trxdir = 1;
    sp->trxdiraddr = 0x53;
    CpuSync();
    return 7;
}

extern void scePrintf(void *);
extern char gs_exec_load_image_dma_timeout_msg[];

typedef struct {
    long q[12];
} GsLoadImage;

/* sceGsExecLoadImage(src, dst): waits for the GS transfer unit
 * (0x1000A000 bit 8) to go idle, starts an image load from src (its
 * 0x70000000 scratchpad segment tagged with bit 31) sized by the 15-bit
 * field in src->q[10], waits again and starts the transfer to dst.
 * Returns -1 after 0x1000000 polls in total. The abort is written out in
 * each loop, not shared through a goto: the compiler merges the two copies
 * into retail's single call, where a shared label needs an extra branch. */
int sceGsExecLoadImage(GsLoadImage *src, void *dst) {
    unsigned int count = 0;

    while (*(volatile unsigned int *)0x1000A000 & 0x100) {
        if (count++ > 0x1000000) {
            scePrintf(gs_exec_load_image_dma_timeout_msg);
            return -1;
        }
    }

    *(volatile unsigned int *)0x1000A020 = 6;
    if (((unsigned int)src & 0x70000000) == 0x70000000) {
        *(volatile unsigned int *)0x1000A010 = ((unsigned int)src & 0x0FFFFFFF) | 0x80000000;
    } else {
        *(volatile unsigned int *)0x1000A010 = (unsigned int)src & 0x0FFFFFFF;
    }
    *(volatile unsigned int *)0x1000A000 = 0x101;

    while (*(volatile unsigned int *)0x1000A000 & 0x100) {
        if (count++ > 0x1000000) {
            scePrintf(gs_exec_load_image_dma_timeout_msg);
            return -1;
        }
    }

    *(volatile unsigned int *)0x1000A020 = (unsigned int)(int)(src->q[10] & 0x7FFF);
    if (((unsigned int)dst & 0x70000000) == 0x70000000) {
        *(volatile unsigned int *)0x1000A010 = ((unsigned int)dst & 0x0FFFFFFF) | 0x80000000;
    } else {
        *(volatile unsigned int *)0x1000A010 = (unsigned int)dst & 0x0FFFFFFF;
    }
    *(volatile unsigned int *)0x1000A000 = 0x101;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00122AD4);
