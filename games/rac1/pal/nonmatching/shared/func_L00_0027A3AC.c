/* NON_MATCHING func_L00_0027A3AC -- src/overlays/shared/pause_00277208.c
 * Best so far: SIZE ours 152 / retail 156, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Wall: reads registers it never sets. Tests two object flags (0x30 bit 8 / bit 4 against D_L00_001BA070+0x134/0
 *   hq2/s09: closest is p1.c (152 vs 156 bytes; the flag test `f = (x & 8) != 0` now matches). Retail keeps the gl
 */
extern char D_L00_001BA070[] NOT_SDA;

// Walks a chain of objects through +0x4C, flagging each by its state bits, then stores the last one in the global's child at +0x80.
int func_L00_0027A3AC(char *m, int a1, int a2, int a3) {
    char *g = D_L00_001BA070;
    int n = *(int *)(g + 0x134);
    int f = a2;
    int flag = a3;
    char *o = m;
    if (n) {
        int t = *(int *)(m + 0x30) & 8;
        f = t != 0;
    }
    if (*(int *)(g + 0x138)) {
        if (*(int *)(m + 0x30) & 4) {
            f = 1;
        }
    }
    if (f) {
        flag = 1;
        o = *(char **)(m + 0x4C);
        for (;;) {
            f = 0;
            if (n) {
                int t = *(int *)(o + 0x30) & 8;
                f = t != 0;
            }
            if (*(int *)(g + 0x138)) {
                if (*(int *)(o + 0x30) & 4) {
                    f = 1;
                }
            }
            if (!f) {
                break;
            }
            o = *(char **)(o + 0x4C);
        }
    }
    if (flag) {
        *(char **)(*(char **)(D_L00_001BA070 + 4) + 0x80) = o;
    }
    return 0;
}
