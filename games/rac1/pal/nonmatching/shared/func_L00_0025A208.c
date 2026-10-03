/* NON_MATCHING func_L00_0025A208 -- src/overlays/shared/mobyutil_00258BC8.c
 * Best so far: SIZE ours 228 / retail 232, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0025A208: looks up table entry idx (D_L00_001ABBC0, bound D_L00_001600B4), caches it in D_L00_001601C
 *   Best shape is p5.c (212 vs 232 bytes): all globals plain MACRO_ADDR, one shared call block. Difference: retail
 *   Would unblock: a source form in which the a2!=0 read of D_L00_001601D4 is not CSE'd with the read for lb (p3 w
 */
extern int D_L00_001600B4 MACRO_ADDR;
extern int D_L00_001ABBC0[];
extern int D_L00_001601CC MACRO_ADDR;
extern int D_L00_001601D4 MACRO_ADDR;
extern short D_L00_001601D0 MACRO_ADDR;
extern int D_L00_00160098 MACRO_ADDR;
extern int func_L00_0025A2F0(int *, int, int, int);

/* look up table entry idx, cache it, and maybe pass it on */
int func_L00_0025A208(int *out, int idx, int a2, int a3) {
    int *p;
    int neg;
    if (idx < 0 || D_L00_001600B4 < idx) {
        *out = 0;
        return -1;
    }
    *out = 0;
    D_L00_001601D4 = 0;
    p = (int *)D_L00_001ABBC0[idx];
    D_L00_001601CC = (int)p;
    if (p == 0) {
        return -1;
    }
    D_L00_001601D0 = *(unsigned short *)p & 0x7FFF;
    D_L00_001601D4 = D_L00_00160098 + (D_L00_001601D0 << 8);
    *out = D_L00_001601D4;
    neg = (unsigned)(int)*(char *)(D_L00_001601D4 + 0x20) >> 31;
    if (a2 != 0) {
        if (a3 != 0 && !neg) {
            return func_L00_0025A2F0(out, D_L00_001601D4, a2, a3);
        }
        return 0;
    }
    if (a3 != 0 || neg) {
        return func_L00_0025A2F0(out, D_L00_001601D4, a2, a3);
    }
    return 0;
}
