/* NON_MATCHING func_L00_002A86D8 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: BYTES 40/836 (95.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns one of four debris mobys (class 0xD/0xE/0xF/0x10 by kind 0/5/0x14/0x32, scale 0.65-0.75), parents it to
 *   Best is p6.c (size 844 vs 836): switch compare tree matched with gotos (if 5 / if <6 / 0x14 / 0x32 / default f
 *   NOTE: src/overlays/shared/vendor_002A5138.c now declares `extern int func_L00_002A84B0(void *, int, float, flo
 *   q27/s04: p11 = p6 with the func_L00_002A84B0 prototype (void*,int,float,float) and `flags & 2` written at its 
 */
#include "common.h"
extern void *func_0020D348(int oClass);
extern void func_001FA1F8(void *, void *);
extern void func_L00_00251328(void *, int, int, int);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FF4B0(float *, float *, float);
extern void func_00215380(void *arg0, void *axis, float angle);
extern int func_L00_002A84B0(void *, int, float, float);
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern short D_L00_00161470;
extern char D_0013E633[];

/* Spawns a debris moby of a class chosen by kind, parented to the root of parent's chain, and sets up its motion data. */
void *func_L00_002A86D8(char *parent, void *pos, void *vel, int flags, int kind, int b) {
    char *m;
    char *d;
    char *top;
    char *q;
    float scale;
    float f20;
    float tmp[4];
    float cr[4];

    if (kind == 5)
        goto c5;
    if (kind < 6)
        goto dflt;
    if (kind == 0x14)
        goto c14;
    if (kind == 0x32)
        goto c32;
dflt:
    m = func_0020D348(0xD);
    scale = 0.65f;
    goto join;
c5:
    m = func_0020D348(0xE);
    scale = 0.7f;
    goto join;
c14:
    m = func_0020D348(0xF);
    scale = 0.75f;
    goto join;
c32:
    m = func_0020D348(0x10);
    scale = 0.6f;
join:
    if (m != 0) {
        d = *(char **)(m + 0x78);
        d[0x40] = b;
        top = parent;
        if (parent != 0) {
            while (*(char **)(top + 0xB8) != 0)
                top = *(char **)(top + 0xB8);
        }
        *(unsigned short *)(m + 0x34) |= 0x100;
        *(char **)(m + 0xB8) = top;
        m[0x73] = 0x14;
        m[0x30] = 0x40;
        *(short *)(m + 0x32) = 0x40;
        m[0x20] = 1;
        m[0x31] = 1;
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * scale;
        func_001FA1F8(m + 0xC0, m + 0x40);
        if ((*(unsigned short *)(parent + 0x34) & 0x20) && (q = *(char **)(*(char **)(parent + 0x78) + 0xC)) != 0) {
            func_L00_00251328(m, (unsigned char)q[4], (unsigned char)q[5], (unsigned char)q[6]);
        } else {
            *(long *)(m + 0x38) = *(long *)(parent + 0x38);
        }
        *(short *)(d + 0x6E) = func_L00_00258BC8(0, func_001F9850(0x258));
        *(short *)(d + 0x6C) = -1;
        qcopy(m + 0x10, pos);
        qcopy(d + 0x20, vel);
        if (func_002140B0(2) != 0)
            d[0x55] |= 1;
        *(float *)(d + 0x50) = func_00214158();
        *(float *)(d + 0x58) = func_00214158();
        *(float *)(d + 0x68) = *(float *)(m + 0x18);
        f20 = func_002140F8(0.0f, *(float *)&D_L00_00161470) * 0.017453292f * D_0015EE6C;
        func_001F9BF0(tmp, parent + 0x10, m + 0x10);
        func_001F9CA0(cr, d + 0x20, tmp);
        func_L00_001FF4B0((float *)(d + 0x30), cr, 1.0f);
        func_00215380(d + 0x30, d + 0x30, f20);
        if (flags & 2) {
            float a = 0.0f;
            if (flags & 0x10)
                a = func_002140F8(-30.0f, 30.0f) * 0.017453292f;
            func_L00_002A84B0(m, *(int *)(D_0013E633 + 0x2E9D), a, 0.0f);
        }
        func_L00_00251E30(m);
    } else {
        D_0015EE98 += kind;
    }
    return m;
}
