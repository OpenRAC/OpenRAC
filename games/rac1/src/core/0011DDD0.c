#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x11DDD0-0x11DF10. Boundaries are retail's linker fill
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

ASM_FUNC("asm/handwritten/core_text", func_0011DDD0);

ASM_FUNC("asm/handwritten/core_text", func_0011DDE0);

/* kCopy: alarm.o's own copy of the word-at-a-time copy helper (nbytes
 * rounded down to words); returns 0. Same body as func_0011DA08
 * (initsys.o), func_0011D370 and func_0011DC08 (libosd.o's copy) -- each
 * kernel module linked its own instance of this helper. */
int alarm_kCopy(int *dst, int *src, unsigned int nbytes) {
    unsigned int i;
    for (i = 0; i < nbytes >> 2; i++) {
        *dst++ = *src++;
    }
    return 0;
}

ASM_FUNC("asm/handwritten/core_text", func_0011DE28);

extern void alarm_set_syscall(int, int);
extern void alarm_kernel_copy(void *, void *, int);
extern int alarm_get_syscall_entry(int);
extern int alarm_syscall_patch_table[];
extern char alarm_kernel_patch_code[];
extern char alarm_scratchpad_patch_code[];

typedef struct { int a; int b; } D_00131358_pair;

/*
 * Same-size near-miss (8/0xD4 bytes). Same shape as func_0011D3C8:
 * guarded by a hardware status bit at 0x10001810 (skip everything if
 * already done), two (addr,value) pokes through func_0011DDD0, two
 * func_0011DDE0 block copies (to a fixed load address and to
 * scratchpad at 0x82000), an interrupt disable/enable bracket, a
 * third func_0011DDD0 poke, then a 6-entry table loop where
 * func_0011DE28 reads the entry's current value back before
 * func_0011DDD0 rewrites it. All three callees are handwritten
 * syscall wrappers.
 */
void InitAlarm(void) {
    int *t;
    D_00131358_pair *p;
    unsigned int i;
    int old;

    if (*(volatile unsigned int *)0x10001810 & 0x100) {
        return;
    }

    t = alarm_syscall_patch_table;
    alarm_set_syscall(t[0], t[1]);
    alarm_kernel_copy((void *)0x80076000, alarm_kernel_patch_code, 0x740);
    alarm_kernel_copy((void *)0x82000, alarm_scratchpad_patch_code, 0x28);
    flush_cache(0);
    flush_cache(2);
    alarm_set_syscall(t[2], t[3]);

    p = (D_00131358_pair *)(t + 4);
    for (i = 2; i < 8; i++) {
        old = alarm_get_syscall_entry(p->a);
        alarm_set_syscall(p->a, old);
        p++;
    }
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0011DF0C);
