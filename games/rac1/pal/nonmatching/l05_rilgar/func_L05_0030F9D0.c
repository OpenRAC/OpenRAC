/* NON_MATCHING func_L05_0030F9D0 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 7/748 (99.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L05_001601AC_p __asm__("D_L05_001601AC") MACRO_ADDR;
extern char *D_L05_00160098_p __asm__("D_L05_00160098") NOT_SDA;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern float func_00214D88_p(float *, float *, float, float, float, float) __asm__("func_00214D88");
extern void func_001F9BC0(void *);

/* Rising platform: placed out of its pose, waits for its switch, slides into place, then sinks when told. */
void func_L05_0030F9D0(char *m) {
    char *d = *(char **)(m + 0x78);
    float v[4];
    float spd;
    float len;
    float dist;
    switch (((unsigned char *)m)[0x20]) {
    case 0: {
        char *p = D_L05_001601AC_p + *(int *)(d + 0x10) * 128;
        qcopy(m + 0x10, p + 0x30);
        *(float *)(m + 0x48) = *(float *)(p + 0x78);
        func_L00_001FF4B0(v, p + 0x10, *(float *)(d + 0x18));
        if (*(int *)(d + 0x1C) != 0) {
            func_001F9C30(v, v, -1.0f);
            *(unsigned short *)(m + 0x34) |= 0x8000;
        }
        func_001F9BD8(m + 0x10, m + 0x10, v);
        m[0x20] = 1;
        break;
    }
    case 1:
        if (((unsigned char *)((*(int *)(d + 0x14) << 8) + (int)D_L05_00160098_p))[0xBC] != 0) {
            func_0022ED80_i(0, 0, m);
            m[0x20] = 2;
            func_001F9BC0(d);
        }
        break;
    case 2: {
        int ix = *(int *)(d + 0x10);
        float k = *(float *)(d + 0x18) * 3.0f;
        char *p = D_L05_001601AC_p + ix * 128;
        spd = 0.0f;
        func_L00_001FF4B0(v, p + 0x10, k);
        if (*(int *)(d + 0x1C) != 0) func_001F9C30(v, v, -1.0f);
        func_001F9BD8(v, p + 0x30, v);
        func_001F9BF0(v, v, m + 0x10);
        dist = func_001F9CB8(v);
        len = func_001F9CB8(d);
        func_00214D88_p(&spd, &len, dist, D_0015EE70 * 20.0f, D_0015EE70 * 30.0f, D_0015EE6C * 10.0f);
        func_L00_001FF4B0(d, v, len);
        func_001F9BD8(m + 0x10, m + 0x10, d);
        if (func_001F9CB8(v) < 0.01f) m[0x20] = 3;
        break;
    }
    case 3: {
        char *p = D_L05_001601AC_p + *(int *)(d + 0x10) * 128;
        func_00214D88_p((float *)(m + 0x18), (float *)(d + 8), *(float *)(p + 0x38) - 5.9f, D_0015EE70 * 20.0f,
                      D_0015EE70 * 40.0f, D_0015EE6C * 10.0f);
        if (*(float *)(m + 0x18) < *(float *)(p + 0x38) - 5.9f) m[0x20] = 4;
        break;
    }
    }
}
