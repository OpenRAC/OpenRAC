#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x122630-0x122818. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph015.o (sceGsSetDefLoadImage). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
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

extern void scePrintf(void *);
extern char gs_load_image_too_big_msg[];

typedef int u128_li __attribute__((mode(TI)));
typedef struct {
    unsigned long NLOOP:15; unsigned long EOP:1; unsigned long pad16:30; unsigned long PRE:1;
    unsigned long PRIM:11; unsigned long FLG:2; unsigned long NREG:4;
    unsigned long REGS0:4; unsigned long REGS1:4; unsigned long REGS2:4; unsigned long REGS3:4;
    unsigned long REGS4:4; unsigned long REGS5:4; unsigned long REGS6:4; unsigned long REGS7:4;
    unsigned long REGS8:4; unsigned long REGS9:4; unsigned long REGS10:4; unsigned long REGS11:4;
    unsigned long REGS12:4; unsigned long REGS13:4; unsigned long REGS14:4; unsigned long REGS15:4;
} LiGifTag __attribute__((aligned(16)));
typedef struct {
    LiGifTag giftag0;
    unsigned long bitbltbuf; long bitbltbufaddr;
    unsigned long trxpos; long trxposaddr;
    unsigned long trxreg; long trxregaddr;
    unsigned long trxdir; long trxdiraddr;
    LiGifTag giftag1;
} LiLoadImage __attribute__((aligned(16)));

int sceGsSetDefLoadImage(LiLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y, short w, short h)
{
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
