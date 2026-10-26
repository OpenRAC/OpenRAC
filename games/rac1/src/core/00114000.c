#include "common.h"
#include "structs.h"

/*
 * core_text object 0x114000-0x114060. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib's fstatr.o (_fstat_r; the SDK's libc.a). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like libc.a.
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
