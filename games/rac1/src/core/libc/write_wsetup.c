#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/kernel/libkernl_glue.h"
#include "data/executable.h"
#include "core/newlib_types.h"
#include "core/libc/findfp.h"
#include "core/libc/free_malloc_trim.h"
#include "core/libc/lseek_malloc.h"

/*
 * core_text object 0x1188C8-0x118A40. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a): writer.o (_write_r) and wsetup.o
 * (__swsetup), back to back. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like libc.a.
 */

int _write_r(int *errOut, void *a, void *b, void *c) {
    int r;
    SbrkErrno = 0;
    r = libc_glue_write(a, b, c);
    if (r == -1 && SbrkErrno != 0) {
        *errOut = SbrkErrno;
    }
    return r;
}

/* newlib's stdio types (sys/reent.h, 1999-2000), under local names. */

/* _impure_ptr, typed; the declaration in scope above is `void *`. */
extern struct _reent_nl *_impure_ptr_r __asm__("D_0012F86C") NOT_SDA;

/*
 * __swsetup, newlib's text (stdio/wsetup.c), called before the first
 * write to a FILE; FREEUB and HASUB expanded, _free_r = _free_r,
 * __smakebuf = __smakebuf.
 *
 * Exact under 2.9-ee with newlib's FILE layout. Its -fstrict-aliasing is
 * what keeps _flags in a register across the int/pointer stores to _r
 * and _p, masking it as an int (`and` with -0x25) as retail does; an
 * earlier transcription was 8 bytes short under 2.9-ee and 57/268 under
 * 2.95.3.
 */
int libc_swsetup(FILE_nl *fp) {
    /* Make sure stdio is set up. */

    do {
        if ((fp)->_data == 0)
            (fp)->_data = _impure_ptr_r;
        if (!(fp)->_data->__sdidinit)
            __sinit((fp)->_data);
    } while (0);

    /*
     * If we are not writing, we had better be reading and writing.
     */

    if ((fp->_flags & 0x0008) == 0)         /* __SWR */
    {
        if ((fp->_flags & 0x0010) == 0)     /* __SRW */
            return -1;                      /* EOF */
        if (fp->_flags & 0x0004)            /* __SRD */
        {
            /* clobber any ungetc data */
            if (((fp)->_ub._base != 0))     /* HASUB */
            {                               /* FREEUB */
                if ((fp)->_ub._base != (fp)->_ubuf)
                    _free_r(fp->_data, (char *)(fp)->_ub._base);
                (fp)->_ub._base = 0;
            }
            fp->_flags &= ~(0x0004 | 0x0020);   /* __SRD | __SEOF */
            fp->_r = 0;
            fp->_p = fp->_bf._base;
        }
        fp->_flags |= 0x0008;
    }

    /*
     * Make a buffer if necessary, then set _w.
     */
    if (fp->_bf._base == 0)
        __smakebuf(fp);

    if (fp->_flags & 0x0001)                /* __SLBF */
    {
        /*
         * It is line buffered, so make _lbfsize be -_bufsize
         * for the putc() macro.  We will change _lbfsize back
         * to 0 whenever we turn off __SWR.
         */
        fp->_w = 0;
        fp->_lbfsize = -fp->_bf._size;
    } else
        fp->_w = fp->_flags & 0x0002 ? 0 : fp->_bf._size;  /* __SNBF */

    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00118A34);
