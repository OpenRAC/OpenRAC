#include "common.h"
#include "structs.h"

/*
 * core_text object 0x112468-0x1138A8. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a): closer.o (_close_r) and dtoa.o (quorem,
 * _dtoa_r), back to back. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like libc.a.
 */

/* Declarations in scope here before the split. */
extern long func_00116F68(int arg0, int arg1, int arg2);

extern int D_0015ED10;

int func_00112468(int *errOut, int arg1) {
    int r;
    D_0015ED10 = 0;
    r = func_00119100(arg1);
    if (r == -1 && D_0015ED10 != 0) {
        *errOut = D_0015ED10;
    }
    return r;
}

/* newlib dtoa.c quorem(b,S): one digit of b/S for dtoa's digit-generation
 * loop -- estimate the digit q from the top limbs, subtract q*S from b
 * (trimming b's leading zero limbs), then bump q by one more if b is
 * still >= S (the estimate can undershoot by 1), subtracting S again. */
extern int func_00115CE8(void *, void *); /* cmp */

typedef struct Bigint_1154D0 {
    struct Bigint_1154D0 *next;
    int k, maxwds, sign, wds;
    unsigned int x[1];
} Bigint_1154D0;

#define STOREINC(xc, hi, lo) \
    (((unsigned short *)(xc))[1] = (unsigned short)(hi), \
     ((unsigned short *)(xc))[0] = (unsigned short)(lo), \
     (xc)++)

int func_001124C0(Bigint_1154D0 *b, Bigint_1154D0 *S) {
    int n;
    int borrow, y, z;
    unsigned int carry, q, ys, si, zs;
    unsigned int *bx, *bxe, *sx, *sxe;

    n = S->wds;
    if (b->wds < n)
        return 0;
    sx = S->x;
    sxe = sx + --n;
    bx = b->x;
    bxe = bx + n;
    q = *bxe / (*sxe + 1);
    if (q) {
        borrow = 0;
        carry = 0;
        do {
            si = *sx++;
            ys = (si & 0xffff) * q + carry;
            zs = (si >> 16) * q + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            STOREINC(bx, z, y);
        } while (sx <= sxe);
        if (!*bxe) {
            bx = b->x;
            while (--bxe > bx && !*bxe)
                --n;
            b->wds = n;
        }
    }
    if (func_00115CE8(b, S) >= 0) {
        q++;
        borrow = 0;
        carry = 0;
        bx = b->x;
        sx = S->x;
        do {
            si = *sx++;
            ys = (si & 0xffff) + carry;
            zs = (si >> 16) + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            STOREINC(bx, z, y);
        } while (sx <= sxe);
        bx = b->x;
        bxe = bx + n;
        if (!*bxe) {
            while (--bxe > bx && !*bxe)
                --n;
            b->wds = n;
        }
    }
    return q;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001126D8);

INCLUDE_ASM("asm/nonmatchings/core_text", func_001138A4);
