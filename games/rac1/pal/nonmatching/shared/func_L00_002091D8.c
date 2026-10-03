/* NON_MATCHING func_L00_002091D8 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 948 / retail 956, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002091D8 (9 of 10 runs used; best candidate p8.c, 948 vs 956 bytes, p7.c is close too): steers the pl
 *   (clamped yaw delta via func_001FA790/001FA748, aim vector into D_L00_001AEF10), then runs a 4-probe loop over 
 *   collecting lo/hi limits into moby+0x84/0x88.
 *   Difference: saved float registers. Retail puts cs(F90(ang)) in $f26 and the 2^-10 loop constant in $f25; ours 
 *   (cs $f25, const $f26), and that reorders the loop's mul/add schedule. The loop's p-pointer hi is a fresh lui i
 *   shared with $s5 (PRE) in p7; p8 makes it fresh but moves a lui into the bc1f delay slot. Structure otherwise r
 *   Unblock: something that lowers cs's allocation priority relative to the hoisted constant (unknown), then re-tu
 */
extern unsigned char D_0013E633[];
extern char D_L00_001AEF10[];
extern float D_L00_0017C400[];
extern int D_L00_00173F40[];
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);
extern void func_L00_00251388(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern float func_001F9B98(float, float);
extern float func_001F9B90(float, float);

// Steers the player moby's aim toward its target, then fits the moby's two span limits over four probe offsets.
void func_L00_002091D8(void) {
    char *p = (char *)D_0013E633 + 0xE1D;
    int s = *(int *)(p + 0x208C);
    float a[4];
    float v1[4];
    float v2[4];
    float x, c, c2, ang, sc, lo, hi, t, sn, cs, b;
    float *tb;
    int i;
    if (s == 0x12 || s == 0x15 || *(int *)(p + 0x2084) == 0x7D || s == 0x16) {
        char *q = (char *)D_0013E633 + 0xE1D;
        *(unsigned short *)(*(char **)(q + 0x2080) + 0x34) &= 0xFBFF;
        return;
    }
    *(unsigned short *)(*(char **)(p + 0x2080) + 0x34) |= 0x400;
    *(char *)(*(char **)(p + 0x2080) + 0xBD) = 1;
    x = 0.97f;
    if (*(short *)(p + 0x30E) != 0 || *(int *)(p + 0x208C) == 4) x = 1.5f;
    {
        char *q = (char *)D_0013E633 + 0xE1D;
        c = func_001FA790(x, *(float *)(q + 0xD20));
    }
    if (0.052f < c) c = 0.052f;
    else if (c < -0.052f) c = -0.052f;
    {
        char *g = (char *)D_0013E633 + 0xE1D;
        *(float *)(g + 0xD20) = func_001FA748(*(float *)(g + 0xD20), c);
        func_L00_00251388(*(char **)(g + 0x2080), a);
        ang = func_L00_001FF860(a[0], a[1]);
        cs = func_001F9F90(ang);
        sn = func_001F9FA8(ang);
        c2 = func_001F9F90(*(float *)(g + 0xD20));
        a[0] = cs * c2;
        a[1] = sn * c2;
        a[2] = -func_001F9FA8(*(float *)(g + 0xD20));
        qcopy(D_L00_001AEF10, a);
        t = *(float *)(g + 0x2D8);
        if (t < 1.0f) {
            b = *(float *)(g + 0x2F0);
            if (1.0f <= b) {
                t = b;
            } else {
                b = *(float *)(g + 0x2F4);
                if (1.0f < b) {
                    t = b;
                } else {
                    *(int *)(*(char **)(g + 0x2080) + 0x84) = 0;
                    *(int *)(*(char **)(g + 0x2080) + 0x88) = 0;
                    return;
                }
            }
        }
        hi = t;
    }
    {
        char *dd = (char *)D_L00_00173F40;
        tb = D_L00_0017C400;
        lo = hi;
        for (i = 3; i >= 0; i--) {
            char *q = (char *)D_0013E633 + 0xE1D;
            char *m = *(char **)(q + 0x2080);
            sc = *(float *)(m + 0xC) * 0.0009765625f;
            func_001F9C30(v1, m, 0.0009765625f);
            v1[2] = v1[2] + tb[2] * sc;
            v1[0] = v1[0] + cs * tb[0] * sc;
            v1[1] = v1[1] + sn * tb[1] * sc;
            func_L00_001FF4B0(v2, a, (v1[2] - t + 0.5f) / -a[2]);
            func_001F9BD8(v2, v1, v2);
            if (func_L00_001EFFF0(v1, v2, 0x22, 0, 0) != 0) {
                lo = func_001F9B98(lo, *(float *)(dd + 0x28));
                hi = func_001F9B90(hi, *(float *)(dd + 0x28));
            }
            tb += 4;
        }
    }
    {
        char *q = (char *)D_0013E633 + 0xE1D;
        *(float *)(*(char **)(q + 0x2080) + 0x84) = lo - 0.12f;
        *(float *)(*(char **)(q + 0x2080) + 0x88) = hi + 0.24f;
        if (*(float *)(*(char **)(q + 0x2080) + 0x84) + 3.0f < *(float *)(*(char **)(q + 0x2080) + 0x88))
            *(float *)(*(char **)(q + 0x2080) + 0x88) = *(float *)(*(char **)(q + 0x2080) + 0x84) + 3.0f;
    }
}
