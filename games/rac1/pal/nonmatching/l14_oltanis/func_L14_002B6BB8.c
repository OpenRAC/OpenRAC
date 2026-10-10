/* NON_MATCHING func_L14_002B6BB8 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 18/712 (97.5% of the bytes match), checked 2026-10-10.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a02: emits six colored motes and sixteen radial sparks. Best p5.c BYTES 20/712; scoped second-loop coun
 *   Stopped budget 8/8. Left two packed-color shift sources (+f0/+fc) and f22/f20 constant setup around +1bc..+1cc
 */
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_001F9850(int);
extern void func_001F9C30(void *, void *, float);
extern float func_L00_00258C80(float, float);
extern void func_002156E0(void *, void *, void *, float);
extern void func_L00_0026D588(void *, void *);
extern void func_L00_002D4CE8(void *, void *, int, int);
extern float D_L14_0015F660[] MACRO_ADDR;
extern char D_L14_001D8BD0[];

/* emits six glowing particles and sixteen radial sparks around a heading */
void func_L14_002B6BB8(char *m, float *input, float angle) {
    float v[4], pos[4], axis[4], rotated[4], saved[4];
    float a = func_001FA748(angle, *(float *)(m + 0x48));
    int i;
    v[0] = func_001F9F90(a) * 0.8f;
    v[1] = func_001F9FA8(a) * 0.8f;
    v[2] = 0.0f;
    func_001F9BD8(pos, input, v);
    for (i = 5; i >= 0; i--) {
        int sign = func_002140B0(2);
        float life;
        int color;
        char *p;
        if (func_002140B0(2)) sign = -sign;
        life = func_002140F8(40000.0f, 150000.0f);
        color = func_L00_00258BC8(10, 127);
        {
            int hi = (color << 8) | 0x7F000000;
            color = color | ((color << 16) | hi);
        }
        p = func_L00_0026DEA0(pos, sign, D_L14_0015F660, color, 0.2f, 1.0f, 1.01f, life);
        if (p) {
            char *d = p + 0x20;
            *(short *)(p + 0xA) = func_001F9850(120);
            *(int *)(d + 4) = 2;
            d[0xA] = 127;
            d[0xB] = (unsigned char)p[0xA];
        }
    }
    qcopy(axis, v);
    func_001F9C30(v, v, 0.0625f);
    v[2] = func_002140F8(0.025f, 0.06f);
    qcopy(saved, pos);
    {
    int n;
    for (n = 0; n < 16; n++) {
        a = func_L00_00258C80(0.0f, 0.5f);
        func_002156E0(rotated, v, axis, (float)n * 0.6283f + a);
        pos[0] = saved[0] + func_L00_00258C80(0.0f, 0.2f);
        pos[1] = saved[1] + func_L00_00258C80(0.0f, 0.2f);
        pos[2] = saved[2] + func_L00_00258C80(0.0f, 0.2f);
        func_L00_0026D588(pos, rotated);
    }
    }
    func_L00_002D4CE8(D_L14_001D8BD0, saved, 0, 0);
}
