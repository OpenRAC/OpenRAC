#include "common.h"
#include "structs/misc.h"

/*
 * core_text object 0x1144D0-0x114518. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * newlib's locale.o (the SDK's libc.a): _localeconv_r and localeconv.
 * The linker dead-stripped the unreferenced _setlocale_r and setlocale
 * down to their last word each (setlocale_r_stripped_tail, setlocale_stripped_tail). Built
 * with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like libc.a.
 */


extern void *_impure_ptr NOT_SDA;

LINKER_REMNANT("asm/remnants/core_text", func_001144D0);

extern char lconv[];

/* arg0 unused: retail loads it into $a0 for this call too but the
   function body never reads it. */
void *_localeconv_r(void *arg0) {
    return lconv;
}

LINKER_REMNANT("asm/remnants/core_text", func_001144E8);

void *libc_localeconv(void) {
    return _localeconv_r(_impure_ptr);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00114514);
