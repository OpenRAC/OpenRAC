/* NON_MATCHING func_L07_0030FB00 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 316 / retail 308, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fills an effect descriptor with float constants (scaled by D_0015EE6C/EE70), copies vec to stack, calls func_L
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_L00_0025BBA0(void *, float *, void *, void *);
extern void func_L00_0025D5B0(float, void *, void *, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

/* Fill an effect descriptor with constants and start it from a position. */
void func_L07_0030FB00(char *m, char *s, float *v) {
    float a[4] __attribute__((aligned(16)));
    float out[4] __attribute__((aligned(16)));
    *(float *)(s + 0x14) = 0.002f;
    *(float *)(s + 0x18) = D_0015EE6C * 9.0f;
    *(float *)(s + 0x1C) = D_0015EE6C * 7.0f;
    *(float *)(s + 0x10) = D_0015EE70 * 29.4f;
    *(int *)(s + 0x20) = 0x400;
    *(float *)(s + 0x28) = 1.0f;
    *(float *)(s + 0x30) = 0.24f;
    *(float *)(s + 0x34) = 0.5f;
    *(float *)(s + 0x38) = 0.65f;
    *(float *)(s + 0x48) = 0.2f;
    *(float *)(s + 0x4C) = 0.01f;
    *(float *)(s + 0x50) = 3.0f;
    *(float *)(s + 0x54) = 9.0f;
    *(int *)(s + 0x24) = 0xD;
    *(char *)(s + 0x3D) = 0;
    *(u128 *)a = *(u128 *)v;
    func_L00_0025BBA0(a, out, s + 0x18, s + 0x1C);
    func_L00_0025D5B0(out[0], m, s, 0xC, 1, 0);
}
