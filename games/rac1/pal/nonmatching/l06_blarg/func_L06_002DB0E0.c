/* NON_MATCHING func_L06_002DB0E0 -- src/overlays/l06_blarg/vendor_002B5990.c
 * Best so far: BYTES 16/248 (93.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_367: switch on moby[0x20] (case 0 copies pos into data, transforms, sets state 1; case 2 eases a sc
 *   Difference: only the s0/s1 swap (retail data=$s0, moby+0x10=$s1; ours reversed). Tried local-order, inline mob
 *   D_0015EE70 needs lui+lwc1 but the file declares it short (gp-rel) later; p4 reads it as (&D_0015EE6C)[1] to ge
 */
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214D88(float *, float *, float, float, float, float);
extern void func_001F9C08(void *, void *, void *, float);
extern float D_0015EE6C MACRO_ADDR;

// Updates a moby that swells its scale vector toward a target.
void func_L06_002DB0E0(unsigned char *moby) {
    char *data;
    data = *(char **)(moby + 0x78);
    switch (moby[0x20]) {
    case 0: {
        char *p = (char *)moby + 0x10;
        char *d = data;
        qcopy(d, p);
        data = d + 0x10;
        func_L00_001FF4B0(data, moby + 0xD0, 11.0f);
        func_001F9BD8(data, data, p);
        moby[0x20] = 1;
        break;
    }
    case 2: {
        float a = (&D_0015EE6C)[1] * 10.0f;
        func_00214D88((float *)(data + 0x20), (float *)(data + 0x24), *(float *)(data + 0x28), a, a, D_0015EE6C * 20.0f);
        func_001F9C08(moby + 0x10, data, data + 0x10, *(float *)(data + 0x20));
        if (*(float *)(data + 0x20) == *(float *)(data + 0x28)) moby[0x20] = 1;
        break;
    }
    }
}
