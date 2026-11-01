#include "common.h"
#include "structs.h"
#include <stdarg.h>

     /* DeleteSema */
extern int r1_func_00118CB0(int);

typedef signed int s32_1B4C8;

extern s32_1B4C8 r1_func_00118CB0();

extern int r1_D_0012FDA0;

extern char r1_D_00157E80[];

extern s32 r1_func_00118C90(s32);

extern s32 r1_func_00118CB0(s32);

extern int r1_func_00118C70(void *);

extern s32_1B4C8 r1_func_00118C70();

extern int r1_D_0012FD9C;

extern void func_0011DD48(void);

extern int r1_D_0012FD9C NOT_SDA;

/* rac1-decomp src/core/00119D88.c: func_0011B7F8 */
/* The out-of-range guard comes first and returns 0: 2.9-ee then branches
   to the in-range block on the true side of the sltiu, as retail does
   (bnez). The in-range-first if/else that matched under 2.95.3 is laid
   out the other way round by 2.9-ee. */
void *func_0011D950(unsigned int arg0) {
    void *ret;
    func_0011D868();
    r1_func_00118CB0(r1_D_0012FDA0);
    if (arg0 >= 0x20) {
        r1_func_00118C90(r1_D_0012FDA0);
        return 0;
    }
    ret = &r1_D_00157E80[arg0 * 0x10];
    r1_func_00118C90(r1_D_0012FDA0);
    return ret;
}

extern int r1_func_00118CA0(int);
typedef struct {
    s32 v;
} __attribute__((packed)) Word;
typedef struct {
    Word ret;
    Word func;
    Word dest;
    Word size;
} FsRcvHead;
typedef struct {
    void *dest;
} __attribute__((packed)) FsRcvPtr;
typedef struct {
    Word dest;
    Word size;
} FsRcvBuf;
typedef struct {
    s32 n1;
    s32 n2;
    u8 *d1;
    u8 *d2;
    u8 buf1[0x40];
    u8 buf2[0x40];
} FsRcvRead;
typedef struct {
    u8 b[0x144];
} __attribute__((packed)) FsDirent;
typedef struct {
    u8 b[0x40];
} __attribute__((packed)) FsStat;
extern volatile s32 r1_D_0012FD10[];
extern u32 r1_D_0012FD90[];
extern s32 r1_D_0012FD98[];
extern u8 r1_D_00157600[];
extern void *r1_func_00115248(void *, const void *, u32);


/* rac1-decomp src/core/00119D88.c: func_0011D9C0 */
void func_0011D9C0(s32 *arg) {
    FsRcvHead h;
    FsRcvHead *src;
    FsRcvBuf *bs;
    union {
        FsRcvPtr p;
        FsRcvBuf b;
    } u;
    FsRcvRead *r;
    u8 *pkt;
    u8 *base;
    u8 *d;
    u32 *bank = r1_D_0012FD90;
    s32 idx;
    s32 i;
    s64 ret;
    u32 n;
    u32 addr;

    idx = r1_D_0012FD98[0] != 0 ? arg[3] : 0;
    *bank = idx;
    base = r1_D_00157600;
    addr = (u32)base;
    addr += (u32)idx * 0x440;
    pkt = (u8 *)(addr | 0x20000000);
    src = (FsRcvHead *)pkt;
    h.ret = src->ret;
    h.func = src->func;
    h.dest = src->dest;
    h.size = src->size;
    if (h.ret.v >= 0) {
        r1_func_00115248((void *)h.dest.v, pkt + 0x10, h.size.v);
    }
    switch (h.func.v) {
    case 2:
        r = (FsRcvRead *)(pkt + 0x14);
        if (r->n1 > 0) {
            d = r->d1;
            for (i = 0; i < r->n1; i++) {
                d[i] = r->buf1[i];
            }
        }
        if (r->n2 > 0) {
            d = r->d2;
            for (i = 0; i < r->n2; i++) {
                d[i] = r->buf2[i];
            }
        }
        break;
    case 11:
        u.p = *(FsRcvPtr *)(pkt + 0x14);
        *(FsDirent *)u.p.dest = *(FsDirent *)(pkt + 0x18);
        break;
    case 12:
        u.p = *(FsRcvPtr *)(pkt + 0x14);
        *(FsStat *)u.p.dest = *(FsStat *)(pkt + 0x18);
        break;
    case 23:
    case 25:
    case 26:
        bs = (FsRcvBuf *)(pkt + 0x14);
        u.b.dest = bs->dest;
        u.b.size = bs->size;
        n = u.b.size.v;
        if (n > 0x400) {
            u.b.size.v = 0x400;
            n = 0x400;
        }
        r1_func_00115248((void *)u.b.dest.v, pkt + 0x1C, n);
        break;
    }
    if (h.ret.v < 0) {
        u32 semaphore = 0u - (u32)h.ret.v;
        ret = (s32)semaphore;
        h.ret.v = (s32)semaphore;
        for (i = 0; i < 32; i++) {
            if (((s32 *)r1_D_0012FD10)[i] == ret) {
                r1_D_0012FD10[i] = -1;
                break;
            }
        }
        return;
    }
    r1_func_00118CA0(h.ret.v);
}

/* rac1-decomp src/core/00119D88.c: func_0011BBF0 */
void func_0011DD48(void) {
    int buf[8];
    if (r1_D_0012FD9C == -1) {
        buf[5] = 0;
        buf[2] = 1;
        buf[1] = 1;
        r1_D_0012FD9C = r1_func_00118C70(buf);
    }
}
