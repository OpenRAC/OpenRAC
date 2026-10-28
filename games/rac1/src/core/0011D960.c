#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x11D960-0x11DBE8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl): diei.o (DIntr, EIntr) and
 * initsys.o (supplement_crt0, kCopy, kFindAddress, FindAddress,
 * InitSystemCallTableAddress, _InitSys). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libkernl.a.
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

ASM_FUNC("asm/handwritten/core_text", func_0011D960);

ASM_FUNC("asm/handwritten/core_text", func_0011D9A8);

extern int create_semaphore(void *);
extern int FirstSemaphore;
extern int SecondSemaphore;

void supplement_crt0(void) {
    int a[8];
    int b[8];
    a[1] = 1;
    a[2] = 1;
    b[1] = 1;
    b[2] = 1;
    FirstSemaphore = create_semaphore(a);
    SecondSemaphore = create_semaphore(b);
}

/*
 * initsys.o's kCopy: a word-at-a-time copy of n bytes (rounded down to
 * words); returns 0. The same function is func_0011D370 (alarm.o), and
 * func_0011DC08/func_0011DDF0 in the other kernel modules.
 *
 * Exact under 2.9-ee: the empty delay slot of the loop branch is
 * 2.9-ee's own short-loop padding (it pads the loop and leaves the bnez
 * unfilled). Under 2.95.3 reorg sinks `dst++` into the slot, 52 bytes
 * against 56, which is what kept this a stub.
 */
int kCopy(int *dst, int *src, unsigned int nbytes) {
    unsigned int i;
    for (i = 0; i < nbytes >> 2; i++) {
        *dst++ = *src++;
    }
    return 0;
}

/* kFindAddress (libkernl.a:initsys.o): scan the words from p up to, not
 * including, end for val; returns the matching address, or 0 if the scan
 * reaches end. */
int *kFindAddress(int *p, int *end, int val) {
    while (*p != val && p < end)
        p++;
    if (!(p < end))
        p = 0;
    return p;
}

ASM_FUNC("asm/handwritten/core_text", func_0011DA80);

LINKER_REMNANT("asm/remnants/core_text", func_0011DA90);

extern void initsys_set_syscall(int, int);
extern int find_address(int, int, void *);
extern int initsys_syscall_patch_table[];
extern int initsys_kernel_scan_address;

/* InitSystemCallTableAddress (libkernl.a:initsys.o): patches two kernel
   entry points found by scanning for known function bodies
   (FindAddress = func_0011DA80, matching kCopy = func_0011DA08 and a
   second reference routine, func_0011DA40) into the two descriptors of
   D_00130410 (kFindAddress/func_0011DB98, called once per descriptor),
   then records where the two scans converge (they start from the same
   base and offset by different constants, and are each advanced by 4
   bytes -- whichever trails -- until they land on the same address) in
   D_00130408. `a`/`b` have to be unsigned: retail's convergence test is
   `sltu`. */
void InitSystemCallTableAddress(void) {
    unsigned int a, b;
    int p1, p2;

    initsys_set_syscall(initsys_syscall_patch_table[0], initsys_syscall_patch_table[1]);
    initsys_set_syscall(initsys_syscall_patch_table[2], initsys_syscall_patch_table[3]);

    p1 = find_address(0x80000000, 0x80080000, (void *)kFindAddress);
    p2 = find_address(0x80000000, 0x80080000, (void *)kCopy);
    a = p1 - 0x20C;
    b = p2 - 0x168;
    while (a != b) {
        if (a < b) {
            p1 = find_address(p1 + 4, 0x80080000, (void *)kFindAddress);
            a = p1 - 0x20C;
        } else {
            p2 = find_address(p2 + 4, 0x80080000, (void *)kCopy);
            b = p2 - 0x168;
        }
    }
    initsys_kernel_scan_address = a;
}

ASM_FUNC("asm/handwritten/core_text", func_0011DB98);

extern void supplement_crt0(void);
extern void InitSystemCallTableAddress(void);
extern void InitAlarm(void);
extern void InitThread(void);
extern void InitExecPS2(void);
extern void InitTLBFunctions(void);

/* _InitSys (libkernl.a:initsys.o): the kernel's one-shot startup sequence,
   run once from the crt0 supplement. Chains six sub-system initialisers;
   the last (InitTLBFunctions, func_0011D3C8) is a tail call since this is
   void and it is the final statement. */
void _InitSys(void) {
    supplement_crt0();
    InitSystemCallTableAddress();
    InitAlarm();
    InitThread();
    InitExecPS2();
    InitTLBFunctions();
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0011DBE4);
