#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "core/mpeg/motion_comp.h"

/*
 * core_text object 0x12A2F0-0x12AC80. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 */

extern void _Error(void *, void *);

extern void dispatch_mpeg_callback();

/* _doCSC (libmpeg). Stack-size probe #2: a 32-byte local (two 16-byte
   compiler stack slots) instead of one, since a single int/4-int array
   both land in one slot here (frame still 0x10 short of retail's 0x70,
   see c1-c4). */
void do_csc(void *arg0, unsigned int arg1, int arg2) {
    char *p = (char *)arg0;
    int r;
    int event[8];

    while (*(volatile int *)0x10002010 < 0) {
    }
    r = disable_interrupts();
    *(volatile unsigned int *)0x1000B010 = arg1 & 0x0FFFFFFF;
    *(volatile unsigned int *)0x1000B020 = (unsigned int)arg2 << 6;
    *(volatile unsigned int *)0x1000B000 = 0x100;
    if (r != 0) {
        enable_interrupts();
    }
    send_ipu_command(arg0, arg2 | 0x70000000);
    event[0] = 4;
    dispatch_mpeg_callback(*(int *)(p + 0x858), event);
    while (((*(volatile unsigned int *)0x1000B000 >> 8) & 1) != 0) {
    }
    while (*(volatile int *)0x10002010 < 0) {
    }
}

ASM_FUNC("asm/handwritten/core_text", func_0012A418);

extern int add_dmac_handler2_r(int, void *, int, void *) __asm__("func_00118AC0");
extern void enable_dmac_atomic(int);

extern char csc_handler_error_msg[];

typedef struct {
    volatile int done; /* 0x00: chunks finished (callback counts) */
    int err;           /* 0x04 */
    int size;          /* 0x08 */
    int next;          /* 0x0C */
    int total;         /* 0x10 */
} CscRec;

/* libmpeg chunked DMA copy: arm the channel-3 callback with a progress record, start the transfer, wait for all
   chunks. */
void do_csc2(void *arg0, unsigned int arg1, int arg2) {
    char *p = (char *)arg0;
    int event[8];
    CscRec rec;
    CscRec *rp;
    int h;
    int r;

    rec.total = arg2 / 0x3FF + 1;
    rec.size = arg2;
    rec.next = (arg1 + 0xFFC00) & 0x0FFFFFFF;
    rec.err = 0;
    rec.done = 0;
    rp = &rec;
    while (*(volatile int *)0x10002010 < 0) {
    }
    h = add_dmac_handler2_r(3, (void *)do_csc2_dma_handler, 0, rp);
    *(volatile unsigned int *)0x1000E010 = 8;
    enable_dmac_atomic(3);
    r = disable_interrupts();
    {
        volatile unsigned int *madr = (volatile unsigned int *)0x1000B010;
        volatile unsigned int *qwc = (volatile unsigned int *)0x1000B020;
        volatile unsigned int *chcr = (volatile unsigned int *)0x1000B000;
        *madr = arg1 & 0x0FFFFFFF;
        *qwc = 0xFFC0;
        *chcr = 0x100;
    }
    if (r != 0) {
        enable_interrupts();
    }
    *(volatile unsigned int *)0x10002000 = 0x700003FF;
    event[0] = 4;
    dispatch_mpeg_callback(*(void **)(p + 0x858), event);
    while (rec.done < rec.total) {
    }
    if (rec.err != 0) {
        _Error(p, csc_handler_error_msg);
    }
    while (*(volatile int *)0x10002010 < 0) {
    }
    disable_dmac_atomic(3);
    remove_dmac_handler(3, h);
}

ASM_FUNC("asm/handwritten/core_text", func_0012A718);

extern void *dispatch_mpeg_callback_result(void *, int *) __asm__("func_0012BC78");
/* Transfer the MPEG input through the IPU/SPR DMA channel, splitting large blocks. */

extern void enable_dmac_atomic(int);
extern void do_csc2(void *, unsigned int, int);
extern void csc_store_ref_image_dma_handler();

void _csc_storeRefImage(void *arg0, int *arg1) {
    char *p = (char *)arg0;
    int ev[8];
    int loc[4];
    int n, small, r, addr;
    unsigned int mask;
    void *h;

    ev[0] = 2;
    n = arg1[3] * arg1[4];
    dispatch_mpeg_callback_result(*(void **)(p + 0x858), ev);
    if ((*(volatile unsigned int *)0x10002010 & 0x4000) != 0) {
        *(volatile unsigned int *)0x10002010 = 0x40000000;
    }
    small = n < 0x400;
    while (*(volatile int *)0x10002010 < 0) {
    }
    send_ipu_command(arg0, 0);
    addr = arg1[0];
    while (*(volatile int *)0x10002010 < 0) {
    }
    mask = 0x0FFFFFFF;
    loc[1] = addr & mask;
    loc[0] = n * 0x18;
    if ((unsigned int)loc[0] > 0xFFFF) {
        h = add_dmac_handler2(4, (void *)csc_store_ref_image_dma_handler, 0, &loc[0]);
        *(volatile unsigned int *)0x1000E010 = 0x10;
        enable_dmac_atomic(4);
        r = disable_interrupts();
        *(volatile unsigned int *)0x1000B410 = loc[1];
        *(volatile unsigned int *)0x1000B420 = 0xFFFF;
        *(volatile unsigned int *)0x1000B400 = 0x101;
        if (r != 0) {
            enable_interrupts();
        }
        loc[1] = (loc[1] + 0xFFFF0) & mask;
        loc[0] = loc[0] - 0xFFFF;
        if (small) {
            do_csc(arg0, *(int *)(p + 0xD8), n);
        } else {
            do_csc2(arg0, *(int *)(p + 0xD8), n);
        }
        disable_dmac_atomic(4);
        remove_dmac_handler(4, (int)h);
    } else {
        r = disable_interrupts();
        *(volatile unsigned int *)0x1000B410 = arg1[0] & mask;
        *(volatile unsigned int *)0x1000B420 = loc[0];
        *(volatile unsigned int *)0x1000B400 = 0x101;
        if (r != 0) {
            enable_interrupts();
        }
        loc[0] = 0;
        if (small) {
            do_csc(arg0, *(int *)(p + 0xD8), n);
        } else {
            do_csc2(arg0, *(int *)(p + 0xD8), n);
        }
    }
    ev[0] = 3;
    dispatch_mpeg_callback_result(*(void **)(p + 0x858), ev);
}

extern long sysbit_flush(void *, int);

/* _sysbitInit: points the bit stream at arg0 at the arg3-byte ring buffer
   at arg2, reading from arg1, and resets it. The store order is
   load-bearing (LEVERS.md lever 3): arg1's +0x8 store has to come before
   its +0xC store, after the other fields in retail's order. */
void sysbit_init(void *arg0, int arg1, int arg2, int arg3) {
    char *p = (char *)arg0;
    *(int *)(p + 0x28) = arg3;
    *(int *)(p + 0x8) = arg1;
    *(long *)(p + 0x0) = 0;
    *(int *)(p + 0x10) = 0;
    *(long *)(p + 0x18) = 0;
    *(int *)(p + 0x20) = arg2;
    *(int *)(p + 0x24) = arg2 + arg3;
    *(int *)(p + 0xC) = arg1;
    sysbit_flush(arg0, 0);
}

/* Peek n high bits of the 64-bit accumulator, without consuming them.
 * As a C shift, this requires 1 <= n <= 64. GNU 2.10-ee assembles the
 * final int conversion directly into the return delay slot. */
int sysbit_show(void *arg0, int n) {
    unsigned long acc = *(unsigned long *)arg0;
    return (int)(acc >> (64 - n));
}

/* The bitstream reader's state (sysbit_init sets it up). */
typedef struct {
    unsigned long acc; /* 0x00: the next bits, top-aligned */
    unsigned char *start0; /* 0x08 */
    unsigned char *cur; /* 0x0C: next byte to load */
    unsigned int bits; /* 0x10: valid bits in acc */
    long total; /* 0x18: bits consumed so far */
    unsigned char *start; /* 0x20: the ring buffer */
    unsigned char *end; /* 0x24 */
    int len; /* 0x28 */
} BitStream;

/* Consume n bits, then refill the accumulator a byte at a time from the
   ring buffer until more than 56 bits are buffered; returns the new
   total (mpeg2decode's Flush_Buffer shape). The `c` local and the
   returned total are load-bearing: they bring the function to the 40
   insns that put GCSE's hash buckets, and so PRE's registers, in
   retail's order. */
long sysbit_flush(void *arg0, int n) {
    BitStream *p = arg0;
    p->acc <<= n;
    p->bits -= n;
    if (p->bits <= 56) {
        do {
            unsigned int c = *p->cur++;
            p->acc |= (unsigned long)c << (56 - p->bits);
            if (p->cur >= p->end)
                p->cur = p->start;
            p->bits += 8;
        } while (p->bits <= 56);
    }
    return p->total += n;
}

/* Get n bits: peek them (sysbit_show), then consume them
   (sysbit_flush). */
int sysbit_get(void *arg0, int arg1) {
    int r = sysbit_show(arg0, arg1);
    sysbit_flush(arg0, arg1);
    return r;
}

/* Get one bit, as sysbit_get. */
int sysbit_marker(void *arg0) {
    int r;
    r = sysbit_show(arg0, 1);
    sysbit_flush(arg0, 1);
    return r;
}

/* _sysbitJump (libmpeg bit.c): skip n bytes, then refill through
   _sysbitFlush. Clearing acc and bits before the total update is the
   one statement order that matches. */
void sysbit_jump(void *arg0, int n) {
    BitStream *p = arg0;
    p->acc = 0;
    p->bits = 0;
    p->total += n * 8;
    p->cur = p->start0 + (p->total >> 3);
    if (p->cur >= p->end)
        p->cur -= p->len;
    sysbit_flush(p, 0);
}

unsigned int sysbit_ptr(char *arg0, int arg1) {
    unsigned int v = *(int *)(arg0 + 0x8) + (arg1 >> 3);
    if (v >= *(unsigned int *)(arg0 + 0x24)) {
        v -= *(int *)(arg0 + 0x28);
    }
    return v;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012AC7C);
