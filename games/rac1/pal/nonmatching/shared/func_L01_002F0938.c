/* NON_MATCHING func_L01_002F0938 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 528 / retail 524, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns two particle effects (func_L01_00287F20) on a moby, each from its own set of random ranges in the level
 *   p4.c is one instruction off (SIZE 528 vs 524): everything matches except an extra `lq $ra,0x40($sp)` in ours a
 *   Key facts: v[3] must be function scope; func_L01_00287F20 declared (float *, float, float, int, int, int); mob
 */
extern short D_L01_00161A70, D_L01_00161A74, D_L01_00161A78, D_L01_00161A48, D_L01_00161A4C;
extern short D_L01_00161A54, D_L01_00161A50, D_L01_00161A40, D_L01_00161A44, D_L01_00161A60;
extern short D_L01_00161A64, D_L01_00161A6C, D_L01_00161A68, D_L01_00161A58, D_L01_00161A5C;
extern int func_002140B0(int);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern void func_L01_00287F20(float *, float, float, int, int, int);

/* Spawns up to two particle effects on the moby, each aimed by a random direction and speed. */
void func_L01_002F0938(unsigned char *moby) {
    char *d;
    float v[3];
    if (moby[0x31] != 0) {
    d = *(char **)(moby + 0x78);
    if (func_002140B0(*(int *)&D_L01_00161A70 - 1) == 0) {
        float a = func_00214158();
        float z = 0.0f;
        float b = func_002140F8(z, *(float *)&D_L01_00161A78);
        float c = func_002140F8(z, *(float *)&D_L01_00161A48) * *(float *)(d + 0x250);
        float e = func_002140F8(*(float *)&D_L01_00161A48, *(float *)&D_L01_00161A4C) * *(float *)(d + 0x250);
        int s = func_001FA898_r(func_002140F8((float)*(int *)&D_L01_00161A50, (float)*(int *)&D_L01_00161A54));
        v[0] = func_001F9F90(a) * b;
        v[1] = func_001F9FA8(a) * b;
        v[2] = z;
        func_001F9BD8(v, v, moby + 0x10);
        func_L01_00287F20(v, c, e, *(int *)&D_L01_00161A40, *(int *)&D_L01_00161A44, func_001F9850(s));
    }
    if (func_002140B0(*(int *)&D_L01_00161A74 - 1) == 0) {
        float a = func_00214158();
        float z = 0.0f;
        float b = func_002140F8(z, *(float *)&D_L01_00161A78);
        float c = func_002140F8(z, *(float *)&D_L01_00161A60) * *(float *)(d + 0x250);
        float e = func_002140F8(*(float *)&D_L01_00161A60, *(float *)&D_L01_00161A64) * *(float *)(d + 0x250);
        int s = func_001FA898_r(func_002140F8((float)*(int *)&D_L01_00161A68, (float)*(int *)&D_L01_00161A6C));
        v[0] = func_001F9F90(a) * b;
        v[1] = func_001F9FA8(a) * b;
        v[2] = z;
        func_001F9BD8(v, v, moby + 0x10);
        func_L01_00287F20(v, c, e, *(int *)&D_L01_00161A58, *(int *)&D_L01_00161A5C, func_001F9850(s));
    }
    }
}
