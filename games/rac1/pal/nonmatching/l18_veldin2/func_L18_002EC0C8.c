/* NON_MATCHING func_L18_002EC0C8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 51/452 (88.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1 (10 runs)
 *   Function (level 18 Veldin 2): if gp flag D_L18_0016207C is set, clears it, reinitialises a camera-like path: c
 *   gp offsets: -0x4C84=0016207C, -0x4BF0=00162110, -0x4B94=0016216C, -0x4C1C=001620E4, -0x4C80=00162080, -0x4C68/
 *   Best: p7.c with TRY_CFLAGS=-mno-split-addresses (436/452). Fixes that mattered: gp globals as `short` read via
 *   Remaining: retail splits the array addresses (4 lui, then 4 addiu) yet stores the 8 shorts and D_L18_00162168 
 */
#include "common.h"

extern int func_001F9850(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);

extern short D_L18_0016207C;
extern short D_L18_00162110;
extern short D_L18_0016216C;
extern short D_L18_001620E4;
extern short D_L18_00162080;
extern short D_L18_00162098;
extern short D_L18_001620A0;
extern short D_L18_001620B4;
typedef struct { short v; short p; } S4;
extern int D_L18_00162168;
extern short D_L18_0016214E[4] MACRO_ADDR;
extern S4 D_L18_0016215C MACRO_ADDR;
extern S4 D_L18_0016215E MACRO_ADDR;
extern S4 D_L18_00162166 MACRO_ADDR;
extern S4 D_L18_00162158 MACRO_ADDR;
extern S4 D_L18_00162160 MACRO_ADDR;
extern S4 D_L18_0016215A MACRO_ADDR;
extern S4 D_L18_00162162 MACRO_ADDR;
extern S4 D_L18_00162164 MACRO_ADDR;
extern char D_L18_001D9FD0[];
extern char D_L18_001D9FC0[];
extern char D_L18_001DA250[];
extern char D_L18_001DA2A0[];
extern char D_L18_001DA3E0[];

void func_L18_002EC0C8(char *arg) {
    char *p;
    char *q;
    char *r;
    char *s;
    char *t;
    int i;
    char a[16];
    char b[16];
    char *data = *(char **)(arg + 0x78);

    if (*(int *)&D_L18_0016207C != 0) {
        char *m = arg + 0x10;
        *(int *)&D_L18_0016207C = 0;
        *(int *)&D_L18_0016216C = 0;
        D_L18_00162168 = func_001F9850(*(int *)&D_L18_00162110);
        *(int *)&D_L18_001620E4 = 0x7F2020;
        func_001F9BF0(a, data + 0x10, m);
        func_L00_001FF4B0(b, a, *(float *)&D_L18_00162080 / 20.0f);
        qcopy(D_L18_001D9FD0, m);
        {
            char *bq = D_L18_001D9FC0, *br = D_L18_001DA250, *bs = D_L18_001DA2A0;
            char *bt = D_L18_001DA3E0, *bp = D_L18_001D9FD0;
            s = bs + 0x10;
            t = bt + 4;
            r = br + 4;
            q = bq + 0x10;
            p = bp + 0x10;
        }
        for (i = 0x12; i >= 0; i--) {
            func_001F9BD8(p, q, b);
            q += 0x10;
            p += 0x10;
            *(int *)r = 0;
            func_001F9BC0(s);
            s += 0x10;
            *(int *)t = 0;
            r += 4;
            t += 4;
        }
        {
            short *h = D_L18_0016214E;
            for (i = 3; i >= 0; i--) {
                *h = -1;
                h--;
            }
        }
        D_L18_0016215C.v = 8;
        D_L18_0016215E.v = 4;
        D_L18_00162166.v = 2;
        D_L18_00162158.v = 8;
        D_L18_00162160.v = 4;
        D_L18_0016215A.v = 4;
        D_L18_00162162.v = 2;
        D_L18_00162164.v = 4;
        *(int *)&D_L18_00162098 = 0;
        *(int *)&D_L18_001620A0 = 0;
        *(int *)&D_L18_001620B4 = 0;
    }
}
