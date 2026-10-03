/* NON_MATCHING func_L03_002D5220 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 132 / retail 128, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef union { long long quad; float f[4]; } L03Vector;
extern float D_0015EE6C;
extern float func_001FA748(float, float);
extern void func_L00_002617B0(void *, void *, void *, void *);

void func_L03_002D5220(unsigned char *moby) {
    L03Vector zero;
    L03Vector vector;
    char *data = *(char **)(moby + 0x78);
    zero.quad = 0;
    vector.quad = *(long long *)(moby + 0x40);
    moby[0x30] = 0xFF;
    *(short *)(moby + 0x32) = 0xFF;
    *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), D_0015EE6C * 0.47996554f);
    func_L00_002617B0(data + 0x20, &zero, &vector, moby + 0x40);
}
