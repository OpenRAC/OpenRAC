/* NON_MATCHING func_L18_002F7F00 -- src/overlays/l18_veldin2/vendor_002F2AE0.c
 * Best so far: SIZE ours 868 / retail 876, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Logic is fully decoded; p1.c is correct instruction-for-instruction except:
 *   1. retail holds %hi(D_0013E633+0xE1D) in $s2 and re-adds %lo for the 0x208C load
 *   (second LO_SUM from one CSE'd hi); ours either re-emits lui (no s2, 864/868) or
 *   keeps the full address (run 2). Tried: one pointer, two pointers, fresh expressions,
 *   alias name for the 0xE9D access, -mno-split-addresses (868 same). Wall: hi CSE.
 *   2. v[2]=0 store placement: retail sw before mul/swc1 v[1] in the merged join; ours
 *   after or (in per-branch form) not merged.
 *   3. c.lt.s/nop placement after mtc1 $0 in first compare (minor schedule tie).
 */
extern char D_0013E633[];
extern char D_0013E633_b[] __asm__("D_0013E633");
extern char *D_L18_0016016C MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L18_00162400;
extern short D_L18_00162404;
extern short D_L18_001623EC;
extern short D_L18_001623E8;
extern short D_L18_001623E4;
extern short D_L18_001623E0;
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern int func_L18_002F7CD8(char *moby, float arg, void *x);
extern float func_00214D28(float *p, float target, float maxstep);

void func_L18_002F7F00(unsigned char *m) {
    char *a = D_0013E633 + 0xE1D;
    char *a2 = D_0013E633 + 0xE1D;
    char *d = *(char **)(m + 0x78);
    float v[4];
    float g, x, y, ang, lim, len, lo, hi, t;
    x = func_L00_001FF860(*(float *)(a + 0x80) - *(float *)(D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x30),
                          *(float *)(a + 0x84) - *(float *)(D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x34));
    y = func_L00_001FF860(*(float *)(d + 0x3C0) - *(float *)(D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x30),
                          *(float *)(d + 0x3C4) - *(float *)(D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x34));
    ang = func_001FA748(x, func_001FA790(y, x) < 0.0f ? -2.3561945f : 2.3561945f);
    ang = func_001FA790(ang, y);
    if (ang > 0.17453292f) {
        ang = 0.17453292f;
    } else if (ang < -0.17453292f) {
        ang = -0.17453292f;
    }
    ang = func_001FA748(ang, y);
    if (m[0x20] == 2) {
        v[0] = func_001F9F90(ang) * *(float *)&D_L18_00162400;
        v[1] = func_001F9FA8(ang) * *(float *)&D_L18_00162400;
    } else {
        v[0] = func_001F9F90(ang) * *(float *)&D_L18_00162404;
        v[1] = func_001F9FA8(ang) * *(float *)&D_L18_00162404;
    }
    *(int *)&v[2] = 0;
    func_001F9BD8(v, v, D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x30);
    len = func_001F9D48(d + 0x3C0, D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x30);
    if (len > *(float *)&D_L18_00162404 + 1.0f) {
        t = 10.0f;
    } else if (*(int *)(a2 + 0x208C) == 0xF) {
        t = 20.0f;
    } else {
        float k0, k1;
        lo = *(float *)&D_L18_001623EC;
        hi = *(float *)&D_L18_001623E8;
        k0 = *(float *)&D_L18_001623E4;
        k1 = *(float *)&D_L18_001623E0;
        if (m[0x20] == 0xD) {
            lo *= 0.5f; hi *= 0.5f; k0 = 2.0f; k1 = 5.5f;
        } else if (m[0x20] == 0xF) {
            lo *= 1.5f; hi *= 1.5f; k0 = 6.0f; k1 = 10.0f;
        }
        len = func_001F9D48(m + 0x10, D_0013E633 + 0xE9D);
        if (len > hi) {
            len = hi;
        } else if (len < lo) {
            len = lo;
        }
        t = (k1 - k0) * (1.0f - (len - lo) / (hi - lo)) + k0;
    }
    func_L18_002F7CD8(m, t * D_0015EE6C, v);
    func_00214D28((float *)(d + 0x3C8), *(float *)(D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x38), D_0015EE6C * 4.0f);
}
