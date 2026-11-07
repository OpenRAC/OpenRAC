#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "core/cdvd/read.h"

/*
 * core_text object 0x123168-0x1236F0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Two Sony libraries back to back, both built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE): libgraph's sceGsSyncVCallback (sceGsSyncVCallback,
 * the last of libgraph's modules, which end 8-byte aligned so no fill
 * separates them) and the start of libdma ("libdma: sync timeout";
 * sceDmaGetChan, sceDmaReset, sceDmaPutEnv, sceDmaPause, ...).
 */

extern void scePrintf();

/*
 * libgraph's sceGsSyncVCallback: install (or, called with arg0 == 0,
 * uninstall) the V-sync handler at offset 8/0xC of CoreDataTable
 * (sceGsGetGParam's sceGsGetGParam -- the same global sce_gs_put_disp_env reads
 * the mode flag from), under DisableIntc/EnableIntc (disable_intc_atomic/
 * enable_intc_atomic) of INTC 2 (VBLANK start) around the AddIntcHandler/
 * RemoveIntcHandler syscalls (add_intc_handler/remove_intc_handler). Returns the
 * previous handler.
 *
 * Exact under 2.9-ee as written. Under 2.95.3 it was 8/0xA0: that
 * compiler set up a0 (lvl) before a1 (arg0) for the add_intc_handler call
 * whatever the spelling, where retail (and 2.9-ee) has a1 first.
 */
int sceGsSyncVCallback(void *arg0) {
    char *g = (char *)sceGsGetGParam();
    int lvl = 2;
    int prev = *(int *)(g + 8);

    if (arg0 == 0) {
        disable_intc_atomic(lvl);
        remove_intc_handler(lvl, *(int *)(g + 0xC));
        *(int *)(g + 0xC) = 0;
        *(int *)(g + 8) = 0;
    } else {
        if (prev != 0) {
            disable_intc_atomic(lvl);
            remove_intc_handler(lvl, *(int *)(g + 0xC));
        }
        *(int *)(g + 8) = (int)arg0;
        *(int *)(g + 0xC) = add_intc_handler(lvl, arg0, -1);
        enable_intc_atomic(lvl);
    }
    return prev;
}

extern int sceDmaPause(void *);
extern char dma_sync_timeout_msg[];

/*
 * Wait for a DMA channel: spin while CHCR.STR (bit 8 of *arg0) is set;
 * after 0x1000000 polls report through scePrintf(dma_sync_timeout_msg,
 * "libdma: sync timeout") and sceDmaPause the channel on
 * every further poll.
 *
 * The countdown starts at 0x1000000 and is decremented before the test:
 * retail's `lui 0x100; addiu -1` (splat's "D_FFFFFF") is that first
 * n-- folded into the constant, and reorg copies the loop's n-- into the
 * bnez slot. The old reading (0xFFFFFF, `if (n < 0)` then `n--`) gave
 * `lui 0xff; ori 0xffff` and was 24/112. The channel word is read
 * volatile, as the hardware register it is.
 */
void dma_wait_channel(void *arg0) {
    int n = 0x1000000;

    while ((*(volatile int *)arg0 & 0x100) != 0) {
        if (--n < 0) {
            scePrintf(dma_sync_timeout_msg);
            sceDmaPause(arg0);
        }
    }
}

LINKER_REMNANT("asm/remnants/core_text", func_00123278);

int dma_scratchpad_address(int arg0) {
    if ((unsigned int)arg0 >> 28 == 7) {
        arg0 &= 0x0FFFFFFF;
        arg0 |= 0x80000000;
    }
    return arg0;
}

/* Zero-fill n bytes at dst.

   The plain post-decrement loop: the counter becomes a new pseudo (n - 1
   in $v0, -1 in $v1) instead of the parameter decremented in place, and
   the empty delay slot is 2.9-ee itself -- it pads the short loop with
   nops in its own output and leaves the bne unfilled, so the assembler
   puts the nop in the slot. That is the "unfilled short-loop slot" seen
   elsewhere in core_text. Under 2.95.3 the same C gets the registers
   right but reorg fills the slot (6/52); the old do-while was 2/52. */
void dma_memclr(char *dst, int n) {
    while (n-- != 0) {
        *dst++ = 0;
    }
}

extern int DmaChannels[];

/* sceDmaGetChan: the channel's register block, 0 for an id past the ten
   channels. The in-range return comes first under 2.9-ee (the
   `>= 0xA` guard-first form matched 2.95.3 and is 21/40 here). */
int sce_dma_get_chan(unsigned int arg0) {
    if (arg0 < 0xA) {
        return DmaChannels[arg0];
    }
    return 0;
}

/*
 * sceDmaReset(mode): ret = D_CTRL.DMAE; for each of the ten channels
 * whose flag (DmaChannelsUsed[i]) is set, zero chcr/tadr/madr/as1/as0/sadr of
 * the channel's registers (DmaChannels[i]); D_STAT = 0xFF1F, then
 * D_STAT &= 0xFF1F0000; zero a 0x14-byte sceDmaEnv (dma_memclr) and
 * sceDmaPutEnv it (sceDmaPutEnv); if mode == 1 set DMAE. Exact under
 * both compilers.
 *
 * Levers (build-sn/try/sceDmaReset/RESULT.md):
 *  - an index loop: loop.c strength-reduces it into the two pointers and
 *    the down-counter itself, in retail's init order (the old
 *    hand-written two-pointer loop swapped them);
 *  - the MMIO spelled inline as `*(volatile unsigned int *)0x1000E0x0`
 *    (a pointer local becomes absolute `$at` addressing: the old 228);
 *  - the two D_STAT stores are not volatile, only its read, so the second
 *    store can go into the jal slot as in retail (a volatile store never
 *    does);
 *  - sceDmaPutEnv returns int: the `li 1` after it is in $v1.
 */
extern int DmaChannelsUsed[];
extern int sceDmaPutEnv(void *);

int sceDmaReset(int arg0) {
    char buf[0x14];
    int flag;
    int i;

    flag = *(volatile unsigned int *)0x1000E000 & 1;

    for (i = 0; i < 10; i++) {
        if (DmaChannelsUsed[i] != 0) {
            int *p = (int *)DmaChannels[i];
            p[0] = 0;
            p[0xC] = 0;
            p[4] = 0;
            p[0x14] = 0;
            p[0x10] = 0;
            p[0x20] = 0;
        }
    }

    *(unsigned int *)0x1000E010 = 0xFF1F;
    *(unsigned int *)0x1000E010 = *(volatile unsigned int *)0x1000E010 & 0xFF1F0000;

    dma_memclr(buf, 0x14);
    sceDmaPutEnv(buf);

    if (arg0 == 1) {
        *(volatile unsigned int *)0x1000E000 = *(volatile unsigned int *)0x1000E000 | 1;
    }
    return flag;
}

typedef struct {
    unsigned char sts;
    unsigned char std;
    unsigned char mfd;
    unsigned char rcyc;
    unsigned short express;
    unsigned short notify;
    unsigned short sqwc;
    unsigned short tqwc;
    void *rbadr;
    unsigned int rbmsk;
} sceDmaEnv;
extern unsigned char dma_sts_channel_codes[];
extern unsigned char dma_std_control_encodings[];
extern unsigned char dma_mfd_control_encodings[];
extern sceDmaEnv dma_last_applied_environment;

/* sceDmaPutEnv. Adapted from Lombyte's project (GPL-3.0) for PAL. */
int sceDmaPutEnv(void *arg0) {
    sceDmaEnv *env = arg0;
    unsigned int ctrl;
    unsigned int pcr;
    unsigned int sqwc;
    unsigned int rbor;
    unsigned int rbsr;

    ctrl = *(volatile unsigned int *)0x1000E000;
    pcr = *(volatile unsigned int *)0x1000E020;
    sqwc = *(volatile unsigned int *)0x1000E030;
    rbor = *(volatile unsigned int *)0x1000E050;
    rbsr = *(volatile unsigned int *)0x1000E040;
    if (env->sts >= 10) {
        return -1;
    }
    if (env->std >= 10) {
        return -2;
    }
    if (env->mfd >= 10) {
        return -3;
    }
    if (env->rcyc >= 7) {
        return -4;
    }
    ctrl = (ctrl & ~0x30) | (dma_sts_channel_codes[env->sts] << 4);
    ctrl = (ctrl & ~0xC0) | (dma_std_control_encodings[env->std] << 6);
    ctrl = (ctrl & ~0xC) | (dma_mfd_control_encodings[env->mfd] << 2);
    if (env->rcyc != 0) {
        ctrl |= 2;
        ctrl = (ctrl & ~0x300) | ((env->rcyc - 1) << 8);
    } else {
        ctrl &= ~2;
    }
    pcr = (env->express << 16) | env->notify;
    sqwc = (env->tqwc << 16) | env->sqwc;
    rbor = (unsigned int)env->rbadr;
    rbsr = env->rbmsk;
    *(volatile unsigned int *)0x1000E000 = ctrl;
    *(volatile unsigned int *)0x1000E020 = pcr;
    *(volatile unsigned int *)0x1000E030 = sqwc;
    *(volatile unsigned int *)0x1000E050 = rbor;
    *(volatile unsigned int *)0x1000E040 = rbsr;
    dma_last_applied_environment = *env;
    return 0;
}

LINKER_REMNANT("asm/remnants/core_text", func_001235C0);

extern int dma_scratchpad_address(int);
extern void dma_wait_channel(void *);

/* Reset the record at p with dma_wait_channel, keep dma_scratchpad_address(arg1) in
   p[12] unless it holds -1, clear p[8], and set flags 0x105 in p[0] with
   bits 2-3 cleared. The -1 test is unsigned, as retail builds 0xFFFFFFFF. */
void sce_dma_send(int *p, int arg1) {
    int r = dma_scratchpad_address(arg1);

    dma_wait_channel(p);
    if ((unsigned int)p[12] != 0xFFFFFFFFU) {
        p[12] = r;
    }
    p[8] = 0;
    p[0] = (p[0] & ~0xC) | 0x105;
}

LINKER_REMNANT("asm/remnants/core_text", func_00123630);

/*
 * sceDmaPause: DI (disable_interrupts), force D_ENABLEW.CPND (0x10000) if
 * D_ENABLER has it clear, read D_CTRL (a volatile read for its side
 * effect only), clear the channel's CHCR.STR (bit 8), restore
 * D_ENABLEW, EI (enable_interrupts) if DI said interrupts were on, and
 * return the old CHCR.
 *
 * Exact under 2.9-ee. The mask has to be an unsigned constant
 * (`old & 0xFFFFFEFF`; `~0x100` is one addiu, SIZE 148), and the channel
 * word goes through a `volatile unsigned int *`, which keeps its load
 * after the D_CTRL read and gives retail's $4-$7 allocation. Under
 * 2.95.3 the same C differs in the prologue order only (8/152).
 */
int sceDmaPause(void *arg0) {
    volatile unsigned int *d = arg0;
    int en;
    unsigned int prev, old;
    en = disable_interrupts();
    prev = *(volatile unsigned int *)0x1000F520;
    if ((prev & 0x10000) == 0)
        *(volatile unsigned int *)0x1000F590 = prev | 0x10000;
    *(volatile unsigned int *)0x1000E000;
    old = *d;
    *d = old & 0xFFFFFEFF;
    *(volatile unsigned int *)0x1000F590 = prev;
    if (en != 0)
        enable_interrupts();
    return old;
}

LINKER_REMNANT("asm/remnants/core_text", func_001236E8);
