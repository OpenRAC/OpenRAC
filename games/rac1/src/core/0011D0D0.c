#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x11D0D0-0x11D700. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl): the IOP reset (sceSifRebootIop,
 * func_0011D248, "rom0:UDNL "), the boot-time kernel patch/poke sequence
 * (func_0011D3C8) and the TLB setup (func_0011D4A0/func_0011D4E0,
 * "# TLB spad=0 kernel=1:%d ..."). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE).
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
extern void sce_sif_stop_dma(void);
extern u32 sce_sif_get_reg_u(u32 reg) __asm__("func_00118E70");
extern u32 sce_sif_set_reg(u32 reg, u32 val);
extern void sceSifWriteBackDCache(void *p, s32 size);
extern u32 sce_sif_set_dma(SifDmaData *dma, s32 count);

/* sceSifResetIop: like sceSifRebootIop (func_0011D248) but with a caller
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

extern void tty_reset_init_flag(void);

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
 * output and leaves the bnez unfilled (as in libdma's func_001232A8).
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

extern int set_syscall(int, int);
extern void kcopy(void *, void *, int);
extern int get_entry_address(int);
extern void flush_cache(int);
extern int tlb_functions_syscall_table[];
extern char tlb_functions_kernel_code[];
extern int tlb_saved_syscall_3_entry;

/*
 * Exact (it was a 2/0xC4 near-miss until the int/void fix below). Boot-time
 * hardware init: three explicit (addr, value) pokes through
 * func_0011D3B8, a func_0011D360 block copy of D_0012FDB8 (0x330 bytes)
 * to a fixed load address, an interrupt-disable/enable bracket, then a
 * loop over 5 more 8-byte table entries where func_0011D3A8 reads the
 * entry's current value back before func_0011D3B8 rewrites it. All
 * three callees are handwritten syscall wrappers (0x74/0x5A/0x5B).
 *
 * The loop counter has to be `unsigned int`, same tell as
 * func_0011DCB8 (see that comment): as `int` this compiler reverses
 * the up-count into a down-count-from-5 with `bgezl`, giving retail's
 * `addiu s2,zero,0x1`/`sltiu ...,0x8` only with `unsigned`. What's
 * left is a single instruction's destination register ($v0 vs $v1) on
 * the loop-continuation test -- tried caching `p->a` in a local
 * instead of re-reading it for both calls, which regressed badly
 * (605 words), and reordering the locals, which did nothing; the fix was
 * func_0011D3B8's return type (below).
 */
typedef struct { int a; int b; } D_00130138_pair;

extern void set_syscall_v(int, int) __asm__("func_0011D3B8");

/* func_0011D3B8 returns nothing: declared int, its result register
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

extern int init_tlb_32mb(void);
extern int kernel_init_tlb(void);

/* Set up the TLB for the machine's memory size: the 32 MB layout
   (func_0011D4E0) when func_00118EA0 reports 0x2000000, the kernel's own
   routine (func_00118EB0) otherwise. Returns its callees' values: retail
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
