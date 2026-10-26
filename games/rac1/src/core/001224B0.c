#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x1224B0-0x122630. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph008.o (sceGsPutDrawEnv) and graph011.o
 * (sceGsSyncV), back to back. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libgraph.a.
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
extern GsGParam *sceGsGetGParam(void);
extern void wait_vblank_start(void);
extern long wait_vblank_csr(void);

/* Returns the current field, bit 13 of the GS CSR (read directly at
   0x12001000, or through func_00118F60), when the display is
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
