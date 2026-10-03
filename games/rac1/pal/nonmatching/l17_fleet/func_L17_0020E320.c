/* NON_MATCHING func_L17_0020E320 -- src/overlays/l17_fleet/help_00202740.c
 * Best so far: SIZE ours 1200 / retail 1192, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   run14 (p12): SIZE 1176; separate md/p1..p4 vars fixed regs; only 0x11 case differs (retail: bne h,0x28F->skip 
 *   run15 (p13): S11 as inline func(0x82,1);return 1 matches retail S11 shape except retail merges it into S0 bloc
 *   run16 (p14): S11 nested-if instead of &&: same as p12
 *   run17 (p15): all four sites inline (no goto) -> no merge at all, SIZE 1228. Best so far p12 (SIZE 1176, only S
 *   run18 (p16): a0=0x76 hoisted in S11: same as p12. Stop (three changes, same S11 difference).
 *   STOPPED. Best p12.c (SIZE 1176/1192; everything before S11 matches, S12/S3/S0 sharing via goto shared, head vi
 *   What it does: per-frame hit reaction for the tracked object; picks reaction code by camera mode (+0x20A4) and 
 *   Left: in case 0 / mode 0x11, retail has bne h,0x28F -> skip with `b L544` (unconverted, delay slot li 0x82, lw
 */
#include "common.h"
extern char D_0013E633[];
extern char D_0013DE6E[];
extern char D_L17_00178A80[];
extern int D_L17_0015F6A8 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern int D_0015EFA4[2] MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern void func_L00_00207220(void);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L17_0021ED38(int, int);
extern int func_L00_00217570(int, int);
extern void func_L00_00211338(float *v, int each, float s, float z);
extern void func_001F9BD8(void *, void *, void *);

/* Per-frame hit-reaction setup for the tracked object: picks a reaction code and scales the shared camera vectors. */
int func_L17_0020E320(int arg) {
    char *a = (char *)D_0013E633 + 0xE1D;
    char *m;
    char *b;
    char *c;
    char *d;
    char *q;
    char *e;
    char *p;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    int md;
    char *g;
    float v[4];
    float f2;
    int mode = *(int *)(a + 0x208C);
    int s2;
    int a0;
    int idx;
    int h;

    if (mode == 0x14 || mode == 7 || *(int *)(a + 0x2084) == 0x32) {
    fail:
        return 0;
    }
    if (*(int *)(a + 0x1C0) != 0) goto fail;
    if (D_L17_0015F6A8 != 0) goto fail;
    *(int *)(a + 0x2280) = 0;
    m = *(char **)(a + 0x2080);
    idx = *(unsigned char *)(m + 0xA4);
    if (idx == 0xFF) goto fail;
    e = (char *)D_L17_00178A80 + (idx << 6);
    if (*(char **)(e + 0x34) != m) goto fail;
    if (((*(int *)(e + 0x24) ^ 1) & 1) != 0) goto fail;
    *(int *)(D_0013DE6E + 0x222 + D_0015EE84 * 4) += 1;
    D_0015EFA4[1] = D_0015EFA4[1] + 1;
    *(int *)(a + 0x2280) = *(int *)(e + 0x20);
    if (mode == 0xF) {
        *(short *)(a + 0x5BE) = 1;
        return 0;
    }
    if (arg == 0) return 1;
    func_L00_00207220();
    s2 = 0;
    if (*(int *)(e + 0x30) & 1) {
        qcopy(v, e + 0x10);
        if (*(float *)(e + 0x1C) == 5627.925f) s2 = 1;
    } else {
        p = *(char **)(e + 0x20);
        if (p != 0) {
            func_001F9BF0(v, a + 0x80, p + 0x10);
        } else {
            v[0] = func_001F9F90(func_001FA748(*(float *)(a + 0x98), 3.1415927f));
            v[1] = func_001F9FA8(func_001FA748(*(float *)(a + 0x98), 3.1415927f));
            v[2] = 0;
        }
    }
    c = (char *)D_0013E633 + 0xE1D;
    switch (*(unsigned char *)(c + 0x20A4)) {
    case 0:
        p = *(char **)(c + 0x2280);
        if (p != 0) {
            h = *(short *)(p + 0xA6);
            if (h == 0x4EB || h == 0x558) {
                a0 = 0x80;
            shared:
                func_L17_0021ED38(a0, 1);
                return 1;
            }
        }
        b = (char *)D_0013E633 + 0xE1D;
        md = *(int *)(b + 0x208C);
        if (md == 0x16) {
            func_L17_0021ED38(0x6D, 1);
            *(float *)(b + 0x128) = D_0015EE6C * 7.0f;
            return 1;
        }
        if (md == 0x12) {
            a0 = 0x75;
            p1 = *(char **)(b + 0x2280);
            if (p1 != 0) {
                if (*(short *)(p1 + 0xA6) == 0x28F) {
                    a0 = 0x82;
                    goto shared;
                }
            }
            func_L17_0021ED38(a0, 1);
        } else if (md == 0x11) {
            p2 = *(char **)(b + 0x2280);
            if (p2 != 0 && *(short *)(p2 + 0xA6) == 0x28F) {
                func_L17_0021ED38(0x82, 1);
                return 1;
            }
            a0 = 0x76;
            if (D_0015EE84 == 0xF || D_0015EE84 == 0x11) {
                d = (char *)D_0013E633 + 0xE1D;
                p3 = *(char **)(d + 0x2280);
                if (p3 != 0) {
                    h = *(short *)(p3 + 0xA6);
                    if (h == 0x28F || h == 0x7B || h == 0x29D) {
                        func_L00_00217570(0x1C, 0);
                    }
                }
            }
            func_L17_0021ED38(a0, 1);
        } else if (md == 3) {
            a0 = 0x16;
            p4 = *(char **)(b + 0x2280);
            if (p4 != 0) {
                if (*(short *)(p4 + 0xA6) == 0x28F) {
                    a0 = 0x82;
                    goto shared;
                }
            }
            func_L17_0021ED38(a0, 1);
        } else {
            func_L17_0021ED38(0x16, 1);
        }
        {
            float x;
            float y;
            f2 = D_0015EE6C;
            x = f2 * 5.7f;
            y = f2 * 2.4f;
            q = (char *)D_0013E633 + 0xE1D;
            if (*(unsigned char *)(q + 0x12E7) != 0) {
                x = 0.0f;
                y = f2 * 1.7f;
            }
            func_L00_00211338(v, s2, x, y);
        }
        if (s2 != 0) {
            if (*(unsigned char *)(e + 0x28) == 4) v[2] = v[2] * 2.0f;
        }
        break;
    case 1:
        func_L17_0021ED38(0x46, 1);
        func_L00_00211338(v, s2, D_0015EE6C * 5.0f, D_0015EE6C * 3.0f);
        break;
    case 3:
        func_L17_0021ED38(0x56, 1);
        func_L00_00211338(v, s2, D_0015EE6C * 5.0f, D_0015EE6C * 2.4f);
        break;
    }
    g = (char *)D_0013E633 + 0xEFD;
    func_001F9BD8(g, g, v);
    g = g + 0x20;
    func_001F9BD8(g, g, v);
    return 1;
}
