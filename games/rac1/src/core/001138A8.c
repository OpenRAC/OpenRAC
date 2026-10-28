#include "common.h"
#include "structs.h"

/*
 * core_text object 0x1138A8-0x1138B8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib's errno.o (__errno; the SDK's libc.a). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like libc.a.
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;

extern void *_impure_ptr NOT_SDA;

int __errno(void) {
    return (int)_impure_ptr;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001138B4);
