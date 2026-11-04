#include "common.h"
#include "structs/misc.h"
#include "core/newlib_types.h"
#include "core/startup/crt0.h"
#include "core/libc/findfp.h"

/*
 * core_text object 0x1138B8-0x113A70. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a): exit.o (exit) and fflush.o (fflush), back
 * to back. Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like
 * libc.a; both are newlib's own text.
 */

/* newlib's stdio types (sys/reent.h, 1999-2000), under local names. */

/* _impure_ptr, typed; the declaration in scope above is `void *`. */
extern struct _reent_nl *_impure_ptr_r __asm__("D_0012F86C") NOT_SDA;

/* exit, newlib's text: run the atexit handlers (LIFO), the reent's
   cleanup hook (_cleanup_r), then _exit, as a tail jump. */
void libc_exit(int code) {
    register struct _atexit_nl *p;
    register int n;

    for (p = _impure_ptr_r->_atexit; p; p = p->_next)
        for (n = p->_ind; --n >= 0;)
            (*p->_fns[n])();
    if (_impure_ptr_r->__cleanup)
        (*_impure_ptr_r->__cleanup)(_impure_ptr_r);
    func_0012DA28(code);
}

extern int _fwalk(struct _reent_nl *, int (*)(FILE_nl *));

/* fflush, newlib's text: a NULL fp flushes every FILE through _fwalk
   (_fwalk); CHECK_INIT keeps its `do { } while (0)`. */
int libc_fflush(FILE_nl *fp) {
    register unsigned char *p;
    register int n, t;

    if (fp == 0)
        return _fwalk(_impure_ptr_r, libc_fflush);

    do {
        if ((fp)->_data == 0)
            (fp)->_data = _impure_ptr_r;
        if (!(fp)->_data->__sdidinit)
            __sinit((fp)->_data);
    } while (0);

    t = fp->_flags;
    if ((t & 0x0008) == 0 || (p = fp->_bf._base) == 0)   /* __SWR */
        return 0;
    n = fp->_p - p;             /* write this much */

    /*
     * Set these immediately to avoid problems with longjmp
     * and to allow exchange buffering (via setvbuf) in user
     * write function.
     */
    fp->_p = p;
    fp->_w = t & (0x0001 | 0x0002) ? 0 : fp->_bf._size;  /* __SLBF|__SNBF */

    while (n > 0) {
        t = (*fp->_write)(fp->_cookie, (char *)p, n);
        if (t <= 0) {
            fp->_flags |= 0x0040;   /* __SERR */
            return -1;              /* EOF */
        }
        p += t;
        n -= t;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00113A6C);
