#include "common.h"
#include "structs.h"

/*
 * core_text object 0x1144D0-0x114518. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib's locale.o (the SDK's libc.a): _localeconv_r and localeconv.
 * The linker dead-stripped the unreferenced _setlocale_r and setlocale
 * down to their last word each (func_001144D0, func_001144E8). Built
 * with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like libc.a.
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

LINKER_REMNANT("asm/remnants/core_text", func_001144D0);

extern char locale_lconv[];

/* arg0 unused: retail loads it into $a0 for this call too but the
   function body never reads it. */
void *_localeconv_r(void *arg0) {
    return locale_lconv;
}

LINKER_REMNANT("asm/remnants/core_text", func_001144E8);

void *libc_localeconv(void) {
    return _localeconv_r(_impure_ptr);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00114514);
