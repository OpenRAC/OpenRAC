/* NON_MATCHING func_L02_002FCA80 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: BYTES 18/316 (94.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tests whether a target entry's condition holds (g==k shortcut; class 6 compares global+0x2FC; type 1 distance 
 *   Best: p12.c (18 bytes differ). Body and branches match (arms 1-3 share the `if (r) return 1;` via `goto four`/
 *   Left: prologue schedule only. Retail loads D_L02_0016755C (lui/lw) first, then lh, then D_L02_0015F050; ours l
 */
#include "common.h"

extern float func_001F9D10(void *, void *);
extern int func_00215570(void *, int);
extern int func_L00_00260AB0(void *, int);
extern int func_L00_00260B68(float *, int);
extern int func_L00_0025A778(void *, void *, int);
extern char *D_L02_0016755C;
extern char *D_L02_0015F050;
extern char *D_L02_001B0DB0[];
extern char D_0013F450[];

/* Tests whether a target entry's condition currently holds. */
int func_L02_002FCA80(char *a) {
    char *g = D_L02_0016755C;
    int idx = *(short *)(a + 0x84);
    char *tab = D_L02_0015F050;
    char *e = *(char **)(tab + (idx << 5) + 0x1C);
    char *k = *(char **)(e + 0x48);
    int r;
    if (g == 0 || g != k) {
        if (*(short *)(e + 0x3C) == 6) {
            return *(char **)(D_0013F450 + 0x2FC) == k;
        }
        if ((unsigned char)e[0x22] == 1) {
            return func_001F9D10(D_0013F450 + 0x80, k + 0x10) < *(float *)(e + 0x24);
        }
        if (*(int *)(e + 0xC) >= 0) {
            r = func_00215570(D_0013F450 + 0x80, *(int *)(e + 0xC));
        } else if (*(int *)(e + 0x10) >= 0) {
            r = func_L00_00260AB0(D_0013F450 + 0x80, *(int *)(e + 0x10));
        } else if (*(int *)(e + 8) >= 0) {
            r = func_L00_00260B68((float *)(D_0013F450 + 0x80), *(int *)(e + 8));
        } else {
            goto four;
        }
        if (r) return 1;
        goto zero;
    four:
        if (*(int *)(e + 0x14) >= 0) {
            char *t = D_L02_001B0DB0[*(int *)(e + 0x14)];
            if (func_L00_0025A778(D_0013F450 + 0x80, t + 0x10, *(int *)t)) return 1;
        }
    zero:
        return 0;
    }
    return 1;
}
