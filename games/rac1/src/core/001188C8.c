#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x1188C8-0x118A40. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a): writer.o (_write_r) and wsetup.o
 * (__swsetup), back to back. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like libc.a.
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
struct __sbuf_nl {
    unsigned char *_base;
    int _size;
};
struct _atexit_nl {
    struct _atexit_nl *_next;
    int _ind;
    void (*_fns[32])(void);
};
struct _reent_nl {
    char _pad0[0x38];
    int __sdidinit;                             /* 0x38 */
    void (*__cleanup)(struct _reent_nl *);      /* 0x3C */
    char _pad40[0x148 - 0x40];
    struct _atexit_nl *_atexit;                 /* 0x148 */
};
typedef struct {
    unsigned char *_p;          /* 0x00 */
    int _r;
    int _w;
    short _flags;               /* 0x0C */
    short _file;
    struct __sbuf_nl _bf;       /* 0x10 */
    int _lbfsize;
    void *_cookie;              /* 0x1C */
    int (*_read)(void *, char *, int);
    int (*_write)(void *, const char *, int);
    long (*_seek)(void *, long, int);
    int (*_close)(void *);
    struct __sbuf_nl _ub;       /* 0x30 */
    unsigned char *_up;
    int _ur;
    unsigned char _ubuf[3];
    unsigned char _nbuf[1];
    struct __sbuf_nl _lb;       /* 0x44 */
    int _blksize;
    int _offset;
    struct _reent_nl *_data;    /* 0x54 */
} FILE_nl;

/* _impure_ptr, typed; the declaration in scope above is `void *`. */
extern struct _reent_nl *_impure_ptr_r __asm__("D_0012F86C") NOT_SDA;

extern void __sinit_r(struct _reent_nl *) __asm__("func_00113AE0");
extern void _free_r_r(struct _reent_nl *, void *) __asm__("func_00113B70");
extern void __smakebuf_r(FILE_nl *) __asm__("func_00114578");

/*
 * __swsetup, newlib's text (stdio/wsetup.c), called before the first
 * write to a FILE; FREEUB and HASUB expanded, _free_r = func_00113B70,
 * __smakebuf = func_00114578.
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
            __sinit_r((fp)->_data);
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
                    _free_r_r(fp->_data, (char *) (fp)->_ub._base);
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
        __smakebuf_r(fp);

    if (fp->_flags & 0x0001)                /* __SLBF */
    {
        /*
         * It is line buffered, so make _lbfsize be -_bufsize
         * for the putc() macro.  We will change _lbfsize back
         * to 0 whenever we turn off __SWR.
         */
        fp->_w = 0;
        fp->_lbfsize = -fp->_bf._size;
    }
    else
        fp->_w = fp->_flags & 0x0002 ? 0 : fp->_bf._size;  /* __SNBF */

    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00118A34);
