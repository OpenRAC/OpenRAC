#include "common.h"
#include "structs.h"

/*
 * core_text object 0x113A70-0x113B70. Boundaries are retail's linker fill
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

void findfp_std(void *arg0, int arg1, int arg2, int arg3) {
    Node1E4 *self = (Node1E4 *)arg0;
    self->owner = (void *)arg3;
    self->flags = arg1;
    self->handle = arg2;
    self->fn20 = __sread;
    self->fn24 = __swrite;
    self->fn28 = __sseek;
    self->fn2C = __sclose;
    self->unk04 = 0;
    self->unk08 = 0;
    self->unk10 = 0;
    self->unk18 = 0;
    self->self = self;
    self->unk00 = 0;
}

extern void libc_fflush(void);
extern void _fwalk(void *, void *);

/* Retail is a bare tail jump: Sony's 2.9-ee, which builds this SDK object,
   emits it for a void function that ends in a call. */
void _cleanup_r(void *arg0) {
    _fwalk(arg0, libc_fflush);
}

LINKER_REMNANT("asm/remnants/core_text", func_00113AD8);

/* newlib's __sinit (findfp.c): the reent's three built-in FILEs become
   stdin, stdout and stderr through std() (func_00113A70), then the glue
   list points at them; the cleanup hook is _cleanup_r (func_00113AC8). */
void __sinit(void *arg0) {
    char *p = (char *)arg0;
    char *n0 = p + 0x1E4;

    *(void (**)(void *))(p + 0x3C) = _cleanup_r;
    *(int *)(p + 0x38) = 1;
    findfp_std(n0, 4, 0, (int)p);
    findfp_std(p + 0x23C, 9, 1, (int)p);
    findfp_std(p + 0x294, 0xA, 2, (int)p);
    *(void **)(p + 0x1E0) = n0;
    *(int *)(p + 0x1DC) = 3;
    *(int *)(p + 0x1D8) = 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00113B6C);
