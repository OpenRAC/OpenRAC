/* NON_MATCHING func_L12_00309AE8 -- src/overlays/shared/vendor_002BD3D0.c
 * Best so far: BYTES 28/524 (94.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Launch setup for a moby: random angle, direction vector, speeds from D_0015EE70*40/*100 and func_001F9B50, the
 *   Remaining (p5.c, SIZE 532 vs 524): ours reloads D_0015EE70 for d+0x34 (extra lui/lwc1) where retail reuses the
 *   Unblock: find the wording that shares the D_0015EE70 load across the two stores (CSE) while keeping the later 
 */
extern int func_002140B0(int);
extern void func_0022ED80(int, int, int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001F9B50(float);
extern void func_L00_0025BBA0(void *, float *, void *, void *);
extern void func_L00_0025D5B0(float, void *, void *, int, int, int);
extern void func_00213DE0(void *, int, int, int);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

// Initializes a moby's launch parameters from a random angle and starts its animation.
void func_L12_00309AE8(int unused, char *m) {
    char *d = *(char **)(m + 0x78);
    float v[5];
    float p, q, s1, s2;
    if (func_002140B0(3) == 0) {
        func_0022ED80(1, 0x20, (int)m);
    }
    *(float *)(d + 0x48) = 0.7f;
    *(int *)(d + 0x40) = func_001FA898_r(716.8f);
    *(int *)(d + 0x6C) = 0;
    *(unsigned short *)(m + 0x34) &= 0xFFF9;
    m[0x30] = 0xFF;
    v[4] = func_001FA748(*(float *)(m + 0x48), 3.14159274f);
    v[0] = func_001F9F90(v[4]);
    v[1] = func_001F9FA8(v[4]);
    v[2] = 1.0f;
    v[3] = 5499.9f;
    s1 = D_0015EE70 * 40.0f;
    *(float *)(d + 0x34) = D_0015EE70 * 100.0f;
    *(float *)(d + 0x30) = s1;
    p = func_001F9B50(s1 * 4.0f);
    q = func_001F9B50(D_0015EE70 * 40.0f * 4.0f);
    s2 = D_0015EE70 * 40.0f;
    p = (p + p) / s2;
    q = (q + q) / s2;
    *(float *)(d + 0x38) = 3.0f / p + D_0015EE70 * 100.0f * 0.5f * q;
    *(float *)(d + 0x3C) = func_001F9B50(s2 * 4.0f);
    d[0x5D] = 0;
    *(int *)(d + 0x44) = 1;
    func_L00_0025BBA0(v, v + 4, d + 0x38, d + 0x3C);
    func_L00_0025D5B0(v[4], m, d + 0x20, 1, 5, 2);
    *(float *)(d + 0x6C) = D_0015EE6C + D_0015EE6C;
    *(float *)(d + 0x70) = -1.0f;
    *(float *)(d + 0x74) = -1.0f;
    *(unsigned short *)(m + 0x34) &= 0xEFFF;
    func_00213DE0(m, 1, 0, 0);
}
