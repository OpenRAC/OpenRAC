/* NON_MATCHING func_L00_002D19E8 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: BYTES 56/1148 (95.1% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   CrateDropBolts: crate break. Copies a 28-byte rodata table to a local (T28 struct assign), then either runs th
 *   Best candidate: build-sn/try/func_L00_002D19E8/base.c (BYTES 56/1148, same size, all mnemonics aligned except 
 *   Differences: (1) retail loads 1.0f (lui/mtc1 f23) after the jal func_L00_00258250 and sets i=0 in the `b` dela
 */
#include "common.h"
typedef struct { int w[7]; } T28;
typedef struct { int kind; float a; float b; int n; } Loc;
extern void func_001F9BC0(void *);
extern int func_L00_00261478(int unused, char *m, char *a, void *b, char *c, void *d);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9B88(float);
extern void func_L00_002584A8(void *, int, int);
extern int func_L00_00258250(void *, void *);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_0025A5D8(float *r0, float *r1, float a, float b, float c);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00262BC0(int, void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern char *func_L00_002C6608(void *, int, int, int);
extern T28 D_L00_001EA1B0;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int D_L00_0015F678 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L00_001B0830[];
extern short D_L00_00161998;
extern short D_L00_0016199C;
extern short D_L00_001619A0;
extern short D_0015EE70_s __asm__("D_0015EE70");

/* crate break: scatter bolts from the crate, or finish a pickup timer */
void func_L00_002D19E8(char *m) {
    char *d = *(char **)(m + 0x78);
    int a = *(short *)(m + 0xA6);
    T28 tb = D_L00_001EA1B0;
    float v20[4];
    float v30[4];
    float v40[4];
    Loc L;
    char *j;
    int i;
    L.kind = *(short *)(d + 0xC6) == 0;
    if (a == 0x1F5) L.kind = 0;
    if (L.kind) {
        func_001F9BC0(v20);
        if (*(int *)(d + 0xF0)) {
            if (m) *(short *)(m + 0x36) = 0x7F80;
            func_L00_00261478((int)m, (char *)*(int *)(d + 0xF0), m + 0x10, m + 0x40, (char *)v30, v40);
            func_001F9BF0(v20, v30, m + 0x10);
        }
        if (D_0015EE84_m == 0xF) {
            if (func_001F9B88(*(float *)(m + 0x10) - 166.0f) < 6.0f) {
                if (func_001F9B88(*(float *)(m + 0x14) - 193.0f) < 6.0f) {
                    int t = D_L00_0015F678;
                    D_L00_0015F678 = 0;
                    func_L00_002584A8(m, 0x100, -1);
                    D_L00_0015F678 = t;
                    return;
                }
            }
        }
        func_L00_002584A8(m, 0x100, *(int *)(d + 0xC0));
        return;
    }
    if (a != 0x1F5) {
        int r;
        switch (*(unsigned char *)(d + 0xCB)) {
        case 1: r = 0xA; break;
        case 6: r = 0xB; break;
        case 8: r = 0xD; break;
        case 3: r = 0xF; break;
        case 2: r = 0x10; break;
        case 5: r = 0x11; break;
        case 10: r = 0x13; break;
        case 4: r = 0x14; break;
        case 11: r = 0x17; break;
        case 7: r = 0x18; break;
        case 9: r = 0x19; break;
        case 0:
        default: r = -1; break;
        }
        L.kind = r;
        for (L.n = func_L00_00258250(m, &L), i = 0; i < L.n && (j = func_L00_002C6608(m + 0x10, L.kind, -1, 0)) != 0; i++) {
            char *p = *(char **)(j + 0x78);
            float f21, f20, v, s;
            *(short *)(p + 4) = *(unsigned short *)(d + 0xC4);
            f21 = func_00214158();
            f20 = func_002140F8(0.0f, *(float *)&D_L00_00161998) * D_0015EE6C;
            *(float *)(p + 0x10) = func_001F9F90(f21) * f20;
            *(float *)(p + 0x14) = func_001F9FA8(f21) * f20;
            *(float *)(p + 0x18) = 0.0f;
            v = func_002140F8(*(float *)&D_L00_0016199C, *(float *)&D_L00_001619A0) * D_0015EE6C;
            *(float *)(p + 0x18) = v;
            if (*(int *)(d + 0xC0) >= 0) {
                float a0 = *(float *)&D_0015EE70_s * 10.8f * -0.5f;
                char *t = D_L00_001B0830[*(int *)(d + 0xC0)];
                int r2 = func_L00_0025A5D8(&L.a, &L.b, a0, v - a0, *(float *)(j + 0x18) - *(float *)(t + 0x18));
                s = 120.0f;
                if ((float)r2 >= 1.0f) {
                    if (L.a > 0.0f && L.a < s) s = L.a;
                }
                func_001F9C30(v20, p + 0x10, s);
                func_001F9BD8(v20, v20, m + 0x10);
                func_L00_00262BC0(*(int *)(d + 0xC0), m + 0x10, v20, v30);
                func_001F9BF0(v20, v30, j + 0x10);
                *(int *)(v20 + 2) = 0;
                func_L00_001FF4B0(v40, v20, 0.25f);
                func_001F9BF0(v20, v20, v40);
                func_001F9C30(v20, v20, 1.0f / s);
                *(float *)(p + 0x10) = v20[0];
                *(float *)(p + 0x14) = v20[1];
            }
        }
    }
    if (*(short *)(m + 0xB4)) *(short *)(m + 0xB4) = 0;
    func_L00_002584A8(m, 0x100, -1);
}
