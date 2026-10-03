#include "common.h"
#include "structs.h"

/*
 * core_text object 0x116FA0-0x1188C8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib's vfprintf.c (the SDK's libc.a member vfprintf.o): __sprint,
 * __sbprintf, vfprintf, _vfprintf_r, cvt and exponent. Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like libc.a; newlib's own text for
 * each function compiles to retail under it.
 */

/* Declarations in scope here before the split. */
extern long func_00116F68(int arg0, int arg1, int arg2);
extern int D_0015ED10;
extern void *D_0012F86C NOT_SDA;
extern int func_001162B8(void *arg0, void *arg1, void *arg2);
extern int func_00116320(void *arg0, void *arg1, void *arg2);
extern long func_001163A0(void *arg0, void *arg1, void *arg2);
extern void func_00116408(void *arg0);
extern void func_00113968(void);
extern void func_00114438(void *, void *);
extern char D_00152470[];
extern int func_00119088();
extern int func_00119110();
extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");
extern int func_00112468(int *errOut, int arg1);

extern int func_00114060(int, void *);

/* newlib's stdio types (sys/reent.h, 1999-2000), under local names. */
struct __sbuf_16FA0 {
    unsigned char *_base;
    int _size;
};
struct _reent_16FA0 {
    char _pad[0x38];
    int __sdidinit;             /* 0x38 */
};
typedef struct {
    unsigned char *_p;          /* 0x00 */
    int _r;
    int _w;
    short _flags;               /* 0x0C */
    short _file;
    struct __sbuf_16FA0 _bf;    /* 0x10 */
    int _lbfsize;
    void *_cookie;              /* 0x1C */
    int (*_read)(void *, char *, int);
    int (*_write)(void *, const char *, int);
    long (*_seek)(void *, long, int);
    int (*_close)(void *);
    struct __sbuf_16FA0 _ub;    /* 0x30 */
    unsigned char *_up;
    int _ur;
    unsigned char _ubuf[3];
    unsigned char _nbuf[1];
    struct __sbuf_16FA0 _lb;    /* 0x44 */
    int _blksize;
    int _offset;
    struct _reent_16FA0 *_data; /* 0x54 */
} FILE_16FA0;
struct __siov_16FA0 {
    void *iov_base;
    int iov_len;
};
struct __suio_16FA0 {
    struct __siov_16FA0 *uio_iov;
    int uio_iovcnt;
    int uio_resid;
};

/* __sprint, newlib's text: flush the uio through __sfvwrite
   (func_00114060) if it holds anything. */
int func_00116FA0(FILE_16FA0 *fp, struct __suio_16FA0 *uio) {
    register int err;

    if (uio->uio_resid == 0) {
        uio->uio_iovcnt = 0;
        return (0);
    }
    err = func_00114060((int)fp, uio);
    uio->uio_resid = 0;
    uio->uio_iovcnt = 0;
    return (err);
}

/* vfprintf and fflush return int; the declarations in scope above say
   void. */
extern int func_001170A0_f(FILE_16FA0 *, const char *, void *) __asm__("func_001170A0");
extern int func_00113968_f(FILE_16FA0 *) __asm__("func_00113968");

/* __sbprintf, newlib's text: printf to an unbuffered file through a
   temporary 1 KB buffer (BUFSIZ), then copy the error status back. */
int func_00116FE8(FILE_16FA0 *fp, const char *fmt, void *ap) {
    int ret;
    FILE_16FA0 fake;
    unsigned char buf[1024];

    /* copy the important variables */
    fake._data = fp->_data;
    fake._flags = fp->_flags & ~0x0002; /* __SNBF */
    fake._file = fp->_file;
    fake._cookie = fp->_cookie;
    fake._write = fp->_write;

    /* set up the buffer */
    fake._bf._base = fake._p = buf;
    fake._bf._size = fake._w = sizeof(buf);
    fake._lbfsize = 0;  /* not actually used, but Just In Case */

    /* do the work, then copy any error status */
    ret = func_001170A0_f(&fake, fmt, ap);
    if (ret >= 0 && func_00113968_f(&fake))
        ret = -1;       /* EOF */
    if (fake._flags & 0x0040)   /* __SERR */
        fp->_flags |= 0x0040;
    return (ret);
}

extern void func_00113AE0(void *);
extern void func_00117118(void *, void *, int, int);
extern int func_00117118_i(struct _reent_16FA0 *, FILE_16FA0 *, const char *, void *)
    __asm__("func_00117118");

/*
 * vfprintf, newlib's text: CHECK_INIT (fault in the reent from
 * _impure_ptr, D_0012F86C, and run __sinit, func_00113AE0, if it has not
 * been), then return _vfprintf_r's result.
 *
 * CHECK_INIT has to stay the macro's `do { ... } while (0)`: its loop
 * notes are what leave retail's `nop` after the _data store and give the
 * argument registers retail's $17/$18 roles. Without the wrapper 2.9-ee
 * is one instruction short (116/120). (Under 2.95.3 the old field-access
 * spelling was the right size but 20/120, with $17/$18 swapped.) It
 * returns the callee's value, so 2.9-ee keeps the call.
 */
int func_001170A0(FILE_16FA0 *fp, const char *fmt0, void *ap) {
    do {
        if ((fp)->_data == 0)
            (fp)->_data = (struct _reent_16FA0 *)D_0012F86C;
        if (!(fp)->_data->__sdidinit)
            func_00113AE0((fp)->_data);
    } while (0);
    return func_00117118_i(fp->_data, fp, fmt0, ap);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00117118);

/* newlib's `union double_union` over the little-endian word order:
   word0 (the sign/exponent word) is i[1]. */
union double_union_16FA0 {
    double d;
    unsigned int i[2];
};
extern char *func_001126D8_d(void *, double, int, int, int *, int *, char **)
    __asm__("func_001126D8");

/* cvt, newlib's text: convert a double for the %e/%f/%g conversions
   through _dtoa_r (func_001126D8), padding trailing zeros unless %g
   without the ALT flag. */
char *func_00118630(void *data, double value, int ndigits, int flags,
                    char *sign, int *decpt, int ch, int *length) {
    int mode, dsgn;
    char *digits, *bp, *rve;
    union double_union_16FA0 tmp;

    if (ch == 'f') {
        mode = 3;               /* ndigits after the decimal point */
    } else {
        /* To obtain ndigits after the decimal point for the 'e'
         * and 'E' formats, round to ndigits + 1 significant
         * figures.
         */
        if (ch == 'e' || ch == 'E') {
            ndigits++;
        }
        mode = 2;               /* ndigits significant digits */
    }

    tmp.d = value;
    if (tmp.i[1] & 0x80000000) { /* this will check for < 0 and -0.0 */
        value = -value;
        *sign = '-';
    } else
        *sign = '\000';
    digits = func_001126D8_d(data, value, mode, ndigits, decpt, &dsgn, &rve);
    if ((ch != 'g' && ch != 'G') || flags & 0x001) { /* ALT */
        bp = digits + ndigits;
        if (ch == 'f') {
            if (*digits == '0' && value)
                *decpt = -ndigits + 1;
            bp += *decpt;
        }
        if (value == 0) /* kludge for __dtoa irregularity */
            rve = bp;
        while (rve < bp)
            *rve++ = '0';
    }
    *length = rve - digits;
    return (digits);
}

/* exponent, newlib's text, with its MAXEXP of 308: the 0x140 frame and
   `t = expbuf + 308` are retail's. */
int func_001187E0(char *p0, int exp, int fmtch) {
    register char *p, *t;
    char expbuf[308];

    p = p0;
    *p++ = fmtch;
    if (exp < 0) {
        exp = -exp;
        *p++ = '-';
    }
    else
        *p++ = '+';
    t = expbuf + 308;
    if (exp > 9) {
        do {
            *--t = (exp % 10) + '0';
        } while ((exp /= 10) > 9);
        *--t = exp + '0';
        for (; t < expbuf + 308; *p++ = *t++);
    }
    else {
        *p++ = '0';
        *p++ = exp + '0';
    }
    return (p - p0);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001188C0);
