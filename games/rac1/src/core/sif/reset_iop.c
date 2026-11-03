#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "core/kernel/libkernl_glue.h"

/*
 * core_text object 0x11D0D0-0x11D700. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl): the IOP reset (sceSifRebootIop,
 * sce_sif_reboot_iop, "rom0:UDNL "), the boot-time kernel patch/poke sequence
 * (InitTLBFunctions) and the TLB setup (InitTLB/init_tlb_32mb,
 * "# TLB spad=0 kernel=1:%d ..."). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE).
 */

typedef struct {
    u64 psize : 8;
    u64 dsize : 24;
    u64 dest : 32;
    s32 cid;
    u32 opt;
} SifCmdHeader;

typedef struct {
    SifCmdHeader header;
    s32 arglen;
    s32 mode;
    char arg[80];
} SifResetPacket;

typedef struct {
    u32 data;
    u32 addr;
    s32 size;
    s32 mode;
} SifDmaData;

extern SifResetPacket sif_reset_iop_packet;

extern u32 sce_sif_get_reg_u(u32 reg) __asm__("func_00118E70");

extern void sceSifWriteBackDCache(void *p, s32 size);

/* sceSifResetIop: like sceSifRebootIop (sce_sif_reboot_iop) but with a caller
   -supplied argument string and reboot mode. Adapted from Lombyte's project (GPL-3.0)
   for PAL. */
s32 sceSifResetIop(const char *arg, s32 mode) {
    SifDmaData dma;
    u32 addr;
    s32 arglen;

    sce_sif_stop_dma();
    addr = sce_sif_get_reg_u(0x80000000);
    sif_reset_iop_packet.mode = mode;
    for (arglen = 0; arg[arglen] != 0; arglen++) {
        sif_reset_iop_packet.arg[arglen] = arg[arglen];
    }
    sif_reset_iop_packet.header.dest = 0;
    sif_reset_iop_packet.arglen = arglen;
    sif_reset_iop_packet.header.cid = 0x80000003;
    sif_reset_iop_packet.header.dsize = 0;
    sif_reset_iop_packet.header.psize = sizeof(SifResetPacket);
    dma.data = (u32)&sif_reset_iop_packet;
    dma.addr = addr;
    dma.size = sizeof(SifResetPacket);
    dma.mode = 0x44;
    sceSifWriteBackDCache(&sif_reset_iop_packet, sizeof(SifResetPacket));
    sce_sif_set_reg(4, 0x40000);
    if (sce_sif_set_dma(&dma, 1)) {
        sce_sif_set_reg(4, 0x10000);
        sce_sif_set_reg(4, 0x20000);
        sce_sif_set_reg(0x80000002, 0);
        sce_sif_set_reg(0x80000000, 0);
        return 1;
    }
    return 0;
}

int sce_sif_sync_iop(void) {
    if (sce_sif_get_reg(0x4) & 0x40000) {
        tty_reset_init_flag();
        return 1;
    }
    return 0;
}

extern char iop_reboot_udnl_prefix[];   /* "rom0:UDNL " */
extern char iop_reboot_param_too_long_msg[];
extern void scePrintf();
extern void sceSifInitRpc(int);
extern void sceSifExitRpc(void);

/* sceSifRebootIop: reset the IOP with "rom0:UDNL " + img (at most 80
   characters). Adapted from Lombyte's project (GPL-3.0) for PAL. */
int sce_sif_reboot_iop(char *img) {
    char *prefix = iop_reboot_udnl_prefix;
    char param[80];
    char *p;
    char *d;

    p = img;
    while (*p) {
        p++;
    }
    if ((unsigned int)(p + 11 - img) > 80) {
        scePrintf(iop_reboot_param_too_long_msg, img);
        return 0;
    }
    sceSifInitRpc(0);
    sceSifExitRpc();
    d = param;
    while (*prefix) {
        *d++ = *prefix++;
    }
    while (*img) {
        *d++ = *img++;
    }
    *d = 0;
    return sceSifResetIop(param, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0011D358);

ASM_FUNC("asm/handwritten/core_text", func_0011D360);

/*
 * A word-at-a-time copy of nbytes (rounded down to words); returns 0.
 *
 * Exact under 2.9-ee. Retail leaves the loop branch's delay slot empty,
 * and that is 2.9-ee itself: it pads the short loop with nops in its own
 * output and leaves the bnez unfilled (as in libdma's dma_memclr).
 * Under 2.95.3 reorg fills the slot with the `addiu $4,$4,4` bump, 52
 * bytes against 56, which is what kept this a stub.
 */
int tlbfunc_kcopy(int *dst, int *src, unsigned int nbytes) {
    unsigned int i;
    for (i = 0; i < nbytes >> 2; i++) {
        *dst++ = *src++;
    }
    return 0;
}

ASM_FUNC("asm/handwritten/core_text", func_0011D3A8);

ASM_FUNC("asm/handwritten/core_text", func_0011D3B8);

extern int tlb_functions_syscall_table[];
extern char tlb_functions_kernel_code[];
extern int tlb_saved_syscall_3_entry;

/*
 * Boot-time
 * hardware init: three explicit (addr, value) pokes through
 * set_syscall, a kcopy block copy of tlb_functions_kernel_code (0x330 bytes)
 * to a fixed load address, an interrupt-disable/enable bracket, then a
 * loop over 5 more 8-byte table entries where get_entry_address reads the
 * entry's current value back before set_syscall rewrites it. All
 * three callees are handwritten syscall wrappers (0x74/0x5A/0x5B).
 *
 * The loop counter has to be `unsigned int`, same tell as
 * InitExecPS2 (see that comment): as `int` this compiler reverses
 * the up-count into a down-count-from-5 with `bgezl`, giving retail's
 * `addiu s2,zero,0x1`/`sltiu ...,0x8` only with `unsigned`. What's
 * left is a single instruction's destination register ($v0 vs $v1) on
 * the loop-continuation test -- tried caching `p->a` in a local
 * instead of re-reading it for both calls, which regressed badly
 * (605 words), and reordering the locals, which did nothing; the fix was
 * set_syscall's return type (below).
 */
typedef struct {
    int a;
    int b;
} D_00130138_pair;

extern void set_syscall_v(int, int) __asm__("func_0011D3B8");

/* set_syscall returns nothing: declared int, its result register
   pushed the loop test to $v1. */
int InitTLBFunctions(void) {
    int *t = tlb_functions_syscall_table;
    D_00130138_pair *p;
    unsigned int i;
    int old;
    int r;

    set_syscall_v(t[0], t[1]);
    kcopy((void *)0x80075000, tlb_functions_kernel_code, 0x330);
    flush_cache(0);
    flush_cache(2);
    set_syscall_v(t[2], t[3]);
    set_syscall_v(t[4], t[5]);

    p = (D_00130138_pair *)(t + 6);
    for (i = 3; i < 8; i++) {
        old = get_entry_address(p->a);
        set_syscall_v(p->a, old);
        p++;
    }

    r = get_entry_address(3);
    tlb_saved_syscall_3_entry = r;
    return r;
}

ASM_FUNC("asm/handwritten/core_text", func_0011D490);

/* Set up the TLB for the machine's memory size: the 32 MB layout
   (init_tlb_32mb) when get_memory_size reports 0x2000000, the kernel's own
   routine (kernel_init_tlb) otherwise. Returns its callees' values: retail
   keeps the frame and calls both, and 2.9-ee turns both arms into tail
   jumps (44 bytes against 64) when it is void. */
int InitTLB(void) {
    if (get_memory_size() == 0x2000000) {
        return init_tlb_32mb();
    } else {
        return kernel_init_tlb();
    }
}

ASM_FUNC("asm/handwritten/core_text", func_0011D4E0);

INCLUDE_ASM("asm/nonmatchings/core_text", func_0011D6D4);
