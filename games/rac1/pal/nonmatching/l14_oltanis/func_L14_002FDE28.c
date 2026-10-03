/* NON_MATCHING func_L14_002FDE28 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 516 / retail 520, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steers a moby: computes target vec via func_L00_0025E860, then springs pos/vel/rotation.
 *   Only difference: D_0015EE6C (float read via lui+lwc1 in retail at +44). MACRO_ADDR float gets lui dropped (gp-
 *   Unblock: a declaration making D_0015EE6C lui-form without perturbing schedule (p3.c otherwise 1 insn short).
 */
extern void func_L00_0025E860(int a, char *pos, char *b, char *c, int d, float f);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern float func_001F9CE8(void *);
extern char *D_L14_001B0F30[];
extern float D_0015EE6C MACRO_ADDR;
extern short D_L14_00162018;
extern short D_L14_0016201C;
extern short D_L14_00162020;
extern short D_L14_00162024;
extern short D_L14_00162028;

// Steers a moby toward a target position and updates its velocity/rotation springs.
void func_L14_002FDE28(char *m) {
    char *d = *(char **)(m + 0x78);
    float v[3];
    float t[3];
    float a, b, c;
    func_L00_0025E860((int)D_L14_001B0F30[*(int *)(d + 0x68)], (char *)v, d + 0x60, d + 0x64, 1, (*(float *)&D_L14_00162018) * D_0015EE6C);
    func_L00_0025C918((float *)(m + 0x10), (float *)(d + 0x70), v[0], (*(float *)&D_L14_0016201C), (*(float *)&D_L14_00162020), 0.0f);
    func_L00_0025C918((float *)(m + 0x14), (float *)(d + 0x74), v[1], (*(float *)&D_L14_0016201C), (*(float *)&D_L14_00162020), 0.0f);
    func_L00_0025C918((float *)(m + 0x18), (float *)(d + 0x78), v[2], (*(float *)&D_L14_0016201C), (*(float *)&D_L14_00162020), 0.0f);
    func_001F9BF0(t, v, m + 0x10);
    if (func_001F9B88(t[0]) > 0.01f) {
        if (func_001F9B88(t[1]) > 0.01f) {
            a = func_L00_001FF860(t[0], t[1]);
            b = func_001FA790(*(float *)(m + 0x48), a);
            func_L00_002592B0(m, (float *)(d + 0x88), a, (*(float *)&D_L14_0016201C), (*(float *)&D_L14_00162020), 0.0f);
            c = func_L00_001FF860(func_001F9CE8(t), t[2]);
            func_L00_0025C918((float *)(m + 0x44), (float *)(d + 0x84), -c, (*(float *)&D_L14_0016201C), (*(float *)&D_L14_00162020), 0.0f);
            b = b / (*(float *)&D_L14_00162028);
            if (b > 1.0f) b = 1.0f;
            if (b < -1.0f) b = -1.0f;
            func_L00_0025C918((float *)(m + 0x40), (float *)(d + 0x80), b * (*(float *)&D_L14_00162024), 0.003f, (*(float *)&D_L14_00162020), 0.0f);
        }
    }
}
