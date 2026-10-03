/* NON_MATCHING func_L01_00309BB8 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 14/528 (97.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Computes a blend rotation for a moby (two table indices at data+4/+8; -1 uses hero angle path), calls func_L00
 *   p3.c is 14/528 bytes off: (1) the 0x6B94($gp) table is D_L01_0016016C itself (use one MACRO_ADDR symbol, drop 
 *   (2) float constants 0x3F32B8C2, 0x3EB2B8C2, 0x40490FD0 need literals that round to those exact bits (ours gave
 *   (3) hero base D_0013E633+0xE1D should be held in one pointer local (s0) with +0x98/+0x80 offsets. Budget spent
 */
typedef int u128_309BB8 __attribute__((mode(TI)));
extern float func_0020D830(void);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);
extern void func_001F9C08(void *, void *, void *, float);
extern float func_001FA790(float, float);
extern void func_L00_002EBE88(void *);
extern void func_L00_002EBEE0(void *);
extern char D_0013E633[];
extern char *D_L01_0016016C MACRO_ADDR;
extern char *D_L01_0015F16C MACRO_ADDR;

// Computes a rotation/blend for a moby from its data and passes it to two helpers.
void func_L01_00309BB8(char *m) {
    float out[4];
    float v[4];
    float a[4];
    float b[4];
    char *d = *(char **)(m + 0x78);
    float f;
    float k;
    char *t;
    if (*(unsigned char *)(m + 0x52) == 1) {
        f = func_0020D830();
    } else {
        f = 170.0f;
    }
    k = f / 170.0f;
    if (*(int *)(d + 4) == -1 || *(int *)(d + 8) == -1) {
        float ang = func_001FA748(k * 0.69813174f - 0.34906587f, *(float *)(D_0013E633 + 0xE1D + 0x98));
        out[0] = func_001F9F90(ang) * 5.0f;
        out[1] = func_001F9FA8(ang) * 5.0f;
        out[2] = 0;
        func_001F9BD8(out, out, D_0013E633 + 0xE1D + 0x80);
        out[2] = out[2] + 1.0f;
        func_001F9BC0(v);
        v[2] = func_001FA748(ang, 3.1415927f);
        v[1] = k * -0.17f;
    } else {
        func_001F9C08(out, D_L01_0015F16C + *(int *)(d + 4) * 128 + 0x30,
                      D_L01_0015F16C + *(int *)(d + 8) * 128 + 0x30, k);
        t = D_L01_0016016C;
        qcopy(a, t + *(int *)(d + 4) * 128 + 0x70);
        qcopy(b, t + *(int *)(d + 8) * 128 + 0x70);
        v[2] = func_001FA748(func_001FA790(b[2], a[2]) * k, a[2]);
        v[1] = func_001FA748(func_001FA790(b[1], a[1]) * k, a[1]);
        v[0] = func_001FA748(func_001FA790(b[0], a[0]) * k, a[0]);
    }
    func_L00_002EBE88(out);
    func_L00_002EBEE0(v);
}
