#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x123168-0x1236F0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Two Sony libraries back to back, both built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE): libgraph's sceGsSyncVCallback (func_00123168,
 * the last of libgraph's modules, which end 8-byte aligned so no fill
 * separates them) and the start of libdma ("libdma: sync timeout";
 * sceDmaGetChan, sceDmaReset, sceDmaPutEnv, sceDmaPause, ...).
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;
extern void *_impure_ptr NOT_SDA;
extern int __sread(void *arg0, void *arg1, void *arg2);
extern int __swrite(void *arg0, void *arg1, void *arg2);
extern long __sseek(void *arg0, void *arg1, void *arg2);
extern void __sclose(void *arg0);
extern void libc_fflush(void);
extern void _fwalk(void *, void *);
extern char locale_lconv[];
extern int libc_glue_read();
extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");
extern int _close_r(int *errOut, int arg1);
extern int __sfvwrite(int, void *);
extern void __sinit(void *);
extern void _vfprintf_r(void *, void *, int, int);
extern int libc_glue_write();
extern int CoreGlobalWord NOT_SDA;
extern void Deci2Call(int arg0, void *arg1);
extern void *InitBlockData NOT_SDA;
extern int sif_sregs[];
extern void disable_dmac_atomic(int);
extern void remove_dmac_handler(int, int);
extern int sif_cmd_dmac_handler_id;
extern int sif_cmd_init_done;
extern int sif_cmd_data_sys_handlers NOT_SDA;
extern int sif_cmd_data_usr_handlers NOT_SDA;
extern void sif_send_cmd_common(int, int, int, int, int, int, int);
extern void sceSifExitCmd(void);
extern int RpcCommandState NOT_SDA;
extern int disable_interrupts(void);
extern void enable_interrupts(void);
extern int create_semaphore(void *);
extern int fs_iob_sema;
extern int fs_iob_sema_2;
extern char fs_iob_table[];
extern int fs_call_sema;
extern void fs_call_sema_init(void);
extern int fs_call_sema NOT_SDA;
extern char sif_lib_version_string[];
extern char FsResetBuffer[];
extern int fs_expected_version;
extern void libc_memset(void *, int, int);
extern int FsResetState;
extern int iop_heap_bind_state;
extern char iop_heap_rpc_client[];
extern int iop_heap_rpc_reply;
extern int iop_heap_rpc_arg;
extern char lf_iop_version[];
extern int lf_expected_version;
extern int _sceSifLoadModuleBuffer(int arg0, int arg1, int arg2, void *arg3);
extern void tty_reset_init_flag(void);
extern void init_tlb_32mb(void);
extern void kernel_init_tlb(void);
extern int FirstSemaphore;
extern int SecondSemaphore;
extern int osd_syscall_patch_table[];
extern char osd_kernel_patch_code[];
extern int PatchIsNeeded(void);
extern void flush_cache(int);
extern int sce_cd_sync_s(int);
extern void *cd_user_callback;
extern int cd_callback_sema;
extern int cd_ncmd_sema;
extern int cd_scmd_sema;
extern int cd_ncmd_busy;
extern int cd_poweroff_installing;
extern int cd_poweroff_installed;
extern int sif_call_rpc();
extern int cd_ncmd_prechk(int);
extern void signal_semaphore(int);
extern char cd_ncmd_rpc_client[];
extern int cd_ncmd_recv_buffer;
extern void scePrintf();
extern int sce_sif_check_stat_rpc(void *);
extern char cd_scmd_wait_msg[];
extern char cd_scmd_rpc_client[];
extern int cd_debug_level;
extern int cd_scmd_prechk(int);
extern int cd_scmd_recv_buffer;
extern char CoreDataTable[];

extern void *sceGsGetGParam(void);
extern int disable_intc_atomic(int);
extern int remove_intc_handler(int, int);
extern int add_intc_handler(int, void *, int);
extern void enable_intc_atomic(int);

/*
 * libgraph's sceGsSyncVCallback: install (or, called with arg0 == 0,
 * uninstall) the V-sync handler at offset 8/0xC of D_00132E40
 * (func_00121D08's sceGsGetGParam -- the same global func_00122140 reads
 * the mode flag from), under DisableIntc/EnableIntc (func_00119328/
 * func_00119390) of INTC 2 (VBLANK start) around the AddIntcHandler/
 * RemoveIntcHandler syscalls (func_00118A90/func_00118AA0). Returns the
 * previous handler.
 *
 * Exact under 2.9-ee as written. Under 2.95.3 it was 8/0xA0: that
 * compiler set up a0 (lvl) before a1 (arg0) for the func_00118A90 call
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
 * after 0x1000000 polls report through func_0011A6C8(D_001534E0,
 * "libdma: sync timeout") and sceDmaPause (func_00123650) the channel on
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
 * whose flag (D_001534F8[i]) is set, zero chcr/tadr/madr/as1/as0/sadr of
 * the channel's registers (D_00132E70[i]); D_STAT = 0xFF1F, then
 * D_STAT &= 0xFF1F0000; zero a 0x14-byte sceDmaEnv (func_001232A8) and
 * sceDmaPutEnv it (func_001233E8); if mode == 1 set DMAE. Exact under
 * both compilers.
 *
 * Levers (build-sn/try/func_00123308/RESULT.md):
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
extern unsigned char dma_std_channel_codes[];
extern unsigned char dma_mfd_channel_codes[];
extern sceDmaEnv dma_current_env;

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
    ctrl = (ctrl & ~0xC0) | (dma_std_channel_codes[env->std] << 6);
    ctrl = (ctrl & ~0xC) | (dma_mfd_channel_codes[env->mfd] << 2);
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
    dma_current_env = *env;
    return 0;
}

LINKER_REMNANT("asm/remnants/core_text", func_001235C0);

extern int dma_scratchpad_address(int);
extern void dma_wait_channel(void *);

/* Reset the record at p with func_00123208, keep func_00123280(arg1) in
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
 * sceDmaPause: DI (func_0011D960), force D_ENABLEW.CPND (0x10000) if
 * D_ENABLER has it clear, read D_CTRL (a volatile read for its side
 * effect only), clear the channel's CHCR.STR (bit 8), restore
 * D_ENABLEW, EI (func_0011D9A8) if DI said interrupts were on, and
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
    if (en != 0) enable_interrupts();
    return old;
}

LINKER_REMNANT("asm/remnants/core_text", func_001236E8);
