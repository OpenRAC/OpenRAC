#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x121750-0x121D18. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Six Sony SDK archive members back to back (each ends 8-byte aligned, so
 * no fill separates them): libcdvd's cdvd005.o (sceCdRead), cdvd015.o
 * (sceCdGetError, func_00121930), cdvd018.o (sceCdBreak, func_001219C8)
 * and cdvd039.o (sceCdReadClock), then libgraph's graph001.o
 * (sceGsResetGraph, sceGsGetGParam = func_00121D08). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt archives they match.
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
extern int cd_ncmd_disk_ready(void);

/* sceCdRead: queue one CD/DVD read request (lba, sectors, buffer, mode)
 * in the request block at D_001314C0 and start it through the RPC
 * func_0011B4C8. Returns 1 when the RPC was accepted, 0 otherwise.
 * The volatile state clears precede the semaphore read; the verified
 * GNU 2.10-ee profile fills SignalSema's call slot with that word load.
 * Sector size by mode->datapattern: 1 -> 0x918, 2 -> 0x924, else 0x800. */
extern int sif_call_rpc(void *, int, int, void *, int, void *, int, void *, void *);
extern int cd_read_mode_flags NOT_SDA;
extern char cd_read_request[];
extern char cd_read_reply_buffer[];
extern int cd_read_status NOT_SDA;
extern char cd_read_call_msg[];
extern char cd_read_end_msg[];
extern volatile int cd_callback_num;
extern void sceSifWriteBackDCache(void *, int);
extern void cd_read_end_callback();

int sce_cd_read(int lba, int sectors, void *buf, unsigned char *mode) {
    char *req = cd_read_request;
    int size;
    int r;

    if ((cd_read_mode_flags & 1) == 0) {
        if (cd_ncmd_disk_ready() == 6) {
            return 0;
        }
    }
    if (cd_ncmd_prechk(4) == 0) {
        return 0;
    }

    *(int *)(req + 0x0) = lba;
    *(int *)(req + 0x4) = sectors;
    *(void **)(req + 0x8) = buf;
    req[0xC] = mode[0];
    req[0xD] = mode[1];
    req[0xE] = mode[2];
    *(void **)(req + 0x10) = cd_read_reply_buffer;
    *(void **)(req + 0x14) = &cd_read_status;

    switch (mode[2]) {
    case 1:
        size = sectors * 0x918;
        break;
    case 2:
        size = sectors * 0x924;
        break;
    case 0:
    default:
        size = sectors << 11;
        break;
    }

    cd_read_status = 0;
    if ((*(volatile int *)&cd_read_mode_flags & 2) == 0) {
        sceSifWriteBackDCache(buf, size);
    }
    sceSifWriteBackDCache(cd_read_reply_buffer, 0x90);
    sceSifWriteBackDCache(req, 0x18);
    sceSifWriteBackDCache(&cd_read_status, 4);
    if (cd_debug_level > 0) {
        scePrintf(cd_read_call_msg);
    }

    cd_callback_num = 1;
    *(volatile int *)&cd_ncmd_busy = 1;
    r = sif_call_rpc(cd_ncmd_rpc_client, 1, 1, req, 0x18, 0, 0,
                      (void *)cd_read_end_callback, cd_read_reply_buffer);
    if (r < 0) {
        cd_callback_num = 0;
        *(volatile int *)&cd_ncmd_busy = 0;
        signal_semaphore(*(volatile int *)&cd_ncmd_sema);
        return 0;
    }
    if (cd_debug_level > 0) {
        scePrintf(cd_read_end_msg);
    }
    return 1;
}

extern int cd_scmd_prechk(int);
extern int cd_scmd_recv_buffer;

/* sceCdGetError, func_00120E98's sibling: S-command 3, RPC 4, and -1
   rather than 0 as the failure result.

   The RPC failure is handled first (SignalSema, return -1), then the
   success path. That is the layout retail has, and it keeps 2.9-ee from
   cross-jumping the two `return -1` tails into one (SIZE 144/152 with
   the success path first, which 2.95.3 needed). */
int sce_cd_get_error(void) {
    int r;

    if (cd_scmd_prechk(3) == 0) {
        return -1;
    }
    if (sif_call_rpc(cd_scmd_rpc_client, 4, 0, 0, 0, &cd_scmd_recv_buffer, 4, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema);
        return -1;
    }
    r = *(int *)((unsigned int)&cd_scmd_recv_buffer | 0x20000000);
    signal_semaphore(cd_scmd_sema);
    return r;
}

extern volatile int cd_callback_num;

/* sceCdBreak, sibling of func_00121930: S-command 0x1E, RPC 0x16 on
   D_00132E08 under the D_001313EC semaphore, with the D_00131414 busy flag
   set around it; returns the reply word read uncached. The volatile flag
   and semaphore reads stay out of the delay slots, where reorg never puts
   a volatile access (this module sees its globals as volatile, as
   cdvd000.o does).

   As in func_00121930, the RPC failure path comes first: with the success
   path first 2.9-ee merges the two `return 0` tails (SIZE 176/184). */
int sce_cd_break(void) {
    int r;

    if (cd_scmd_prechk(0x1E) == 0) {
        return 0;
    }
    cd_callback_num = 8;
    if (sif_call_rpc(cd_scmd_rpc_client, 0x16, 0, 0, 0, &cd_scmd_recv_buffer, 4, 0, 0) < 0) {
        signal_semaphore(*(volatile int *)&cd_scmd_sema);
        cd_callback_num = 0;
        return 0;
    }
    cd_callback_num = 0;
    r = *(int *)((unsigned int)&cd_scmd_recv_buffer | 0x20000000);
    signal_semaphore(*(volatile int *)&cd_scmd_sema);
    return r;
}

extern char cd_read_clock_msg_1[];
extern char cd_read_clock_msg_2[];
typedef struct {
    char b[8];
} CdClock;

/* sceCdReadClock: S-command 0xF, RPC 1 with a 0x10-byte reply; the
   8-byte clock after the status word is copied out uncached. Failure
   path first, as in func_00121930. */
int sce_cd_read_clock(CdClock *out) {
    int r;

    if (cd_scmd_prechk(0xF) == 0) {
        return 0;
    }
    if (cd_debug_level > 0) {
        scePrintf(cd_read_clock_msg_1);
    }
    if (sif_call_rpc(cd_scmd_rpc_client, 1, 0, 0, 0, &cd_scmd_recv_buffer, 0x10, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema);
        return 0;
    }
    *out = *(CdClock *)(((unsigned int)&cd_scmd_recv_buffer + 4) | 0x20000000);
    if (cd_debug_level > 0) {
        scePrintf(cd_read_clock_msg_2);
    }
    r = *(int *)((unsigned int)&cd_scmd_recv_buffer | 0x20000000);
    signal_semaphore(cd_scmd_sema);
    return r;
}

typedef struct {
    short sceGsInterMode;
    short sceGsOutMode;
    short sceGsFFMode;
    short sceGsVersion;
    volatile int (*sceGsVSCfunc)(int);
    int sceGsVSCid;
} sceGsGParam;

extern void *sceGsGetGParam(void);          /* sceGsGetGParam */
extern int gs_put_imr();                /* GsPutIMR */
extern int remove_intc_handler();                /* RemoveIntcHandler */
extern int disable_intc_atomic();
extern void set_gs_crt(int, int, int);  /* SetGsCrt */

/* sceGsResetGraph. Adapted from Lombyte's project (GPL-3.0) for PAL. */
void sceGsResetGraph(short mode, short inter, short out, short ff) {
    sceGsGParam *gp;

    switch (mode) {
    case 0:
        gp = (sceGsGParam *)sceGsGetGParam();
        *(volatile unsigned long *)0x12001000 = 0x200;
        gp->sceGsInterMode = inter;
        gp->sceGsOutMode = out;
        gp->sceGsVersion = (short)((*(volatile unsigned long *)0x12001000 >> 16) & 0xFF);
        gs_put_imr(0xFF00);
        gp->sceGsFFMode = (ff != 0);
        if (gp->sceGsVSCfunc != 0) {
            disable_intc_atomic(2);
            remove_intc_handler(2, gp->sceGsVSCid);
            gp->sceGsVSCfunc = 0;
            gp->sceGsVSCid = 0;
        }
        set_gs_crt(inter & 1, out & 0xFF, ff & 1);
        break;
    case 1:
        *(volatile unsigned long *)0x12001000 = 0x100;
        break;
    case 5:
        gp = (sceGsGParam *)sceGsGetGParam();
        gp->sceGsFFMode = (ff != 0);
        gp->sceGsInterMode = inter;
        gp->sceGsOutMode = out;
        gp->sceGsVersion = (short)((*(volatile unsigned long *)0x12001000 >> 16) & 0xFF);
        set_gs_crt(inter & 1, out & 0xFF, ff & 1);
        break;
    }
}

extern char CoreDataTable[];

void *sceGsGetGParam(void) {
    return CoreDataTable;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00121D14);
