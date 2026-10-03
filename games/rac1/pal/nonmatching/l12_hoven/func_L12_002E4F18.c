/* NON_MATCHING func_L12_002E4F18 -- src/overlays/l12_hoven/vendor_002C0310.c
 * Best so far: SIZE ours 532 / retail 540, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steers a moby: speed from |target - pos| and |vel|, accelerates/decelerates with clamps, then eases angles via
 *   Best is p4.c: same instruction count and structure; remaining differences are only f0/f1 register choice and t
 *   Operand order of the `d4 * D_0015EE70` products (p2-p4) moved the loads; an untried h-before-g order (p5) was 
 */
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001FA850(float, float);
extern void func_00214D28(void *, float, float);
extern float func_L00_0025CCF0(void *, void *, int, float, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern short D_L12_00161998;

/* steers a moby toward a target direction with clamped angular rates */
void func_L12_002E4F18(char *a, float *b, char *c, int unused, int flag) {
    float v[4];
    char *d = *(char **)(a + 0x78);
    float len, f, g, h;
    char *p;
    func_001F9BF0(v, c, a + 0x10);
    len = func_001F9CB8(v);
    f = func_001F9CB8(d + 0x40);
    if (flag) {
        if (f == 0.0f) f = D_0015EE6C * 0.01f;
        g = D_0015EE70 * *(float *)(d + 0xD4);
        h = (f * f) / (len + len);
        if (h > g) {
            f = f - g;
        } else if (f < *(float *)(d + 0xD0) * D_0015EE6C) {
            f = f + g;
        }
    } else {
        h = *(float *)(d + 0xD0) * D_0015EE6C;
        if (f < h) {
            f = f + *(float *)(d + 0xD4) * D_0015EE70;
        } else if (h < f) {
            f = f - D_0015EE70 * *(float *)(d + 0xD4);
        }
    }
    p = d + 0x40;
    func_L00_001FF4B0(p, v, f);
    func_00214D28(a + 0x40, func_001FA850(b[0], *(float *)(a + 0x40)) / *(float *)&D_L12_00161998, b[0]);
    func_00214D28(a + 0x44, func_001FA850(b[1], *(float *)(a + 0x44)) / *(float *)&D_L12_00161998, b[1]);
    func_L00_0025CCF0(a + 0x48, d + 0xF8, 0, b[2], D_0015EE64 * 0.005f, D_0015EE64 * 0.3f, D_0015EE6C * 1.9188f);
    func_001F9BD8(a + 0x10, a + 0x10, p);
}
