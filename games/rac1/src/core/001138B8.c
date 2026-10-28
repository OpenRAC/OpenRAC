#include "common.h"
#include "structs.h"

/*
 * core_text object 0x1138B8-0x113A70. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a): exit.o (exit) and fflush.o (fflush), back
 * to back. Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like
 * libc.a; both are newlib's own text.
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;
extern void *_impure_ptr NOT_SDA;

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

extern void func_0012DA28(int);  /* _exit (crt0) */

/* exit, newlib's text: run the atexit handlers (LIFO), the reent's
   cleanup hook (_cleanup_r), then _exit, as a tail jump. */
void libc_exit(int code) {
    register struct _atexit_nl *p;
    register int n;

    for (p = _impure_ptr_r->_atexit; p; p = p->_next)
        for (n = p->_ind; --n >= 0;)
            (*p->_fns[n]) ();
    if (_impure_ptr_r->__cleanup)
        (*_impure_ptr_r->__cleanup) (_impure_ptr_r);
    func_0012DA28(code);
}

extern int _fwalk_i(struct _reent_nl *, int (*)(FILE_nl *)) __asm__("func_00114438");
extern void __sinit_r(struct _reent_nl *) __asm__("func_00113AE0");

/* fflush, newlib's text: a NULL fp flushes every FILE through _fwalk
   (func_00114438); CHECK_INIT keeps its `do { } while (0)`. */
int libc_fflush(FILE_nl *fp) {
    register unsigned char *p;
    register int n, t;

    if (fp == 0)
        return _fwalk_i(_impure_ptr_r, libc_fflush);

    do {
        if ((fp)->_data == 0)
            (fp)->_data = _impure_ptr_r;
        if (!(fp)->_data->__sdidinit)
            __sinit_r((fp)->_data);
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

    while (n > 0)
    {
        t = (*fp->_write) (fp->_cookie, (char *) p, n);
        if (t <= 0)
        {
            fp->_flags |= 0x0040;   /* __SERR */
            return -1;              /* EOF */
        }
        p += t;
        n -= t;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00113A6C);
