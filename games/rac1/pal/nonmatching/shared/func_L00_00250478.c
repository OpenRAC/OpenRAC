/* NON_MATCHING func_L00_00250478 -- src/overlays/shared/mobyfunc_0024FD50.c
 * Best so far: BYTES 7/336 (97.9% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L00_00250478 (336 B)
 *   Best: w1.c BYTES 7/336. Only difference: nb*8 gets $t0 and `from` (src + nc*8 + 0x10) gets $a3; retail swaps t
 *   Levers that got here: i*4/j*4 precomputed (pointer-first addu), src+0xA read once into a local,
 *   `from` computed before the 0xA store, `if (nc) { k = nc; do ... while (--k); }` loop.
 *   Tried without effect: nb8 local (x1/x4), from placement (x2/x3), long* index forms (y1/y3), nb<<3,
 *   header store order (z1/z2), flags -fno-force-mem/-fno-regmove/-fno-strength-reduce/-fno-gcse.
 */
#include "common.h"

extern void func_001F99D8(void *, int);
extern void func_L00_001FF040_cp(void *, void *, int) __asm__("func_L00_001FF040");
extern void func_00118D80(int);

/* Builds a frame packet in out from entry (i, j) of the moby's tables. */
void func_L00_00250478(void *a, int i, int j, int o) {
    char *m = (char *)a;
    char *out = (char *)o;
    int io = i * 4;
    int jo = j * 4;
    char *c = *(char **)(m + 0x18);
    char *b = *(char **)(m + 0x14);
    int nc = *(unsigned char *)(c + 8);
    int nb = *(unsigned char *)(b + 8);
    char *src = *(char **)(*(char **)(c + io + 0x48) + jo + 0x1C);
    char *from;
    int len;
    int k;
    unsigned short x;
    unsigned char *idx;
    long *q;
    long *e;
    func_001F99D8(out, ((nb + 3) * 8) & 0xFF0);
    *(short *)(out + 6) = (nb + *(short *)(src + 0xA) + *(short *)(src + 0xE) + 1) >> 1;
    *(short *)(out + 8) = nb * 8;
    from = src + (nc * 8 + 0x10);
    x = *(unsigned short *)(src + 0xA);
    *(short *)(out + 0xA) = x;
    *(short *)(out + 0xC) = (x + nb) * 8;
    *(short *)(out + 0xE) = *(unsigned short *)(src + 0xE);
    len = *(short *)(src + 6) * 16 - nc * 8;
    if (len != 0) {
        func_L00_001FF040_cp(out + (nb * 8 + 0x10), from, len);
    }
    idx = *(unsigned char **)(*(char **)(m + 0x18) + 0x1C) + 4;
    q = (long *)(src + 0x10);
    if (nc != 0) {
        k = nc;
        do {
            ((long *)(out + 0x10))[*idx++] = *q++;
        } while (--k != 0);
    }
    e = (long *)(out + *(short *)(out + 6) * 16);
    if (*e == 0) *e = 1;
    func_00118D80(0);
}
