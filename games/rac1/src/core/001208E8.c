#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x1208E8-0x121750. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libcdvd, cdvd000.o (the RPC core: sceCdDelayThread,
 * sceCdCallback, the N/S command pre-checks, sceCdSync/SyncS, sceCdInit,
 * sceCdDiskReady, sceCdMmode ...). Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libcdvd.a it matches. The module sees
 * most of its globals as volatile (func_001209D8).
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
extern void sceSifAddCmdHandler(int arg0, int arg1, int arg2);
extern void fs_iob_sema_make(void);
extern int sce_sif_alloc_iop_heap(int arg0);

ASM_FUNC("asm/handwritten/core_text", func_001208E8);


/* sceCdDelayThread: sleep on an alarm. Same stack-descriptor idiom as
   func_0011BBF0 (a SemaParam: initCount [2] = 0, maxCount [1] = 1,
   option [5] = 0). The second argument to func_00118B20 (SetAlarm) is the
   address of func_001208E4's *second* instruction -- retail builds it as
   one %hi/%lo pair on `func_001208E4 + 4`, so it is a code address the
   source names by symbol, not a separate label. `unsigned short arg0` is
   what puts `andi $17,$4,0xFFFF` in the prologue.

   Exact under 2.9-ee with the stores written initCount, maxCount, option
   (as func_0011B710). Under 2.95.3 it was 24/104: that build front-loads
   `sd $31` where retail interleaves the descriptor stores between the
   `$16` and `$31` saves. */
void sceCdDelayThread(unsigned short arg0) {
    int buf[8];
    int h;
    buf[2] = 0;
    buf[1] = 1;
    buf[5] = 0;
    h = create_semaphore(buf);
    set_alarm(arg0, (char *)cd_delay_alarm_handler_base + 4, h);
    wait_semaphore(h);
    delete_semaphore(h);
}

extern int sce_cd_sync_s(int);
extern int disable_interrupts(void);
extern void enable_interrupts(void);
extern void *cd_user_callback;

void *sce_cd_callback(void *arg0) {
    void *old;
    int r;
    if (sce_cd_sync_s(1) != 0) {
        return 0;
    }
    r = disable_interrupts();
    old = cd_user_callback;
    cd_user_callback = arg0;
    if (r != 0) {
        enable_interrupts();
    }
    return old;
}

/*
 * _sceCd_cd_callback: the N-command completion callback. Latch *arg0 into
 * D_00131414 and D_00131418; status 0xB clears D_00131414/D_001313F0 and
 * returns; otherwise signal the D_001313E8 semaphore (func_00118CA0,
 * iSignalSema, int return) and D_001313E0 too if D_001313D4 and the
 * handler D_00159840 are set, else clear D_001313F0; finally clear
 * D_00131414.
 *
 * Every global it touches is read and written as `volatile`, through
 * `__asm__` aliases (the file's plain `extern int` declarations below
 * stay as they are). That gives retail's two reloads of D_00131414, the
 * bare `nop`s after both jal (the semaphore-id loads cannot go into the
 * slots) and the unfilled `b` slot; with fewer volatiles the size
 * changes. Exact under 2.9-ee only: 2.95.3's GCSE copies the %hi of
 * D_00131414 into $16 (`lui $2` + `move $16,$2`) where retail loads
 * `lui $16` directly (SIZE 164/160).
 */
extern volatile int cd_callback_num_v __asm__("D_00131414");
extern volatile int D_00131418_v __asm__("D_00131418");
extern volatile int cd_callback_thread_id_v __asm__("D_001313D4");
extern volatile int cd_callback_sema_v __asm__("D_001313E0");
extern volatile int cd_ncmd_sema_v __asm__("D_001313E8");
extern volatile int cd_ncmd_busy_v __asm__("D_001313F0");
extern void *volatile cd_user_callback_v __asm__("D_00159840");
extern int i_signal_semaphore(int);

void cd_ncmd_callback(int *arg0) {
    cd_callback_num_v = *arg0;
    D_00131418_v = cd_callback_num_v;
    if (cd_callback_num_v == 0xB) {
        cd_callback_num_v = 0;
        cd_ncmd_busy_v = 0;
        return;
    }
    i_signal_semaphore(cd_ncmd_sema_v);
    if (cd_callback_thread_id_v != 0 && cd_user_callback_v != 0) {
        i_signal_semaphore(cd_callback_sema_v);
    } else {
        cd_ncmd_busy_v = 0;
    }
    cd_callback_num_v = 0;
}

LINKER_REMNANT("asm/remnants/core_text", func_00120A78);

/* libcdvd S-command completion callback: through the uncached view of the
   receive block, copies n0 bytes of buf0 to dst0 and n1 bytes of buf1 to
   dst1, then finishes like the N-command callback (func_001209D8 on
   D_00131414). Each destination is loaded once inside its own `if`, which
   gives retail's doubled count test. */
extern int cd_callback_num;

typedef struct {
    int n0;
    int n1;
    char *dst0;
    char *dst1;
    char buf0[0x40];
    char buf1[0x40];
} CdRecv;

void cd_read_end_callback(int arg0) {
    CdRecv *p = (CdRecv *)(arg0 | 0x20000000);
    int i;

    if (p->n0 > 0) {
        char *d0 = p->dst0;
        for (i = 0; i < p->n0; i++) d0[i] = p->buf0[i];
    }
    if (p->n1 > 0) {
        char *d1 = p->dst1;
        for (i = 0; i < p->n1; i++) d1[i] = p->buf1[i];
    }
    cd_ncmd_callback((int *)&cd_callback_num);
}

extern int cd_callback_sema;
extern int cd_ncmd_sema;
extern int cd_scmd_sema;
extern int cd_ncmd_busy;

struct SemaParam_120B28 {
    s32 currentCount;
    s32 maxCount;
    s32 initCount;
    s32 numWaitThreads;
    u32 attr;
    u32 option;
};
extern volatile s32 cd_callback_sema_r[] __asm__("D_001313E0");
extern s32 cd_ncmd_sema_r[] __asm__("D_001313E8");
extern s32 cd_scmd_sema_r[] __asm__("D_001313EC");
extern volatile s32 cd_ncmd_busy_r[] __asm__("D_001313F0");
extern s32 create_semaphore_r(struct SemaParam_120B28 *) __asm__("func_00118C70");

/* cmd_sem_init: unless both command semaphores already exist, create them
   (count 1, max 1) and the completion semaphore (count 0) through
   func_00118C70 (CreateSema), then clear D_001313F0.
   Adapted from Lombyte's project (GPL-3.0): src/sdk/library/cmd_sem_init.c, cmd_sem_init. */
void cd_sema_init(void) {
    struct SemaParam_120B28 sema;

    if ((cd_ncmd_sema_r[0] != -1) && (cd_scmd_sema_r[0] != -1)) {
        return;
    }
    sema.option = 0;
    sema.initCount = 1;
    sema.maxCount = 1;
    /* Both results use the same record; storing the first handle inside
       the second call's argument puts the store in that jal's delay slot,
       as retail has it (the open residual of the old near-miss). */
    cd_scmd_sema_r[0] = create_semaphore_r(
        (cd_ncmd_sema_r[0] = create_semaphore_r(&sema)) ? &sema : &sema);
    sema.initCount = 0;
    cd_callback_sema_r[0] = create_semaphore_r(&sema);
    cd_ncmd_busy_r[0] = 0;
}

extern int cd_callback_thread_id;
/* DeleteSema returns int; declared void, the lui after each call would take
   $v0 instead of retail's $v1. */
extern int delete_semaphore_i(int) __asm__("func_00118C80");
extern void signal_semaphore(int);
extern void sceSifRemoveCmdHandler(int);

/*
 * cdvd_exit (libcdvd.a cdvd000.o): if the callback thread exists
 * (D_001313D4, cb_thid), set the callback number (D_00131414,
 * sceCdCbfunc_num) to -1 and SignalSema its semaphore (D_001313E0,
 * cb_semid); DeleteSema the N-command, S-command and callback
 * semaphores; then remove SIF command handler 0x80000012 with interrupts
 * off, re-enabling them if DIntr said they were on.
 *
 * The do-while (0) around the store leaves loop notes in the block, which
 * the scheduler treats as a barrier: the store stays ahead of the
 * semaphore-id load, as in retail, and the load still fills the jal's
 * delay slot. Without it the load is hoisted above the store.
 */
void cdvd_exit(void) {
    int di;
    if (cd_callback_thread_id != 0) {
        do {
            cd_callback_num_v = -1;
        } while (0);
        signal_semaphore(cd_callback_sema);
    }
    delete_semaphore_i(cd_ncmd_sema);
    delete_semaphore_i(cd_scmd_sema);
    delete_semaphore_i(cd_callback_sema);
    di = disable_interrupts();
    sceSifRemoveCmdHandler(0x80000012);
    if (di != 0) {
        enable_interrupts();
    }
}

LINKER_REMNANT("asm/remnants/core_text", func_00120C58);

/* Interrupt handler registered by func_00120CA0. If a handler is set
   (D_00159844) and the flag D_001313E4 is clear, calls it with D_00159848
   as its argument. The linker remnant in front of it was split off. */
extern int cd_poweroff_callback;
extern int cd_poweroff_callback_arg;
extern int cd_poweroff_installing;

void cd_poweroff_handler(void) {
    if (cd_poweroff_callback != 0 && cd_poweroff_installing == 0) {
        ((void (*)(int))cd_poweroff_callback)(cd_poweroff_callback_arg);
    }
}

extern int cd_poweroff_installing;
extern int cd_poweroff_installed;

int cd_install_poweroff_handler(void) {
    int r;

    cd_poweroff_installing = 1;
    r = disable_interrupts();
    sceSifAddCmdHandler(0x80000012, (int)((char *)cd_poweroff_handler_base + 8), 0);
    if (r != 0) {
        enable_interrupts();
    }
    cd_poweroff_installing = 0;
    cd_poweroff_installed = 1;
    return 1;
}

LINKER_REMNANT("asm/remnants/core_text", func_00120D18);

typedef struct {
    char pad[0x24];
    void *serve;
} SifRpcClientData;

extern void cd_sema_init(void);
extern int poll_semaphore(int);
extern int refer_thread_status(int, void *);
extern void sceSifInitRpc(int);
extern int sceSifBindRpc(void *, unsigned int, int);
extern void scePrintf();
extern void signal_semaphore(int);

extern int cd_ncmd_sema;
extern int cd_ncmd_current;
extern int cd_debug_level;
extern int cd_ncmd_rpc_bound;
extern int cd_thread_id;
extern char cd_thread_status[];
extern char cd_ncmd_sema_fail_msg[];
extern char cd_ncmd_bind_error_msg[];
extern char cd_ncmd_rpc_client[];

/* _sceCd_scmd_prechk (libcdvd.a cdvd000.o) */
int cd_ncmd_prechk(int arg0) {
    int i;
    volatile int *sem = &cd_ncmd_sema;

    cd_sema_init();
    if (*sem != poll_semaphore(*sem)) {
        if (cd_debug_level > 0) {
            scePrintf(cd_ncmd_sema_fail_msg, arg0, cd_ncmd_current);
        }
        return 0;
    }
    cd_ncmd_current = arg0;
    refer_thread_status(cd_thread_id, cd_thread_status);
    if (sce_cd_sync_s(1) != 0) {
        signal_semaphore(*sem);
        return 0;
    }
    sceSifInitRpc(0);
    if (cd_ncmd_rpc_bound >= 0) {
        return 1;
    }
    while (1) {
        if (sceSifBindRpc(cd_ncmd_rpc_client, 0x80000595, 0) < 0) {
            if (cd_debug_level > 0) {
                scePrintf(cd_ncmd_bind_error_msg);
            }
            for (i = 0x100000; i != -1; i--)
                ;
        } else {
            if (((SifRpcClientData *)cd_ncmd_rpc_client)->serve != 0) {
                break;
            }
            for (i = 0x100000; i != -1; i--)
                ;
        }
    }
    cd_ncmd_rpc_bound = 0;
    return 1;
}

extern int sif_call_rpc();
extern int cd_ncmd_prechk(int);
extern void signal_semaphore(int);
extern int cd_ncmd_recv_buffer;

/*
 * sceCdNcmdDiskReady, sibling of func_00121930 (and of the func_0011CBC8
 * pair): open the service, run one func_0011B4C8 RPC, then release the
 * lock and hand back the reply. The reply is read back through the
 * uncached mirror of the DMA buffer (| 0x20000000), which is why the
 * address is spelled as an integer or.
 *
 * The RPC failure path comes first (release, return 0), then the success
 * path, as retail lays them out. Under 2.9-ee that is the form that keeps
 * the early guard's `return 0` and the failure arm's `return 0` apart
 * (retail keeps two separate zeroings); the success-first spelling that
 * 2.95.3 needed is cross-jumped by 2.9-ee into one tail (SIZE 144/152).
 * The two compilers merge the tails in mirror-image layouts, so the
 * layout alone does not tell them apart; the prebuilt libcdvd.a, which
 * this matches, is 2.9-ee's.
 */
int cd_ncmd_disk_ready(void) {
    int r;

    if (cd_ncmd_prechk(2) == 0) {
        return 0;
    }
    if (sif_call_rpc(cd_ncmd_rpc_client, 0xE, 0, 0, 0, &cd_ncmd_recv_buffer, 4, 0, 0) < 0) {
        signal_semaphore(cd_ncmd_sema);
        return 0;
    }
    r = *(int *)((unsigned int)&cd_ncmd_recv_buffer | 0x20000000);
    signal_semaphore(cd_ncmd_sema);
    return r;
}

extern char cd_ncmd_wait_msg[];
extern int cd_debug_level;

/* sceCdSyncS */
int sce_cd_sync_s(int arg0) {
    if (arg0 == 0) {
        if (cd_debug_level > 0) {
            scePrintf(cd_ncmd_wait_msg);
        }
        while (cd_ncmd_busy != 0 || sce_sif_check_stat_rpc(cd_ncmd_rpc_client) != 0) {
            sceCdDelayThread(0x3C);
        }
        return 0;
    }
    if (cd_ncmd_busy != 0 || sce_sif_check_stat_rpc(cd_ncmd_rpc_client) != 0) {
        return 1;
    }
    return 0;
}

extern int sce_sif_check_stat_rpc(void *);
extern char cd_scmd_wait_msg[];
extern char cd_scmd_rpc_client[];

/* sceCdSync */
int cd_scmd_sync(int arg0) {
    if (arg0 == 0) {
        if (cd_debug_level > 0) {
            scePrintf(cd_scmd_wait_msg);
        }
        while (sce_sif_check_stat_rpc(cd_scmd_rpc_client) != 0) {
            sceCdDelayThread(0x3C);
        }
        return 0;
    }
    return sce_sif_check_stat_rpc(cd_scmd_rpc_client);
}

extern int cd_scmd_sema;
extern int cd_scmd_current;
extern char cd_scmd_sema_fail_msg[];
extern char cd_scmd_bind_error_msg[];
extern int cd_scmd_rpc_bound;

/* _sceCd_ncmd_prechk (libcdvd.a cdvd000.o) */
int cd_scmd_prechk(int arg0) {
    int i;
    volatile int *sem = &cd_scmd_sema;

    cd_sema_init();
    if (*sem != poll_semaphore(*sem)) {
        if (cd_debug_level > 0) {
            scePrintf(cd_scmd_sema_fail_msg, arg0, cd_scmd_current);
        }
        return 0;
    }
    cd_scmd_current = arg0;
    refer_thread_status(cd_thread_id, cd_thread_status);
    if (cd_scmd_sync(1) != 0) {
        signal_semaphore(*sem);
        return 0;
    }
    sceSifInitRpc(0);
    if (cd_scmd_rpc_bound >= 0) {
        return 1;
    }
    while (1) {
        if (sceSifBindRpc(cd_scmd_rpc_client, 0x80000593, 0) < 0) {
            if (cd_debug_level > 0) {
                scePrintf(cd_scmd_bind_error_msg);
            }
            for (i = 0x100000; i != -1; i--)
                ;
        } else {
            if (((SifRpcClientData *)cd_scmd_rpc_client)->serve != 0) {
                break;
            }
            for (i = 0x100000; i != -1; i--)
                ;
        }
    }
    cd_scmd_rpc_bound = 0;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001211B0);

struct sceSifRpcData {
    void *paddr; /* 0x0: packet in flight; released by _request_end */
    u32 pid; /* 0x4 */
    s32 tid; /* 0x8: semaphore signalled by _request_end when >= 0 */
    u32 mode; /* 0xC */
};
struct sceSifClientData {
    struct sceSifRpcData rpcd; /* 0x0 */
    u32 command; /* 0x10 */
    void *buff; /* 0x14 */
    void *cbuff; /* 0x18 */
    void (*func)(void *); /* 0x1C: end function, called with para when the call completes */
    void *para; /* 0x20 */
    void *serve; /* 0x24: server bound by sceSifBindRpc; 0 until bound */
};
extern s32 cd_debug_level_r[] __asm__("D_001313D0");
extern volatile s32 cd_scmd_sema_r2[] __asm__("D_001313EC");
extern s32 cd_disk_ready_rpc_bound[];
extern struct sceSifClientData cd_disk_ready_rpc_client;
extern s32 cd_disk_ready_send_buffer[];
extern u8 cd_scmd_recv_buffer_r[] __asm__("D_001325C0");
extern u8 cd_disk_ready_start_msg[];
extern u8 cd_disk_ready_bind_error_msg[];
extern u8 cd_disk_ready_end_msg[];
extern s32 poll_semaphore();
extern void signal_semaphore();
extern void cd_sema_init_r() __asm__("func_00120B28");
extern s32 cd_scmd_sync();
extern s32 sceSifBindRpc();
extern void sceSifInitRpc();
extern void sceSifWriteBackDCache();
extern s32 sif_call_rpc();

/* libcdvd disk-ready query (the sceCdDiskReady pattern): take the S-command semaphore, bind the 0x8000059A server once, send mode and return the server's status word. Mode 8 reports -1 on failure, every other mode 6.
   Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/fun_00121190.c, FUN_00121190. */
s32 sce_cd_disk_ready(s32 mode) {
    s32 response;
    s32 spin;

    if (cd_debug_level_r[0] > 0) {
        scePrintf(cd_disk_ready_start_msg);
    }
    cd_sema_init_r();
    if (cd_scmd_sema_r2[0] != poll_semaphore(cd_scmd_sema_r2[0])) {
        return 6;
    }
    if (cd_scmd_sync(1) != 0) {
        signal_semaphore(cd_scmd_sema_r2[0]);
        if (mode == 8) {
            return -1;
        }
        return 6;
    }
    sceSifInitRpc(0);
    if (cd_disk_ready_rpc_bound[0] < 0) {
        for (;;) {
            if (sceSifBindRpc(&cd_disk_ready_rpc_client, 0x8000059A, 0) < 0) {
                if (cd_debug_level_r[0] > 0) {
                    scePrintf(cd_disk_ready_bind_error_msg);
                }
                for (spin = 0x100000; spin != -1; spin--) {
                }
                continue;
            }
            if (cd_disk_ready_rpc_client.serve != 0) {
                cd_disk_ready_rpc_bound[0] = 0;
                break;
            }
            for (spin = 0x100000; spin != -1; spin--) {
            }
        }
    }
    cd_disk_ready_send_buffer[0] = mode;
    sceSifWriteBackDCache(cd_disk_ready_send_buffer, 4);
    if (sif_call_rpc(&cd_disk_ready_rpc_client, 0, 0, cd_disk_ready_send_buffer, 4, cd_scmd_recv_buffer_r, 4, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema_r2[0]);
        if (mode == 8) {
            return -1;
        }
        return 6;
    }
    if (cd_debug_level_r[0] > 0) {
        scePrintf(cd_disk_ready_end_msg);
    }
    response = *(s32 *)((u32)cd_scmd_recv_buffer_r | 0x20000000);
    signal_semaphore(cd_scmd_sema_r2[0]);
    return response;
}

extern int cd_scmd_send_buffer;
extern int cd_scmd_recv_buffer;
extern int cd_scmd_prechk(int);
extern void sceSifWriteBackDCache(void *, int);

/* sceCdMmode */
int sce_cd_mmode(int arg0) {
    int *p = &cd_scmd_send_buffer;
    int r;

    if (cd_scmd_prechk(0x22) == 0) {
        return 0;
    }
    cd_scmd_send_buffer = arg0;
    sceSifWriteBackDCache(p, 4);
    if (sif_call_rpc(cd_scmd_rpc_client, 0x22, 0, p, 4, &cd_scmd_recv_buffer, 4, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema);
        return 0;
    }
    r = *(int *)((unsigned int)&cd_scmd_recv_buffer | 0x20000000);
    signal_semaphore(cd_scmd_sema);
    return r;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012174C);
