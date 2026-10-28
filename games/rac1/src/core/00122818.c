#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x122818-0x122AD8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph016.o (sceGsSetDefStoreImage) and graph017.o
 * (sceGsExecLoadImage), back to back. Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
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

typedef unsigned long u_long_sd;
typedef int u128_sd __attribute__((mode(TI)));
typedef struct {
    unsigned long NLOOP:15; unsigned long EOP:1; unsigned long pad16:30; unsigned long PRE:1;
    unsigned long PRIM:11; unsigned long FLG:2; unsigned long NREG:4;
    unsigned long REGS0:4; unsigned long REGS1:4; unsigned long REGS2:4; unsigned long REGS3:4;
    unsigned long REGS4:4; unsigned long REGS5:4; unsigned long REGS6:4; unsigned long REGS7:4;
    unsigned long REGS8:4; unsigned long REGS9:4; unsigned long REGS10:4; unsigned long REGS11:4;
    unsigned long REGS12:4; unsigned long REGS13:4; unsigned long REGS14:4; unsigned long REGS15:4;
} SdGifTag __attribute__((aligned(16)));
typedef struct {
    unsigned long SBP:14; unsigned long pad14:2; unsigned long SBW:6; unsigned long pad22:2;
    unsigned long SPSM:6; unsigned long pad30:2; unsigned long DBP:14; unsigned long pad46:2;
    unsigned long DBW:6; unsigned long pad54:2; unsigned long DPSM:6; unsigned long pad62:2;
} SdBitbltbuf;
typedef struct {
    unsigned long SSAX:11; unsigned long pad11:5; unsigned long SSAY:11; unsigned long pad27:5;
    unsigned long DSAX:11; unsigned long pad43:5; unsigned long DSAY:11; unsigned long DIR:2;
    unsigned long pad61:3;
} SdTrxpos;
typedef struct {
    unsigned long RRW:12; unsigned long pad12:20; unsigned long RRH:12; unsigned long pad44:20;
} SdTrxreg;
typedef struct { unsigned long pad00; } SdFinish;
typedef struct { unsigned long XDR:2; unsigned long pad02:62; } SdTrxdir;
typedef struct {
    unsigned int vifcode[4];
    SdGifTag giftag;
    SdBitbltbuf bitbltbuf; long bitbltbufaddr;
    SdTrxpos trxpos; long trxposaddr;
    SdTrxreg trxreg; long trxregaddr;
    SdFinish finish; long finishaddr;
    SdTrxdir trxdir; long trxdiraddr;
} SdStoreImage __attribute__((aligned(16)));

int sceGsSetDefStoreImage(SdStoreImage *sp, short sbp, short sbw, short spsm, short x, short y, short w, short h)
{
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
