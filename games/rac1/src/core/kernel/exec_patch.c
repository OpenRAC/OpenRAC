#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"

/*
 * core_text object 0x11DBE8-0x11DD68. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 */

ASM_FUNC("asm/handwritten/core_text", func_0011DBE8);

ASM_FUNC("asm/handwritten/core_text", func_0011DBF8);

/* kCopy: libosd.o's own copy of the word-at-a-time copy helper (nbytes
 * rounded down to words); returns 0. Same body as kCopy
 * (initsys.o), tlbfunc_kcopy (alarm.o) and alarm_kCopy (alarm.o's other
 * copy) -- each kernel module linked its own instance of this helper. */
int osd_kCopy(int *dst, int *src, unsigned int nbytes) {
    unsigned int i;
    for (i = 0; i < nbytes >> 2; i++) {
        *dst++ = *src++;
    }
    return 0;
}

ASM_FUNC("asm/handwritten/core_text", func_0011DC40);

/* Reads a word through get_osd_config_param, rewrites its 13..15 bit field to 1,
   pushes it back, re-reads it and restores the original. Returns whether
   the field read back as 0. `srl` (not `sra`) at the end is the tell that
   the scratch word is unsigned. */
int PatchIsNeeded(void) {
    unsigned int saved;
    unsigned int cur;
    get_osd_config_param(&saved);
    cur = (saved & 0xFFFF1FFF) | 0x2000;
    set_osd_config_param(&cur);
    get_osd_config_param(&cur);
    set_osd_config_param(&saved);
    return ((cur >> 13) & 7) == 0;
}

extern int osd_syscall_patch_table[];
extern char osd_kernel_patch_code[];
extern int PatchIsNeeded(void);

typedef struct {
    int num;
    int fn;
} SysEnt;

/* When PatchIsNeeded says so, installs the kernel entries of the
   {num, fn} table at osd_syscall_patch_table (the third through osd_get_syscall_entry) and
   copies 0x7A8 bytes of osd_kernel_patch_code to 0x80074000. Indexing the table in
   the loop keeps its base live, so the argument loads keep source
   order. */
void InitExecPS2(void) {
    SysEnt *t;
    unsigned int i;

    if (PatchIsNeeded() == 0) {
        return;
    }
    t = (SysEnt *)osd_syscall_patch_table;
    osd_set_syscall(t[0].num, t[0].fn);
    osd_kernel_copy((int)0x80074000, osd_kernel_patch_code, 0x7A8);
    flush_cache(0);
    flush_cache(2);
    osd_set_syscall(t[1].num, t[1].fn);
    for (i = 2; i < 3; i++) {
        osd_set_syscall(t[i].num, osd_get_syscall_entry(t[i].num));
    }
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0011DD64);
