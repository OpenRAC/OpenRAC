/* NON_MATCHING func_L18_002FD058 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: BYTES 4/1384 (99.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1 (match worker)
 *   UpdateMoby_1906 (level 18): 8-state switch on moby[0x20], then common tail (blend/scale vectors, register draw
 *   Best: p13.c, same size 1384, 4 words differ, all in case 0: retail loads 1.0f into $f0 then 0.5f into $f1 (sto
 *   What mattered: crossjump layout needs the case-2 and case-4 transition code duplicated in source (no goto); `m
 */
#include "common.h"

typedef struct { float v[4]; } __attribute__((aligned(16))) QVd058;

extern int func_002140B0(int);
extern float func_00214158(void);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214358(void *, int, float);
extern int func_001F9850(int);
extern int func_L00_002DDEA0(void *, void *);
extern void func_L18_002FD5C0(void *);
extern float fd9a0_c(void *, void *) __asm__("func_L18_002FD9A0");
extern float f9d48_i(void *, void *) __asm__("func_001F9D48");
extern void func_00213DE0(void *, int, int, int);
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern int func_00215B18(char *, float);
extern void func_L00_0025AA20(void *, float, float, float, int, int, int, int, int);
extern int func_L00_0025D6F0(void *, void *);
extern void func_L00_0025F4A8_alt(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int)
    __asm__("func_L00_0025F4A8");
extern void func_001FA1F8(void *, void *);
extern float func_00214D28(float *p, float target, float maxstep);
extern void func_001F9C30(void *, void *, float);
extern void func_001F49B0(void *, void *);
extern void func_L18_002FDD20(void);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L18_00162680;
extern short D_L18_00162690;
extern char D_L18_00162698;

void func_L18_002FD058(unsigned char *m) {
    char tmp[16];
    unsigned char *e = *(unsigned char **)(m + 0x78);
    unsigned char *s0;
    func_L18_002FD5C0(m);
    switch (m[0x20]) {
    case 0: {
        unsigned short h;
        m[0x20] = 8;
        if (func_002140B0(2)) *(unsigned short *)(m + 0x34) |= 0x8000;
        else *(unsigned short *)(m + 0x34) &= 0x7FFF;
        *(unsigned short *)(m + 0x34) |= 0x100;
        m[0x73] = D_L18_00162698;
        *(void **)(e + 0x140) = &D_L18_00162680;
        e[0x58] = 8;
        {
            float half = 0.5f;
            *(float *)(e + 0x20) = 1.0f;
            *(short *)(e + 0x24) = 1;
            e[0x5C] = 1;
            *(float *)(e + 0x30) = half;
        }
        *(float *)(e + 0x1F8) = func_00214158();
        m[0x30] = 0xFF;
        h = *(unsigned short *)(m + 0x34);
        *(short *)(m + 0x32) = 0xFF;
        h |= 1;
        m[0x31] = 0;
        h &= 0xEFFF;
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) = h;
        break;
    }
    case 1: {
        float a = 10.0f;
        float r;
        s0 = m + 0x10;
        *(float *)(e + 0x1D8) = *(float *)(e + 0x1D8) - D_0015EE70 * a;
        func_001F9BD8(s0, s0, e + 0x1D0);
        qcopy(tmp, s0);
        ((float *)tmp)[2] += 1.0f;
        r = func_00214358(tmp, 0, 0.5f);
        if (*(float *)(m + 0x18) < r) {
            *(float *)(m + 0x18) = r;
            m[0x20] = 2;
            if (m[0x53]) func_00213DE0(m, 0, 0, func_001F9850(10));
        } else if (*(float *)(m + 0x18) < a) {
            m[0x20] = 6;
        }
        break;
    }
    case 7:
        if (func_L00_002DDEA0(m, e + 0x70)) {
            m[0x20] = 2;
            *(short *)(e + 0x138) = 0;
        }
        break;
    case 2:
        if (*(int *)(e + 0x1C4) == 2) {
            if (!(10.0f < f9d48_i(m + 0x10, e + 0x180))) break;
        }
        m[0x20] = 3;
        if (m[0x53] != 1) func_00213DE0(m, 1, 0, func_001F9850(10));
        break;
    case 3: {
        float f;
        *(QVd058 *)tmp = *(QVd058 *)(e + 0x180);
        f = fd9a0_c(m, tmp);
        if (*(int *)(e + 0x1C4) == 2 && f < 5.0f) {
            m[0x20] = 2;
            if (m[0x53]) func_00213DE0(m, 0, 0, func_001F9850(10));
            break;
        }
        if (f < 1.5f) {
            m[0x20] = 4;
            if (m[0x53] != 2) {
                int v = (int)*(float *)&D_L18_00162690;
                func_00213DE0(m, 2, v, func_001F9850(5));
            }
        }
        break;
    }
    case 4: {
        float r, k, b, d;
        s0 = m + 0x48;
        r = func_L00_001FF860(*(float *)(e + 0x180) - *(float *)(m + 0x10), *(float *)(e + 0x184) - *(float *)(m + 0x14));
        k = 6.2831855f;
        b = D_0015EE70 * k;
        d = D_0015EE6C * k;
        func_L00_0025CE58((float *)s0, (float *)(e + 0x1E8), r, b, b, d);
        if (func_00215B18((char *)m, 14.0f)) func_L00_0025AA20(m, 0.5f, 1.0f, 1.0f, 1, 1, 0, 1, 0);
        if (m[0x70] & 2) {
            if (*(int *)(e + 0x1C4) == 2) {
                m[0x20] = 2;
                if (m[0x53]) func_00213DE0(m, 0, 0, func_001F9850(10));
                break;
            }
            if (!(2.5f < f9d48_i(m + 0x10, e + 0x180))) break;
            m[0x20] = 3;
            if (m[0x53] != 1) func_00213DE0(m, 1, 0, func_001F9850(10));
            break;
        }
        break;
    }
    case 5:
        if (!(func_L00_0025D6F0(m, e + 0x70) & 0x140)) break;
    case 6:
        qcopy(tmp, m + 0x10);
        ((float *)tmp)[2] += 0.5f;
        func_L00_0025F4A8_alt(m, e + 0x40, tmp, 0.0f, 0.0f, 5, 2, 4, 1.0f, 0.5f, 9.0f, 0.5f, 4, 0.0f, 0, 1, -1, 0);
        m[0x20] = 8;
        if (m[0x53]) func_00213DE0(m, 0, 0, func_001F9850(10));
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) = (*(unsigned short *)(m + 0x34) | 0x41) & 0xEFFF;
        break;
    }
    s0 = m + 0xC0;
    func_001FA1F8(s0, m + 0x40);
    if (*(float *)(e + 0x1F4) != 0.0f) {
        float g;
        func_00214D28((float *)(e + 0x1F4), 0.0f, D_0015EE6C * 4.0f);
        g = 1.0f;
        func_001F9C30(m + 0xE0, m + 0xE0, *(float *)(e + 0x1F4) + g);
        g = g - *(float *)(e + 0x1F4);
        func_001F9C30(s0, s0, g);
        func_001F9C30(m + 0xD0, m + 0xD0, g);
        func_001F49B0(func_L18_002FDD20, m);
    }
}
