/* NON_MATCHING func_L00_002AB170 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 304 / retail 308, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a class 0x4A moby, initializes its data block (d = m+0x78): two func_002140F8(D_0015EE6C*k1, D_0015EE6C
 *   Size matches (308) with the zero floats written as a chain `a = b = c = d = 0.0f` (forces mtc1 $zero,$f1); bes
 *   Unblock: a different source shape for the zero stores/0x5C load that changes scheduler priority; 10 runs spent
 */
extern void *func_0020D348(int oClass);
extern void func_001F9BC0(void *);
extern float func_002140F8(float, float);
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;

/* spawns a class 0x4A moby and initializes its data block */
void *func_L00_002AB170(int a0, void *pos) {
    char *m = func_0020D348(0x4A);
    char *d;
    float z = 0.0f;
    if (m != 0) {
        d = *(char **)(m + 0x78);
        *(unsigned char *)(m + 0x30) = 0xFF;
        *(short *)(m + 0x32) = 0x7F;
        *(unsigned char *)(m + 0x31) = 1;
        *(float *)(m + 0x2C) = *(float *)(m + 0x2C) * 0.5f;
        *(unsigned char *)(m + 0x20) = 0;
        *(int *)(d + 0x30) = a0;
        *(short *)(d + 0x36) = 0;
        *(int *)(d + 0x78) = 0;
        qcopy(m + 0x10, pos);
        func_001F9BC0(d);
        *(float *)(d + 0x38) = func_002140F8(D_0015EE6C * 1.0471975f, D_0015EE6C * 3.1415927f);
        *(float *)(d + 0x3C) = func_002140F8(D_0015EE6C * 1.0471975f, D_0015EE6C * 3.1415927f);
        *(int *)(d + 0x68) = -1;
        *(float *)(d + 0x6C) = z;
        *(int *)(d + 0x5C) = D_L00_0015F6B0;
        *(float *)(d + 0x64) = z;
        *(float *)(d + 0x60) = z;
        *(float *)(d + 0x70) = z;
        *(int *)(d + 0x74) = 0;
        *(int *)(m + 0x94) = 0;
        func_L00_00251E30(m);
    }
    return m;
}
