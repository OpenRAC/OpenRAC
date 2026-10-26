#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x1236F0-0x125218. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
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
extern void sceDmaPause(void *);
extern char dma_sync_timeout_msg[];
extern int DmaChannels[];

typedef struct {
    char pad[0x24];
    void *serve;
} SifRpcClientData;

extern int mc_semaphore;
extern int sce_mc_sync(int, int *, int *);
extern int wait_semaphore(int);
extern int sceSifInitRpc(int);
extern char mc_rpc_client[];
extern int mc_rpc_request;
extern char mc_rpc_reply[];
extern int sceSifBindRpc(void *, int, int);
extern char str_mc_bind_error[];
extern int sif_call_rpc();
extern char str_mc_old_mcserv[];
extern char str_mc_old_mcman[];

/* sceMcInit (libmc): one-shot init. Creates the module semaphore
   D_00132EAC if not already valid, resets any pending command
   (func_00123F30, sceMcSync), takes the semaphore, brings up the SIF RPC
   layer (sceSifInitRpc) and loops binding the IOP memory-card RPC server
   (0x80000400) until D_00159B00's SifRpcClientData.serve is set,
   busy-waiting between binds; a bind failure prints and hangs forever.
   Once bound, issues RPC 0xFE (a handshake/version query) and validates
   the two version words in the reply, printing and clearing serve on any
   failure. Each ->serve access is a fresh cast of D_00159B00, matching
   the local SifRpcClientData idiom already used in
   src/core/001208E8.c, rather than a cached pointer local. */
int sce_mc_init(void) {
    int buf[8];
    int *rbuf;
    int i;
    int r;

    if (mc_semaphore < 0) {
        buf[5] = 0;
        buf[2] = 1;
        buf[1] = 1;
        mc_semaphore = create_semaphore(buf);
    }
    sce_mc_sync(0, 0, 0);
    wait_semaphore(mc_semaphore);
    sceSifInitRpc(0);
    for (;;) {
        if (sceSifBindRpc(mc_rpc_client, 0x80000400, 0) < 0) {
            scePrintf(str_mc_bind_error);
            for (;;) ;
        }
        if (((SifRpcClientData *)mc_rpc_client)->serve != 0) break;
        for (i = 0x100000; i != 0; i--) ;
    }
    r = sif_call_rpc(mc_rpc_client, 0xFE, 0, &mc_rpc_request, 0x30, mc_rpc_reply, 0xC, 0, 0);
    signal_semaphore(mc_semaphore);
    if (r < 0) {
        ((SifRpcClientData *)mc_rpc_client)->serve = 0;
        return r - 100;
    }
    rbuf = (int *)mc_rpc_reply;
    if (rbuf[1] < 0x20A) {
        scePrintf(str_mc_old_mcserv);
        ((SifRpcClientData *)mc_rpc_client)->serve = 0;
        return -0x78;
    }
    if (rbuf[2] < 0x20E) {
        scePrintf(str_mc_old_mcman);
        ((SifRpcClientData *)mc_rpc_client)->serve = 0;
        return -0x79;
    }
    return *(int *)mc_rpc_reply;
}

LINKER_REMNANT("asm/remnants/core_text", func_001238A8);

extern char mc_rpc_client[];
extern int poll_semaphore(int);
extern char mc_rpc_path_request[];
extern void *libc_strncpy(void *, const void *, int);
extern int mc_current_command;

/* sceMcOpen(port, slot, fname, mode) (libmc): same RPC-wrapper shape as
   sceMcDelete (func_00124410) but with the flags/mode argument stored to
   the descriptor's +0x8 word instead of a hardcoded 0, and RPC command 2. */
int sce_mc_open(int port, int slot, int fname_, int mode) {
    char *cd = mc_rpc_client;
    char *fname = (char *)fname_;
    char *fp;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    if (fname == 0 || *fname == 0) {
        signal_semaphore(mc_semaphore);
        return -210;
    }
    libc_strncpy(mc_rpc_path_request + 0x14, fname, 0x3FF);
    fp = mc_rpc_path_request;
    *(int *)(fp + 0x0) = port;
    fp[0x413] = 0;
    *(int *)(fp + 0x8) = mode;
    *(int *)(fp + 0x4) = slot;
    r = sif_call_rpc(cd, 2, 1, fp, 0x414, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 2;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern int sce_mc_open(int, int, int, int);

int sce_mc_mkdir(int arg0, int arg1, int arg2) {
    int r = sce_mc_open(arg0, arg1, arg2, 0x40);
    if (r == 0) {
        mc_current_command = 0xB;
    }
    return r;
}

extern char mc_rpc_client[];
extern int mc_rpc_request;

/* RPC 3 on the client D_00159B00, in func_001239D8's shape: -100 with
   no server bound, -200 when the semaphore wait fails. */
int sce_mc_close(int arg0) {
    char *cd = mc_rpc_client;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    mc_rpc_request = arg0;
    r = sif_call_rpc(cd, 3, 1, &mc_rpc_request, 0x30, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 3;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern char mc_rpc_client[];
extern int mc_rpc_request;

/* RPC 4, the same shape with three request words. `buf` is assigned
   after the semaphore call, so retail builds its address in a temp. */
int sce_mc_seek(int arg0, int arg1, int arg2) {
    char *cd = mc_rpc_client;
    int *buf;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    buf = &mc_rpc_request;
    buf[0] = arg0;
    buf[4] = arg1;
    buf[5] = arg2;
    r = sif_call_rpc(cd, 4, 1, buf, 0x30, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 4;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

/*
 * Unpack a scratchpad-resident descriptor: arg0 is forced into the SPR
 * window (| 0x20000000), then two counted byte runs are copied out of
 * it -- the one at +0x10 to the pointer at +0x8 for the count at +0x0,
 * and the one at +0x50 to the pointer at +0xC for the count at +0x4.
 * The counts are re-read from the descriptor on every iteration, so
 * they are written out in full rather than cached.
 *
 * The explicit zero-trip guard around each do/while is what makes this
 * match, and it is worth knowing why. Written as a plain `for`, the
 * source base (p + 0x10) is loop-invariant, so GCC hoists it ABOVE the
 * guard; retail computes it BELOW, between the guard and the loop head.
 * That one position shifts the loop head by a word, which in turn costs
 * the alignment nop retail carries -- two instructions per loop, 16
 * bytes over the two loops. Spelling the guard yourself puts the
 * invariant where retail has it. (Folding the +0x10 into the load
 * offset instead of keeping a base pointer costs another 8.)
 */
void mc_read_fix_align(void *arg0) {
    char *p = (char *)((unsigned int)arg0 | 0x20000000);
    unsigned char *src;
    char *d;
    int i;

    if (*(int *)(p + 0x0) != 0) {
        d = *(char **)(p + 0x8);
        i = 0;
        if (i < *(int *)(p + 0x0)) {
            src = (unsigned char *)(p + 0x10);
            do {
                *d++ = src[i];
                i++;
            } while (i < *(int *)(p + 0x0));
        }
    }
    if (*(int *)(p + 0x4) != 0) {
        d = *(char **)(p + 0xC);
        i = 0;
        if (i < *(int *)(p + 0x4)) {
            src = (unsigned char *)(p + 0x50);
            do {
                *d++ = src[i];
                i++;
            } while (i < *(int *)(p + 0x4));
        }
    }
}

extern char mc_rpc_client[];
extern int mc_rpc_request;
extern char mc_rpc_end_data[];
extern int sceSifWriteBackDCache(void *, int);
extern void mc_read_fix_align(void *);

int sce_mc_read(int fd, void *buf, int nbyte) {
    char *cd = mc_rpc_client;
    int *p;
    char *next;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    p = &mc_rpc_request;
    next = mc_rpc_end_data;
    p[0] = fd;
    p[7] = (int)next;
    sceSifWriteBackDCache((void *)(p[6] = (int)buf), p[3] = nbyte);
    sceSifWriteBackDCache(next, 0xC0);
    r = sif_call_rpc(cd, 5, 1, p, 0x30, mc_rpc_reply, 4, mc_read_fix_align, next);
    if (r == 0) {
        mc_current_command = 5;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

struct McClient {
    unsigned char reserved_00[0x24];
    int is_initialized;
};
struct McWriteReq {
    int file_descriptor;
    int port;
    int slot;
    int size;
    int offset;
    unsigned int origin;
    int buffer;
    int reserved_1c;
    unsigned char data[0x10];
};
extern struct McClient mc_rpc_client_c __asm__("D_00159B00");
extern struct McWriteReq mc_rpc_request_r __asm__("D_00159B80");

/* sceMcWrite: copies the unaligned lead of the buffer into the request, sends RPC 6. Adapted from Lombyte's project (GPL-3.0) for PAL: src/sdk/storage/memory_card/sce_mc_write.c, sceMcWrite. */
int sce_mc_write_buffer(int fd, unsigned char *buffer, int size) {
    int rpc_result;
    unsigned int i;
    int root_off;
    int base;

    if (mc_rpc_client_c.is_initialized == 0) {
        return -0x64;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -0xC8;
    }
    mc_rpc_request_r.file_descriptor = fd;
    if (size < 0x11) {
        mc_rpc_request_r.origin = size;
        mc_rpc_request_r.size = 0;
        mc_rpc_request_r.buffer = 0;
    } else {
        base = (int)buffer - 0x10;
        root_off = ((int)(buffer - 1) & 0xFFFFFFF0) - base;
        mc_rpc_request_r.size = size - root_off;
        mc_rpc_request_r.origin = root_off;
        mc_rpc_request_r.buffer = (int)(buffer + root_off);
    }
    for (i = 0; i < mc_rpc_request_r.origin; i++) {
        mc_rpc_request_r.data[i] = buffer[i];
    }
    flush_cache(0);
    rpc_result = sif_call_rpc(&mc_rpc_client_c, 6, 1, &mc_rpc_request_r, 0x30, mc_rpc_reply, 4, 0, 0);
    if (rpc_result == 0) {
        mc_current_command = 6;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return rpc_result;
}

ASM_FUNC("asm/handwritten/core_text", func_00123EC0);

extern int sleep_thread(void);
extern void mc_hear_alarm(int, unsigned short, void *);

/* mcDelayThread (libmc): sleep this thread until an alarm of `time`
   h-lines wakes it: SetAlarm (func_00118B20) with mcHearAlarm
   (func_00123EC0) on GetThreadId(), then SleepThread(). */
void mc_delay_thread(int time) {
    set_alarm((unsigned short)time, mc_hear_alarm, get_thread_id());
    sleep_thread();
}

extern char mc_rpc_client[];
extern void mc_delay_thread(int);

/* sceMcSync(mode, &cmd, &result): polls the pending memory card command
   (or waits for it when mode is 0) and hands back its result. Exact
   only under 2.9-ee: 2.95.3's gcse leaves two extra %hi copies. */
int sce_mc_sync(int mode, int *cmd, int *result) {
    int r;

    if (mc_current_command == 0) {
        return -1;
    }
    r = sce_sif_check_stat_rpc(mc_rpc_client);
    if (mode == 0 && r != 0) {
        while (sce_sif_check_stat_rpc(mc_rpc_client) != 0) {
            mc_delay_thread(0x3C);
        }
        r = 0;
    }
    r = (r == 0);
    if (cmd != 0) {
        *cmd = mc_current_command;
    }
    if (r != 0) {
        mc_current_command = 0;
        if (result != 0) {
            *result = *(int *)mc_rpc_reply;
        }
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern int *mc_get_info_type_out;
extern int *mc_get_info_free_out;
extern int *mc_get_info_format_out;

void mc_get_info_end(int arg0) {
    char *p = (char *)(arg0 | 0x20000000);
    if (mc_get_info_type_out != 0) *mc_get_info_type_out = *(int *)(p + 0x0);
    if (mc_get_info_free_out != 0) *mc_get_info_free_out = *(int *)(p + 0x4);
    if (mc_get_info_format_out != 0) *mc_get_info_format_out = *(int *)(p + 0x90);
}

extern int mc_rpc_request_a[] __asm__("D_00159B80");

/* sceMcGetInfo-style RPC 1 (libmc): asks for the card type, free space
   and format state (each only when its result pointer is given); the
   reply lands in D_0015A000 and func_00124010 copies it out. */
int sce_mc_get_info(int port, int slot, int *type, int *free, int *format) {
    int r;

    {
        char *cd = mc_rpc_client;
        if (*(int *)(cd + 0x24) == 0) {
            return -100;
        }
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    mc_rpc_request_a[1] = port;
    mc_rpc_request_a[2] = slot;
    mc_rpc_request_a[7] = (int)mc_rpc_end_data;
    if (type != 0) {
        mc_rpc_request_a[5] = 1;
    } else {
        mc_rpc_request_a[5] = 0;
    }
    if (free != 0) {
        mc_rpc_request_a[4] = 1;
    } else {
        mc_rpc_request_a[4] = 0;
    }
    if (format != 0) {
        mc_rpc_request_a[3] = 1;
    } else {
        mc_rpc_request_a[3] = 0;
    }
    mc_get_info_type_out = type;
    mc_get_info_free_out = free;
    mc_get_info_format_out = format;
    sceSifWriteBackDCache(mc_rpc_end_data, 0xC0);
    r = sif_call_rpc(mc_rpc_client, 1, 1, mc_rpc_request_a, 0x30, mc_rpc_reply, 4, mc_get_info_end, mc_rpc_end_data);
    if (r == 0) {
        mc_current_command = 1;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_001241E8);

/* sceMcGetDir-style RPC 0xD (libmc): name (non-empty), mode, the table
   size and the table, whose cache is written back first (func_0011AD70)
   when count >= 0. Shape as sceMcOpen (func_001238B0). */
int sce_mc_get_dir(int port, int slot, char *name, int mode, int count, void *table) {
    char *cd = mc_rpc_client;
    char *fp;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    if (name == 0 || *name == 0) {
        signal_semaphore(mc_semaphore);
        return -210;
    }
    fp = mc_rpc_path_request;
    *(int *)(fp + 0x0) = port;
    *(int *)(fp + 0x4) = slot;
    *(int *)(fp + 0x8) = mode;
    *(int *)(fp + 0xC) = count;
    *(void **)(fp + 0x10) = table;
    libc_strncpy(fp + 0x14, name, 0x3FF);
    fp[0x413] = 0;
    if (count >= 0) {
        sceSifWriteBackDCache(table, count << 6);
    }
    r = sif_call_rpc(cd, 0xD, 1, fp, 0x414, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 0xD;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_00124338);

/* RPC 0x10 with two request words, as func_00124528 below. */
int sce_mc_format(int arg0, int arg1) {
    char *cd = mc_rpc_client;
    int *buf;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    buf = &mc_rpc_request;
    buf[1] = arg0;
    buf[2] = arg1;
    r = sif_call_rpc(cd, 0x10, 1, buf, 0x30, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 0x10;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern char mc_rpc_client[];

/* sceMcDelete(port, slot, fname): rejects a null/empty fname (SignalSema,
   return -210), else strncpy's it into the shared sifParamFname
   descriptor's name field (D_00159BB0+0x14), fills in port/slot/flags
   and force-terminates at the max length, then runs RPC 0xF (delete)
   through cd_base. Nonzero (queued) result SignalSemas and returns the
   handle; 0 records the command and returns 0 -- the same idiom as
   func_00123C30/sceMcRead. */
int sce_mc_delete(int port, int slot, char *fname) {
    char *cd = mc_rpc_client;
    char *fp;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    if (fname == 0 || *fname == 0) {
        signal_semaphore(mc_semaphore);
        return -210;
    }
    libc_strncpy(mc_rpc_path_request + 0x14, fname, 0x3FF);
    fp = mc_rpc_path_request;
    *(int *)(fp + 0x0) = port;
    *(int *)(fp + 0x4) = slot;
    fp[0x413] = 0;
    *(int *)(fp + 0x8) = 0;
    r = sif_call_rpc(cd, 0xF, 1, fp, 0x414, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 0xF;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern char mc_rpc_client[];
extern int mc_rpc_request;

/* RPC 0x11, the same shape with two request words. */
int sce_mc_unformat(int arg0, int arg1) {
    char *cd = mc_rpc_client;
    int *buf;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_semaphore) < 0) {
        return -200;
    }
    buf = &mc_rpc_request;
    buf[1] = arg0;
    buf[2] = arg1;
    r = sif_call_rpc(cd, 0x11, 1, buf, 0x30, mc_rpc_reply, 4, 0, 0);
    if (r == 0) {
        mc_current_command = 0x11;
    } else {
        signal_semaphore(mc_semaphore);
    }
    return r;
}

extern char RpcQueueData[];
extern int RpcReceiveData;

int sce_dbc_get_mod_version(void) {
    sif_call_rpc(RpcQueueData, 0x80000963, 0, &RpcReceiveData, 0x400,
                  &RpcReceiveData, 0x400, 0, 0);
    return RpcReceiveData;
}

extern char dbc_second_rpc_client[];
extern char str_dbc_bind_failed[];
extern char str_dbc_version_mismatch[];
extern char str_dbc_version_pair_fmt[];
extern char str_dbc_sif_dma_addr_fmt[];
extern int dbc_port_link_states[];
extern char dbc_work_area[];
extern void libc_exit(int) __attribute__((noreturn)); /* exit */
extern void debug_print_stub(void *, ...);
extern int sceDbcSetWorkAddr(int);

/* Bind the two IOP servers 0x80000900 and 0x8000091B (retrying with a
   busy wait; exit(-1) if binding fails), check the server version
   (func_001245F8: major 2), clear the 16-word table D_0015B600 and read
   the configuration into D_0015B580. Returns 1, or 0 on a version
   mismatch. */
int sce_dbc_init(void) {
    int i;
    int v;

    sceSifInitRpc(0);
    for (;;) {
        if (sceSifBindRpc(RpcQueueData, 0x80000900, 0) < 0) {
            debug_print_stub(str_dbc_bind_failed);
            libc_exit(-1);
        }
        if (*(int *)(RpcQueueData + 0x24) != 0) {
            break;
        }
        i = 0x10000;
        while (i--) {
        }
    }
    for (;;) {
        if (sceSifBindRpc(dbc_second_rpc_client, 0x8000091B, 0) < 0) {
            debug_print_stub(str_dbc_bind_failed);
            libc_exit(-1);
        }
        if (*(int *)(dbc_second_rpc_client + 0x24) != 0) {
            break;
        }
        i = 0x10000;
        while (i--) {
        }
    }
    v = sce_dbc_get_mod_version();
    if (v >> 8 != 2) {
        scePrintf(str_dbc_version_mismatch);
        scePrintf(str_dbc_version_pair_fmt, 2, 0, v >> 8, v & 0xFF);
        return 0;
    }
    for (i = 15; i >= 0; i--) {
        dbc_port_link_states[i] = 0;
    }
    debug_print_stub(str_dbc_sif_dma_addr_fmt, dbc_work_area);
    sceDbcSetWorkAddr((int)dbc_work_area);
    return 1;
}

extern char str_dbc_set_work_addr_error[];

/* Same RPC shape as func_001245F8 above (9 args: $4-$11 plus one stack
   slot), but with a different command id and an error path.

   The request word goes into the buffer at +4. It has to be written
   through a local pointer, not as (&D_0015B180)[1]: the latter folds to
   the symbol D_0015B180+4, so the compiler builds THAT address and then
   subtracts 4 to pass the buffer. With `buf`, one lui/addiu serves both
   the store and the two pointer arguments, as retail does.

   The test is written >= 0 so the error path is the fall-through and the
   success path is the branch target, letting the result load sit in the
   bgez delay slot. */
int sceDbcSetWorkAddr(int arg0) {
    int *buf = &RpcReceiveData;

    buf[1] = arg0;
    if (sif_call_rpc(RpcQueueData, 0x80000904, 0, buf, 0x400,
                      buf, 0x400, 0, 0) >= 0) {
        return RpcReceiveData;
    } else {
        debug_print_stub(str_dbc_set_work_addr_error);
        return 0;
    }
}

extern char str_dbc_create_socket_error[];

/* sceDbcCreateSocket (libdbc): copy the caller's socket record (five
   header words, then a 16-byte name) into the RPC buffer D_0015B180, set
   its port (arg1, word 10) and mode (arg2, word 11), then run the
   create-socket RPC (0x80000901) through cd_base. Prints and returns 0
   on failure, else the reply's word 9.

   The header words are copied as pointers. Their alias set then differs
   from the int port/mode stores, so the header loads need not wait for
   those stores; the scheduler then places `i = 0` ahead of them, and the
   loop registers come out in $t0-$t2 as in retail. namesrc/namedst are
   precomputed so each byte address is one base+index add, and the call
   re-takes &D_0015B180 for its buffer arguments. */
int sceDbcCreateSocket(void *arg0, int arg1, int arg2) {
    int *buf = &RpcReceiveData;
    char *namesrc = (char *)arg0 + 0x14;
    char *namedst = (char *)buf + 0x14;
    int i;

    buf[10] = arg1;
    buf[11] = arg2;
    ((void **)buf)[0] = ((void **)arg0)[0];
    ((void **)buf)[1] = ((void **)arg0)[1];
    ((void **)buf)[2] = ((void **)arg0)[2];
    ((void **)buf)[3] = ((void **)arg0)[3];
    ((void **)buf)[4] = ((void **)arg0)[4];
    for (i = 0; i < 0x10; i++) {
        namedst[i] = namesrc[i];
    }
    if (sif_call_rpc(RpcQueueData, 0x80000901, 0, &RpcReceiveData, 0x400,
                       &RpcReceiveData, 0x400, 0, 0) < 0) {
        debug_print_stub(str_dbc_create_socket_error);
        return 0;
    }
    return buf[9];
}

extern void sync_dcache_range(void *, void *);
/* EIntr returns the previous interrupt state; the file declares it void.
   A call that returns a value changes how the lookup after it is
   allocated. */
extern int enable_interrupts_i(void) __asm__("func_0011D9A8");
extern void *memcpy(void *, const void *, unsigned int);
extern char str_dbc_get_dep_number_error[];

/*
 * sceDbcGetDepNumber(port) (libdbc): write back the 0x80-byte DMA buffer
 * D_0015B580 (func_00119288), then with interrupts off (DIntr/EIntr)
 * copy its first 0x40 bytes into the link-state table D_0015B600. A port
 * whose state is not 1 (linked) returns -12. Otherwise RPC 0x80000903
 * with the port as the request word; returns the reply's second word, or
 * reports the failure (func_00124B60) and returns 0.
 *
 * The RPC buffer is re-taken as &D_0015B180 for the call's arguments and
 * `buf` is only assigned where it is used, so the table base and the
 * buffer share retail's saved register.
 */
int sceDbcGetDepNumber(int port) {
    int *buf;

    sync_dcache_range(dbc_work_area, dbc_work_area + 0x80);
    disable_interrupts();
    memcpy(dbc_port_link_states, dbc_work_area, 0x40);
    enable_interrupts_i();
    if (dbc_port_link_states[port] != 1) {
        return -0xC;
    }
    buf = &RpcReceiveData;
    buf[0] = port;
    if (sif_call_rpc(RpcQueueData, 0x80000903, 0, &RpcReceiveData, 0x400,
                       &RpcReceiveData, 0x400, 0, 0) < 0) {
        debug_print_stub(str_dbc_get_dep_number_error);
        return 0;
    }
    return buf[1];
}

LINKER_REMNANT("asm/remnants/core_text", func_00124A68);

extern char str_dbc_receive_data_error[];

/* RPC 0x8000091A round trip on D_0015B180 (the buffer is both request
   and reply), copying the reply's bytes to arg3. `r` is assigned in both
   arms of the copy guard with one return; two returns would be
   cross-jumped into one. */
int sceDbcReceiveData(int arg0, int arg1, int *arg2, char *arg3) {
    int *buf = &RpcReceiveData;
    unsigned char *src;
    int i;
    int r;

    buf[0] = arg0;
    buf[1] = arg1;
    buf[2] = *arg2;
    if (sif_call_rpc(RpcQueueData, 0x8000091A, 0, buf, 0x400, buf, 0x400, 0, 0) < 0) {
        debug_print_stub(str_dbc_receive_data_error);
        return 0;
    }
    r = buf[0x23];
    if (r >= 0) {
        *arg2 = buf[2];
        i = 0;
        if (i < buf[2]) {
            src = (unsigned char *)buf + 0xC;
            do {
                arg3[i] = src[i];
                i++;
            } while (i < buf[2]);
            r = buf[0x23];
        } else {
            r = buf[0x23];
        }
    }
    return r;
}

/* The library's debug print, compiled out to an empty varargs function.
   Only Sony's 2.9-ee gives retail's shape, spilling $5-$11 and no FP
   argument registers; the game's 2.95.3 also saves $f12-$f18. That is
   how this file was found to be built with 2.9-ee (Makefile.sn). */
void debug_print_stub(void *fmt, ...) {
}

/*
 * Reverted: size mismatch (ours=72, retail=60 -- 12 bytes over).
 *
 *   extern int D_00132ED0;
 *   extern char pad2_sockets_b[] __asm__("D_0015B640");
 *
 *   int func_00124B88(void) {
 *       int p = (int)pad2_sockets_b;
 *       int end = p + 0x3300;
 *       D_00132ED0 = 1;
 *       do {
 *           *(int *)(p + 0) = 0;
 *           *(int *)(p + 4) = 0;
 *           *(int *)(p + 8) = 0;
 *           p += 0x330;
 *       } while (p < end);
 *       return 1;
 *   }
 *
 * Zeroes the +0x00/+0x04/+0x08 fields of all 16 entries in the
 * 0x330-stride table D_0015B640, then marks it initialized. Semantics
 * certain; retail's loop is a plain ascending `bnez` with the
 * comparison as `slt` (signed) and no duplicated body. Because
 * end-start (0x3300) is an exact multiple of the stride (0x330) --
 * always 16 iterations -- this compiler can prove the trip count at
 * compile time and reverses the loop into a `bnezl`-based down-count
 * with the first store duplicated as the branch's delay-slot
 * instruction, regardless of whether the cursor is typed `int`,
 * `unsigned int`, or `char *` (tried all three; `int` at least
 * recovers retail's `slt`, but the reversal itself doesn't go away).
 * A different case from the established "unsigned loop counter blocks
 * reversal" lever ([[rac1-64bit-field-type]]'s sibling precedent,
 * e.g. func_0011DCB8) -- that lever stops reversal driven by a small
 * loop-counter comparison, not one driven by a statically-divisible
 * address range.
 */
extern int pad2_initialized;

typedef struct {
    char unk_00[4];
    int  unk_04;
    int  unk_08;
    char *unk_0C;
    char unk_10[0x320];
} Ent330;
extern Ent330 pad2_sockets[];

int sce_pad2_init(void) {
    int p;
    int end;

    pad2_initialized = 1;
    p = (int)pad2_sockets;
    end = p + 0x3300;
    do {
        *(int *)(p + 0) = 0;
        *(int *)(p + 4) = 0;
        *(int *)(p + 8) = 0;
        p += 0x330;
    } while (p < end);
    return 1;
}

extern int sceDbcCreateSocket(void *, int, int);

extern char pad2_sockets_b[] __asm__("D_0015B640");

typedef struct {
    int w0;
    int mode;
    int w2;
    int w3;
    int w4;
    char name[16];
} Pad2SockParam;

/* scePad2CreateSocket(param, buf). c5: `desc.w0 = 0` moved inside the
   else (null) block instead of shared before the if, probing the last
   1-byte beqz/beql (branch-likely) difference from c4. */
int sce_pad2_create_socket(void *param, char *buf) {
    Pad2SockParam desc;
    int r;
    int i;
    char *p;

    if (((int)buf & 0x3F) != 0) {
        return -1;
    }
    if (param != 0) {
        int *src = (int *)param;
        desc.w0 = src[0];
        desc.w2 = src[1];
        desc.w3 = src[2];
        desc.w4 = src[3];
        memcpy(desc.name, (char *)param + 0x10, 16);
    } else {
        desc.w0 = 0;
        desc.w2 = 0;
        desc.w3 = 0;
        desc.w4 = 0;
        desc.name[0] = 0;
    }
    desc.w0 |= 1;
    desc.mode = 1;
    r = sceDbcCreateSocket(&desc, (int)buf, (int)buf + 0x80);
    if (r < 0) {
        return r;
    }
    *(int *)(pad2_sockets_b + r * 0x330) = 1;
    *(char **)(pad2_sockets_b + r * 0x330 + 0xC) = buf;
    p = buf;
    for (i = 1; i >= 0; i--) {
        p[0] = 0;
        *(int *)(p + 0x7C) = 0;
        p[1] = 0;
        p[3] = 0;
        p[2] = 0;
        *(int *)(p + 4) = 0;
        libc_memset(p + 0x1C, 0xFF, 0x20);
        p += 0x80;
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_00124D10);

extern char pad2_sockets_button_slots[];
extern void *sce_pad2_get_side(int);
extern int sce_pad2_link_driver(int);
extern int sce_pad2_set_button_order(unsigned char *, int *);
extern void *libc_memcpy(void *, void *, int);

/* Read entry id's current record into dst: open the entry if it is not
   (func_00125020), fetch the record (func_00125078), copy its p[2]-byte
   payload from +0x1C and hand the bytes after it to func_00125160 with
   the entry's slot array. Returns the length, or -1. */
int scePad2Read(int id, void *dst) {
    unsigned char *p;

    if (*(int *)((char *)pad2_sockets + id * 0x330) == 0) {
        return -1;
    }
    if (pad2_sockets[id].unk_04 == 0) {
        if (sce_pad2_link_driver(id) < 0) {
            return -1;
        }
    }
    p = (unsigned char *)sce_pad2_get_side(id);
    if (p[2] != 0) {
        unsigned char *src = p + 0x1C;

        if (src != 0) {
            libc_memcpy(dst, src, p[2]);
            sce_pad2_set_button_order(p + 0x1C + p[2], (int *)(pad2_sockets_button_slots + id * 0x330));
        }
    }
    if (*(int *)(p + 4) == 0) {
        return -1;
    }
    return p[2];
}

/* 0x330-stride entry table. Declared as a real struct array, not
   `char[]` + byte offset: the two are not codegen-equivalent. Indexing a
   typed array emits `addu base,index`; the char-pointer form emits
   `addu index,base`, and reordering the C addition cannot change it
   because GCC canonicalises the PLUS first. See func_00125078 and the
   same lever on D_001E8F80 in text.c. */



/* The command word, built as bitfields of an `unsigned long` union: the
   union stays in a register, each insert is a 64-bit and/or, and the
   int view is the dsll32/dsra32 truncation retail passes in $a1. */
typedef union {
    struct {
        unsigned long f0 : 14;
        unsigned long f1 : 2;
        unsigned long f2 : 8;
        unsigned long f3 : 8;
    } b;
    int i;
} McCmd;

/* Opens entry `port` of the table if it is not (func_00125020), sends
   command {2, 3, 2, 1} through func_00124A70, expands the 40-bit mask it
   returns into the entry's slots (func_00125160) and returns the result
   word func_00124A70 filled in. */
int sce_pad2_get_button_profile(int port, void *arg1) {
    McCmd cmd;
    int result;
    int r;

    if (pad2_sockets[port].unk_04 == 0) {
        if (sce_pad2_link_driver(port) < 0) {
            return -1;
        }
    }
    cmd.b.f0 = 2;
    cmd.b.f1 = 3;
    cmd.b.f2 = 2;
    cmd.b.f3 = 1;
    r = sceDbcReceiveData(port, cmd.i, &result, arg1);
    if (r < 0) {
        return r;
    }
    sce_pad2_set_button_order(arg1, (int *)pad2_sockets[port].unk_10);
    return result;
}

extern int sce_pad2_check_dma(int);

/* A single stack-resident local shared by both call paths below: its
   byte 0 is the state code every path eventually returns (retail's two
   "side" sub-paths converge on one `sb`/`lbu` round trip through it,
   and the RPC path stages its return through the same byte), and the
   word at +4 is the uninitialised in/out count func_00124A70 wants. */
typedef struct {
    unsigned char state;
    char pad[3];
    int result;
} Pad2Buf;

/* scePad2GetState(port): link the pad if needed (func_00125020), then
   ask func_001250E0 (scePad2CheckDma) whether new data has arrived.
   If so, fetch the active "side" struct (func_00125078) and take its
   state byte directly if it looks initialised (+0x4 != 0); otherwise
   try one more relink and use it anyway on success, else forget this
   port's link and return 0. If CheckDma says nothing changed, build
   the RPC command word (the same McCmd bitfield union as
   func_00124DF0's {2,3,2,1}, here {0xC,2,1,1}) and round-trip it
   through func_00124A70; return its state byte on success, 0 on
   failure. */
int sce_pad2_get_state(int port) {
    Pad2Buf buf;
    char *side;
    McCmd cmd;

    if (pad2_sockets[port].unk_04 == 0) {
        if (sce_pad2_link_driver(port) < 0) {
            return 0;
        }
    }
    if (sce_pad2_check_dma(port) != 0) {
        side = sce_pad2_get_side(port);
        if (*(int *)(side + 0x4) != 0) {
            buf.state = side[0];
        } else if (sce_pad2_link_driver(port) >= 0) {
            buf.state = side[0];
        } else {
            pad2_sockets[port].unk_04 = 0;
            return 0;
        }
        return buf.state;
    }
    cmd.b.f0 = 0xC;
    cmd.b.f1 = 2;
    cmd.b.f2 = 1;
    cmd.b.f3 = 1;
    if (sceDbcReceiveData(port, cmd.i, &buf.result, (char *)&buf) < 0) {
        return 0;
    }
    return buf.state;
}

extern int sceDbcGetDepNumber(int);

/* scePad2LinkDriver(port): opens the link driver for `port` and, on
   success, records the handle in its table entry. The failure path needs
   its own early return: with one shared `return t` the compiler moves the
   result copy to the end of the function. */
int sce_pad2_link_driver(int arg0) {
    int t = sceDbcGetDepNumber(arg0);
    if (t < 0) {
        return t;
    }
    pad2_sockets[arg0].unk_08 = t;
    pad2_sockets[arg0].unk_04 = 1;
    return t;
}

extern void sync_dcache_range(void *a, void *b);

/*
 * Close, not exact (3/100), same size. Logic confirmed: take entry
 * arg0 of the 0x330-stride table D_0015B640, follow its +0xC pointer,
 * record that pointer and pointer+0x80 in a two-slot stack array, call
 * func_00119288(p, p+0x100), then return whichever slot has the smaller
 * +0x7C field -- retail indexes the array with the `slt` result
 * directly, which plain C reproduces.
 *
 * The three differing bytes were one instruction: retail forms the entry
 * address as `addu $2,$2,$4` (base, index) where we emitted
 * `addu $a0,$a0,$v0` (index, base). Writing the addition the other way
 * round changes nothing -- GCC canonicalises the PLUS -- and this was
 * recorded here as "the allocator's destination choice, not operand
 * order", which was wrong. It is operand order, and it is steerable:
 * give the table a real element type and index it. See Ent330 above.
 */
void *sce_pad2_get_side(int arg0) {
    char *p = pad2_sockets[arg0].unk_0C;
    char *slot[2];

    slot[0] = p;
    slot[1] = p + 0x80;
    sync_dcache_range(p, p + 0x100);
    return slot[*(int *)(slot[0] + 0x7C) < *(int *)(slot[1] + 0x7C)];
}

extern int pad2_last_dma_counters[];

/* Has entry arg0's current pair changed? Follow entry arg0 of the
   0x330-stride table to its buffer p; the pair is p+0x7C and p+0x80+0x7C.
   If the first is zero, or both equal the cached pair D_00132ED8[0..1],
   report 0; otherwise cache them and report 1. The two-slot stack array
   is retail's (the same idiom as func_00125078). Exact under this file's
   2.9-ee; under 2.95.3 it was a word long. */
/* The old note's decode, exact since 001236F0.c builds with 2.9-ee. */
int sce_pad2_check_dma(int arg0) {
    char *p = pad2_sockets[arg0].unk_0C;
    char *slot[2];

    slot[0] = p;
    slot[1] = p + 0x80;
    if (*(int *)(p + 0x7C) == 0 ||
        (pad2_last_dma_counters[0] == *(int *)(p + 0x7C) &&
         pad2_last_dma_counters[1] == *(int *)(slot[1] + 0x7C))) {
        return 0;
    }
    pad2_last_dma_counters[0] = *(int *)(slot[0] + 0x7C);
    pad2_last_dma_counters[1] = *(int *)(slot[1] + 0x7C);
    return 1;
}

/* Expand a 40-bit mask into 40 four-word slots. For each set bit, record
   slot type 1 plus the running (a, b) pair; entries 0x10..0x1F and
   0x23..0x26 are "wide" (size 8) and only advance a, everything else is
   size 1 and advances b, rolling a over every eighth. Clear bits zero
   the slot's type and both counters. Initialising b before the loop
   sets i gives i the shorter live range, so this file's compiler
   (2.9-ee) allocates it first, as retail. */
int sce_pad2_set_button_order(unsigned char *src, int *out) {
    int bit;
    int a;
    int i;
    int b;

    bit = 0;
    a = 0;
    b = 0;
    for (i = 0; i < 0x28; i++) {
        if (((*src >> bit) & 1) != 0) {
            out[0] = 1;
            out[2] = a;
            out[3] = b;
            if ((i >= 0x10 && i < 0x20) || (i >= 0x23 && i < 0x27)) {
                out[1] = 8;
                a++;
            } else {
                b++;
                out[1] = 1;
                if ((b & 7) == 0) {
                    a++;
                    b = 0;
                }
            }
        } else {
            out[0] = 0;
            out[2] = 0;
            out[3] = 0;
        }
        bit++;
        if ((bit & 7) == 0) {
            src++;
            bit = 0;
        }
        out += 4;
    }
    return 1;
}

LINKER_REMNANT("asm/remnants/core_text", func_00125210);
