#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x122630-0x122818. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph015.o (sceGsSetDefLoadImage). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
 */


extern void scePrintf();

extern void scePrintf(void *);
extern char gs_load_image_too_big_msg[];

typedef int u128_li __attribute__((mode(TI)));
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
} LiGifTag __attribute__((aligned(16)));
typedef struct {
    LiGifTag giftag0;
    unsigned long bitbltbuf;
    long bitbltbufaddr;
    unsigned long trxpos;
    long trxposaddr;
    unsigned long trxreg;
    long trxregaddr;
    unsigned long trxdir;
    long trxdiraddr;
    LiGifTag giftag1;
} LiLoadImage __attribute__((aligned(16)));

int sceGsSetDefLoadImage(LiLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y, short w, short h) {
    int size = 0;

    switch (dpsm) {
        case 0x00:
        case 0x30:
            size = (w * h) >> 2;
            break;
        case 0x01:
        case 0x31:
            size = (w * h * 3) >> 4;
            break;
        case 0x02:
        case 0x0A:
        case 0x32:
        case 0x3A:
            size = (w * h) >> 3;
            break;
        case 0x13:
        case 0x1B:
            size = (w * h) >> 4;
            break;
        case 0x14:
        case 0x24:
        case 0x2C:
            size = (w * h) >> 5;
            break;
    }
    if (size > 0x7FFF) {
        scePrintf(gs_load_image_too_big_msg);
        return 0;
    }
    *(u128_li *)&lp->giftag0 = (u128_li)0;
    lp->giftag0.NLOOP = 4;
    lp->giftag0.NREG = 1;
    lp->giftag0.REGS0 = 0xE;
    lp->bitbltbuf = ((unsigned long)dbp << 32) | ((unsigned long)dbw << 48) | ((unsigned long)dpsm << 56);
    lp->bitbltbufaddr = 0x50;
    lp->trxpos = ((unsigned long)x << 32) | ((unsigned long)y << 48);
    lp->trxposaddr = 0x51;
    lp->trxreg = (unsigned long)w | ((unsigned long)h << 32);
    lp->trxregaddr = 0x52;
    lp->trxdir = 0;
    lp->trxdiraddr = 0x53;
    *(u128_li *)&lp->giftag1 = (u128_li)0;
    lp->giftag1.NLOOP = size;
    lp->giftag1.EOP = 1;
    lp->giftag1.FLG = 2;
    CpuSync();
    return 6;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00122814);
