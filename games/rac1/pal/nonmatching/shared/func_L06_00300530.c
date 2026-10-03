/* NON_MATCHING func_L06_00300530 -- src/overlays/shared/vendor_002FF000.c
 * Best so far: SIZE ours 448 / retail 452, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds 16 segment vectors in moby data (loop, trig of yaw fan), then sets d+0x21C. p4.c closest (12 bytes shor
 *   Retail keeps 0.0f, 1.0f, 0.15f, 0.5f in saved FP regs (mtc1 $zero,$f23; mov.s $f25,$f20) and stores with swc1;
 *   Unblock: a source form (not found: local floats, ternary, phi variable) that stops constant folding of the 0.0
 */
extern float func_00214358(void *, int, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_00250800(void *, int, void *);
extern float func_001FA888(int);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(float *, float *, float *);
extern int func_001F9850(int);
extern char D_L06_00174700[];
extern short D_L06_00162064;
extern short D_L06_00162068;
// Builds the sixteen-segment fan of data vectors for a moby from its yaw.
void func_L06_00300530(char *m) {
    char *d = *(char **)(m + 0x78);
    float v[4];
    int i;
    float half = 0.5f;
    float one = 1.0f;
    float zero = 0.0f;
    func_00214358(m + 0x10, 0, half);
    func_L00_001FF4B0(d + 0x220, D_L06_00174700, one);
    func_L00_00250800(m, 0, d + 0x210);
    *(float *)(d + 0x218) = *(float *)(m + 0x18);
    for (i = 0; i < 16; i++) {
        float f = func_001FA888(i) * 0.0625f - half;
        f = f * (*(float *)&D_L06_00162064 * 0.017453292f);
        v[0] = func_001F9F90(func_001FA748(f, *(float *)(m + 0x48))) * 0.2f;
        v[1] = func_001F9FA8(func_001FA748(f, *(float *)(m + 0x48))) * 0.2f;
        v[2] = 0.15f;
        func_001F9BD8((float *)(d + 0x230 + i * 16), (float *)(d + 0x210), v);
        {
            float x;
            if (i == 0) {
                x = zero;
            } else if (i == 15) {
                x = zero;
            } else {
                x = one;
            }
            *(float *)(d + 0x23C + i * 16) = x;
        }
    }
    *(float *)(d + 0x21C) = (float)func_001F9850(*(int *)&D_L06_00162068);
}
