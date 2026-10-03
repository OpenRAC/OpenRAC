/* NON_MATCHING func_L06_003020B8 -- src/overlays/shared/vendor_002FF000.c
 * Best so far: SIZE ours 400 / retail 396, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a debris moby: class from rnd(func_002140F8(1085,1089)), copies two 16-byte vectors (arg2/arg3, first c
 *   p5.c same size (396), 95 instruction diffs, all scheduling: retail hoists the 0x4488 lui/mtc1 ($f13) before th
 *   Unblock: the order of the first statements (what the original evaluates first); the field stores of m[0x30]/0x
 */
typedef int u128 __attribute__((mode(TI)));
extern float func_002140F8(float, float);
extern int rnd_a(float) __asm__("func_001FA898");
extern char *mk_a(int) __asm__("func_0020D348");
extern float func_001F9878(float);
extern void upd_a(void *) __asm__("func_L00_00251E30");
extern float D_0015EE6C MACRO_ADDR;
extern short D_L06_00162110;

/* Spawns a debris moby with a random class and random spin. */
char *func_L06_003020B8(int unused, char *pa, char *pb) {
    float a[4];
    float b[4];
    float *va = a;
    float *vb = b;
    char *m;
    char *data;
    qcopy(a, pa);
    qcopy(b, pb);
    m = mk_a(rnd_a(func_002140F8(1085.0f, 1089.0f)));
    if (m != 0) {
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0x7E;
        m[0x31] = 1;
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * *(float *)&D_L06_00162110;
        data = *(char **)(m + 0x78);
        qcopy(m + 0x10, va);
        qcopy(data, vb);
        *(float *)(data + 0x10) = func_002140F8(-360.0f, 360.0f) * 0.017453292f * D_0015EE6C;
        *(float *)(data + 0x14) = func_002140F8(-360.0f, 360.0f) * 0.017453292f * D_0015EE6C;
        *(float *)(data + 0x18) = func_002140F8(-360.0f, 360.0f) * 0.017453292f * D_0015EE6C;
        m[0xBC] = rnd_a(func_001F9878(func_002140F8(60.0f, 120.0f)));
        upd_a(m);
    }
    return m;
}
