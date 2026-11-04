#include "common.h"
#include "structs/misc.h"
#include "data/executable.h"

/*
 * core_text object 0x114000-0x114060. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * newlib's fstatr.o (_fstat_r; the SDK's libc.a). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like libc.a.
 */

int _fstat_r(int *errOut, void *arg1, void *arg2) {
    int r;
    SbrkErrno = 0;
    r = libc_glue_fstat(arg1, arg2);
    if (r == -1 && SbrkErrno != 0) {
        *errOut = SbrkErrno;
    }
    return r;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0011405C);
