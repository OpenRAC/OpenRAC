/* NON_MATCHING func_L07_0030FC38 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 328 / retail 320, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Initializes an emitter data block (floats at 0x10..0x54, int 0x599/0xD, byte 0x3D=0), copies a 16-byte vector 
 */
extern void func_L00_0025BBA0(void *, float *, void *, void *);
extern void func_L00_0025D5B0(float, void *, void *, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
typedef int u128 __attribute__((mode(TI)));

// Initializes a particle emitter's data block and runs its first update.
void func_L07_0030FC38(char *m, char *o, void *vec) {
    u128 v[1];
    float out[4];
    v[0] = *(u128 *)vec;
    *(float *)(o + 0x10) = D_0015EE70 * 29.4f;
    *(float *)(o + 0x14) = 0.0005f;
    *(float *)(o + 0x18) = D_0015EE6C * 9.0f;
    *(float *)(o + 0x1C) = D_0015EE6C * 7.0f;
    *(int *)(o + 0x20) = 0x599;
    *(float *)(o + 0x28) = 1.4f;
    *(float *)(o + 0x30) = 0.2300f;
    *(float *)(o + 0x34) = 0.5f;
    *(float *)(o + 0x38) = 0.65f;
    *(float *)(o + 0x48) = 0.2f;
    *(float *)(o + 0x4C) = 0.01f;
    *(float *)(o + 0x50) = 8.0f;
    *(float *)(o + 0x54) = 19.0f;
    *(int *)(o + 0x24) = 0xD;
    o[0x3D] = 0;
    func_L00_0025BBA0(v, out, o + 0x18, o + 0x1C);
    func_L00_0025D5B0(out[0], m, o, 6, 1, 0);
}
