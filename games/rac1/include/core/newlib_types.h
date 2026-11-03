#ifndef CORE_NEWLIB_TYPES_H
#define CORE_NEWLIB_TYPES_H

#include "common.h"
#include "structs/vector.h"
#include "structs/records.h"

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

typedef struct Bigint_1154D0 {
    struct Bigint_1154D0 *next;
    int k, maxwds, sign, wds;
    unsigned int x[1];
} Bigint_1154D0;

typedef struct MallocChunk {
    unsigned int prev_size;
    unsigned int size;
    struct MallocChunk *fd;
    struct MallocChunk *bk;
} MallocChunk;

#endif /* CORE_NEWLIB_TYPES_H */
