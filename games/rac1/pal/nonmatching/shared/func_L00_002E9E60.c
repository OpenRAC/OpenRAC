/* NON_MATCHING func_L00_002E9E60 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 512 / retail 516, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E9E60 (camera pad axes a,b from pad; counts idle time): p4.c compiles to what reads as retail's co
 *   Blocker is tools/ps2eeas_nops.py: "object has 3 mtc1 uses; source has 2 -- refusing to guess". The shared tail
 *   Lead: let move_sites keep `pending` across a label line reached by fallthrough, then rerun p4.c. Idioms: `*(in
 *   q29 v02: p6.c (p4 plus Lombyte credit, D_0013A5E0 declared unsigned char to match the file's earlier declarati
 */
#include "common.h"
extern unsigned char D_0013A5E0[];
extern char D_0014171B[];
extern int D_0015EED0[] MACRO_ADDR;
extern short D_0015EFA4;
extern int D_0015EFA4_m __asm__("D_0015EFA4") MACRO_ADDR;
extern short D_0015EE84;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern short D_L00_0015F044;
extern short D_L00_00161E5C;
extern short D_L00_00161E60;
extern int func_001F9850(int);
/* Computes two camera input axes from the pad and counts idle time. Adapted from Lombyte (MIT) for PAL: overlays/shared/gameplay_vendor_002e89b0.c, FUN_L00_002e89b0. */
void func_L00_002E9E60(int m, float *a, float *b) {
    char *p = *(char **)((char *)m + 0x70) + 0x1A8;
    char *r;
    int q;
    if (*(int *)(p + 0x10) & 1) {
        *a = 0.0f;
    } else {
        *a = *(float *)(D_0013A5E0 + 0x2560);
        if (D_0015EED0[4] == 0)
            *a = -*a;
    }
    if (*a == 0.0f) {
        float f = *(float *)(p + 0x1C);
        *(int *)&D_L00_00161E5C = 0;
        *(int *)&D_L00_00161E60 = 0;
        if (f != 0.0f)
            *a = f;
    } else {
        *(float *)(p + 0x14) = *(float *)&D_L00_0015F044;
        r = D_0014171B + 0x22D;
        if (*(unsigned short *)(r + 0x40) < 3) {
            *(int *)&D_L00_00161E5C = *(int *)&D_L00_00161E5C + 1;
            if (*(int *)&D_L00_00161E60 == 0) {
                if (func_001F9850(0x3C) < *(int *)&D_L00_00161E5C) {
                    if (*(unsigned short *)(r + 0x40) < 0xFFFF)
                        (*(unsigned short *)(r + 0x40))++;
                    q = func_001F9850(D_0015EFA4_m) / 600;
                    if (*(unsigned short *)(r + 0x42) < q)
                        *(unsigned short *)(r + 0x42) = func_001F9850(D_0015EFA4_m) / 600;
                    *(int *)&D_L00_00161E60 = 1;
                    *(int *)(r + 0x44) = *(int *)(r + 0x44) | (1 << D_0015EE84_m) | 0x80000000;
                }
            }
        }
    }
    if (*(int *)(p + 0x10) & 2) {
        *b = 0.0f;
        *b = *(float *)(p + 0x24) / 0.7f;
    } else {
        float g = *(float *)(D_0013A5E0 + 0x2564);
        *b = -g;
        if (D_0015EED0[3] == 0)
            *b = g;
        if (*b == 0.0f)
            *b = *(float *)(p + 0x24) / 0.7f;
    }
}
