/* NON_MATCHING func_L15_002C62A8 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 824 / retail 816, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steers a moby toward a target point (speed clamp, a heading step via 00259148 twice, two 9BF0/9C30 vector blen
 *   Wall for the u128 form: a TI-mode local whose address is taken (&cur, &tmp, &vt) makes this compiler crash wit
 */
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FF240(void *, void *, void *);
extern void func_L15_00248E58(void *, void *, int, float, float);
extern float func_L00_001FF860(float, float);
extern float func_L00_00259148(float *vel, float cur, float target, float k, float d, float max);
extern float func_001FA790(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

// Steers a moby toward a target point: updates its speed and heading.
void func_L15_002C62A8(char *moby, char *a1, char *a2) {
    char *data = *(char **)(moby + 0x78);
    float cur[4];
    float tgt[4];
    float mpos[4];
    float tmp[4];
    float vt[4];
    float d;
    float s;
    float k;
    float a;
    float r;
    float q;
    float b;
    float f20v;
    float f21v;
    float y;

    qcopy(cur, a1);
    qcopy(tgt, a2);
    qcopy(mpos, moby + 0x10);
    d = func_001F9D10(moby + 0x10, cur);
    s = *(float *)(data + 0x170);
    k = D_0015EE70 * 10.0f;
    if (d <= s * s / (k + k)) {
        *(float *)(data + 0x170) = s - k;
        if (s - k < 0.0f) *(float *)(data + 0x170) = 0.0f;
    } else {
        *(float *)(data + 0x170) = s + k;
        if (D_0015EE6C * 10.0f < s + k) *(float *)(data + 0x170) = D_0015EE6C * 10.0f;
    }
    func_001F9BF0(tmp, cur, moby + 0x10);
    qcopy(vt, tmp);
    func_L00_001FF4B0(vt, vt, *(float *)(data + 0x170) * 0.1f);
    func_001F9C30(tmp, data + 0x150, 0.9f);
    qcopy(data + 0x150, tmp);
    func_L00_001FF240(tmp, data + 0x150, vt);
    func_L15_00248E58(moby, data + 0x150, 0, 1.0f, 2.0f);
    *(float *)(moby + 0x18) = *(float *)(data + 0x1A8);
    a = func_L00_001FF860(tgt[0] - *(float *)(moby + 0x10), tgt[1] - *(float *)(moby + 0x14));
    *(float *)(moby + 0x48) = func_L00_00259148((float *)(data + 0x17C), *(float *)(moby + 0x48), a, 0.01f, 0.3f, 0.1f);
    func_001F9BF0(tmp, moby + 0x10, mpos);
    qcopy(data + 0x150, tmp);
    r = func_001FA790(func_L00_001FF860(*(float *)(data + 0x150), *(float *)(data + 0x154)),
                      *(float *)(moby + 0x48));
    q = func_001F9F90(r);
    f20v = *(float *)(data + 0x170) * 0.34906585f * q / (D_0015EE6C * 10.0f);
    b = func_001F9FA8(r);
    f21v = *(float *)(data + 0x170) * -0.34906585f * b / (D_0015EE6C * 10.0f);
    func_L00_00259148((float *)(data + 0x178), *(float *)(moby + 0x44), f20v,
                      D_0015EE70 * 0.5235988f, D_0015EE70 * 1.0471976f, D_0015EE6C * 0.7853982f);
    y = func_L00_00259148((float *)(data + 0x174), *(float *)(moby + 0x40), f21v,
                          D_0015EE70 * 1.0471976f, D_0015EE70 * 2.0943951f, D_0015EE6C * 1.5707964f);
    *(float *)(moby + 0x44) = y;
    *(float *)(moby + 0x40) = y;
}
