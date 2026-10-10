/* NON_MATCHING func_L00_00267290 -- src/overlays/shared/stream_002670E0.c
 * Best so far: BYTES 28/904 (96.9% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stream slot update: picks a slot from the table at G+0x2084/0x22A8, runs the float distance tests, then either
 *   Left: retail sets $a1 (0x754E) first, then the three function addresses, then $a0 (2), $t1 (&D_0015EE98), $t2 
 *   Unblock: an argument order in the source that gives the a1-first move sequence (the call's parameter order is 
 */
extern int D_L00_0015F6A8 MACRO_ADDR;
extern s32 D_L00_0015F6B0 MACRO_ADDR;
extern char D_L00_00179200[];
extern char D_L00_001C43B0[];
extern int D_0015EE98 MACRO_ADDR;
extern char D_0013D605[];
extern unsigned char D_0013D355[];
extern char D_0013A5E0[];
extern char D_0013E633[];
extern short D_L00_001601F0;
extern s32 func_L00_002676E8(void *p, u8 *out_p);
extern float func_001F9D10_1edff8(void *, void *) __asm__("func_001F9D10");
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern void func_00217AE8(void *, void *, int);
extern s32 func_L00_00267618_676E8(void *) __asm__("func_L00_00267618");
extern void func_L00_0029A7D0(int);
extern int func_001FFCB0(int);
extern void func_L00_00299B68(int);
extern int func_L00_002367A8(int, int);
extern int func_001FFB38(int, int, int, int, int, int, int);
extern int func_001F9850(int);
extern void func_L00_00237B20(void);
extern void func_L00_00237B70(void);
extern void func_L00_00237B90(void);
extern void func_L00_0023A658(void);
extern void func_L00_0023A690(void);
extern void func_L00_0023A788(void);

/* Picks a stream slot for q from the HUD table, updates the counters and dispatches the matching callback. */
int func_L00_00267290(char *p, char *q) {
    char *g = D_0013E633 + 0xE1D;
    char *x;
    char *e;
    char *y;
    char *tb;
    unsigned char *tb2;
    int h4;
    float f, f0;
    int r, i, m;
    int *ptr;

    if (*(int *)(g + 0x2084) == 0x1D || *(int *)(g + 0x22A8) == 0) {
        return 0;
    }
    if (*(unsigned char *)(q + 9) == 0) {
        func_L00_002676E8(p, q);
    }
    if (D_L00_0015F6A8 != 0) {
        return 0;
    }
    if (q == 0) {
        return 0;
    }
    if (*(short *)(q + 0x36) == -1) {
        return 0;
    }
    if (*(unsigned char *)(q + 8) == 0xFF) {
        return 0;
    }
    f0 = func_001F9D10_1edff8(p + 0x10, g + 0x80);
    f = *(float *)(q + 0xC);
    f = f + f;
    if (f < f0) {
        return 0;
    }
    if (*(unsigned char *)(q + 8) == 0) {
        f = func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(p + 0x10), *(float *)(g + 0x84) - *(float *)(p + 0x14));
        f = func_001FA850(f, *(float *)(p + 0x48));
        if (*(float *)(q + 0xC) < f) {
            return 0;
        }
        f = func_L00_001FF860(*(float *)(p + 0x10) - *(float *)(g + 0x80), *(float *)(p + 0x14) - *(float *)(g + 0x84));
        f = func_001FA850(f, *(float *)(g + 0x98));
        if (1.5700000524520874f < f) {
            return 0;
        }
    }
    func_00217AE8(p, q, 0);
    x = D_L00_00179200;
    if (D_L00_0015F6B0 < *(int *)(x + 0x10)) {
        return 0;
    }
    if (*(unsigned char *)(q + 8) != 0 || (*(int *)(D_0013A5E0 + 0x2604) & 0x10) != 0) {
        i = func_L00_00267618_676E8(p);
        tb = D_0013D605 + 0xB3;
        tb = tb + 0xC;
        ptr = (int *)(tb + (i << 4));
        if (*ptr == 0) {
            *ptr = 1;
        }
        *(int *)(x + 8) = (int)p;
        *(int *)(x + 0xC) = (int)q;
        e = (char *)*(int *)(q + 0x3C) + *(short *)(q + 0x36) * 0x1C;
        if (*(unsigned short *)(e + 0x10) & 8) {
            if (*(short *)(e + 8) == 1) {
                D_0015EE98 = D_0015EE98 - *(int *)(D_L00_001C43B0 + *(short *)(e + 0xA) * 0x18);
            } else if (*(short *)(e + 8) == 4) {
                tb2 = D_0013D355 + 0x13B;
            tb2[*(short *)(e + 0xA)] = 0;
            }
        }
        if (*(short *)(e + 4) == -1) {
            func_00217AE8(p, q, 1);
            return 1;
        }
        y = (char *)&D_L00_00179200;
        *(int *)(y + 8) = (int)p;
        *(int *)(y + 0xC) = (int)q;
        if (*(unsigned short *)(e + 4) & 0x4000) {
            func_L00_0029A7D0((short)(*(unsigned short *)(e + 4) ^ 0x4000));
            return 1;
        }
        func_001FFCB0(*(int *)&D_L00_001601F0);
        h4 = *(short *)(e + 4);
        *(int *)&D_L00_001601F0 = -1;
        func_L00_00299B68(h4);
        return 1;
    }
    if (*(unsigned char *)(q + 8) == 0 && (*(int *)(D_0013A5E0 + 0x2604) & 0x10) == 0) {
        r = func_L00_002367A8(*(int *)&D_L00_001601F0, 0xA);
        if (r != 0) {
            *(int *)&D_L00_001601F0 = func_001FFB38(0xC, 0, (int)func_L00_00237B20, (int)func_L00_00237B70, (int)func_L00_00237B90, 0, 0);
        }
        {
            char *e2 = (char *)*(int *)(q + 0x3C) + *(short *)(q + 0x36) * 0x1C;
            if (*(short *)(e2 + 8) != 1 && *(short *)(e2 + 8) != 6) {
                return 0;
            }
        }
        m = func_001FFB38(2, 0x754E, (int)func_L00_0023A658, (int)func_L00_0023A690, (int)func_L00_0023A788, (int)&D_0015EE98, 0x98967F);
        func_L00_002367A8(m, func_001F9850(0x3C));
        return 0;
    }
    return 0;
}
