#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x11DD68-0x11DDD0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), exit.o: kCopy (the byte
 * version), a TLB re-init stub and Exit. Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libkernl.a, which emits
 * both of its tail calls natively.
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
extern void InitTLB(void);

/*
 * exit.o's kCopy: a byte copy loop returning 0. Exact under 2.9-ee, which
 * pads the short loop itself and leaves the branch's delay slot empty,
 * with `dst++` before the branch as retail has it. (Under 2.95.3 reorg
 * sinks `dst++` into the slot, 44 bytes against 48: the old revert.)
 */
int exit_kCopy(char *dst, char *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++) {
        *dst++ = *src++;
    }
    return 0;
}

/* Tail call: retail is `j func_0011D4A0` + nop, with no frame at all.
   2.9-ee emits it natively for a void function that ends in a call. */
void exit_reinit_tlb(void) {
    InitTLB();
}

/*
 * Exit(status): re-init the TLB (func_0011DD98), then the _Exit syscall
 * (func_00118A60) with the status, as a bare tail jump. Exact under
 * 2.9-ee, which emits "call, then tail call" itself; under 2.95.3 it was
 * 4 bytes over (2.95.3 has no sibling calls). The status lives in $16
 * across the first call.
 */
extern void kernel_exit(int);

void Exit(int status) {
    exit_reinit_tlb();
    kernel_exit(status);
}

LINKER_REMNANT("asm/remnants/core_text", func_0011DDC8);
