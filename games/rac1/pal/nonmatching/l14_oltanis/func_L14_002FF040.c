/* NON_MATCHING func_L14_002FF040 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: BYTES 46/788 (94.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L14_001601AC_f __asm__("D_L14_001601AC") MACRO_ADDR;
extern char *D_L14_00160098_f __asm__("D_L14_00160098") MACRO_ADDR;
extern short D_L14_00162078;
extern short D_L14_0016207C;
extern short D_L14_00162080;
extern float D_0015EE6C_f __asm__("D_0015EE6C") MACRO_ADDR;
extern int func_001F9850(int);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern int func_001F9908(void *);
extern void func_L00_0028EBF0(int);
extern void func_0020D678(void *);
extern void func_L00_0025F4A8_x(void *, void *, void *, int, int, int, int, float, float, float, float, float, float, float, int, int, int, int) __asm__("func_L00_0025F4A8");
extern float func_001FA748(float, float);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern float func_L00_0025C7A8(float, float, float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L14_002FF358(char *);
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80(int, int, int);

/* Homing projectile: launches from its spawn point, steers toward its target point while spinning, and at
 * the target kills its sound, removes the linked moby and explodes. */
void func_L14_002FF040(char *m) {
    int *d = *(int **)(m + 0x78);
    float off[4];
    float dir[4];
    switch (((unsigned char *)m)[0x20]) {
    case 0: {
        int t;
        if (d[0] < 0 || d[1] < 0) {
            goto kill;
        }
        m[0x20] = 1;
        ((unsigned char *)m)[0x30] = 0xFF;
        t = *(int *)&D_L14_00162080;
        *(float *)(m + 0x18) += 1.5f;
        d[2] = -1;
        d[3] = func_001F9850(t);
        return;
    }
    case 1:
        return;
    case 2: {
        char *pos = m + 0x10;
        char *at = pos;
        float dist;
        float speed;
        func_001F9BF0(off, D_L14_001601AC_f + (d[0] << 7) + 0x30, pos);
        dist = func_001F9CB8(off);
        func_001F9908(d + 3);
        speed = *(float *)&D_L14_00162078 * D_0015EE6C_f;
        if (dist < speed || dist <= 0.0f) {
            char *o;
            if (d[2] != -1) {
                char *e = D_0013E633 + 0x1D + d[2] * 0x70;
                if (*(char **)(e + 0x88) == m && ((unsigned char *)e)[0x74] != 0) {
                    func_L00_0028EBF0(d[2]);
                }
            }
            o = D_L14_00160098_f + (d[1] << 8);
            d[2] = -1;
            if (o != 0) {
                func_0020D678(o);
            }
            qcopy(at, (char *)((d[0] << 7) + (int)D_L14_001601AC_f) + 0x30);
            qcopy(dir, m + 0xC0);
            dir[2] = 1.0f;
            func_L00_0025F4A8_x(m, dir, at, 0x14, 0xA, 0xC, 0, 0.0f, 0.0f, 10.0f, 6.0f, 9.0f, 1.0f, 30.0f, 1, 10, -1, 0);
        kill:
            func_0020D678(m);
            return;
        }
        *(float *)(m + 0x40) = func_001FA748(*(float *)(m + 0x40), 0.034906585f);
        *(float *)(m + 0x44) = func_L00_0025C7A8(*(float *)(m + 0x44), -func_L00_001FF860(func_001F9CE8(off), off[2]), *(float *)&D_L14_0016207C);
        *(float *)(m + 0x48) = func_L00_0025C7A8(*(float *)(m + 0x48), func_L00_001FF860(off[0], off[1]), *(float *)&D_L14_0016207C);
        func_001F9C30(dir, m + 0xC0, speed);
        func_001F9BD8(pos, pos, dir);
        func_L14_002FF358(m);
        if (func_L00_0028EB98(m, d[2]) == 0) {
            d[2] = func_0022ED80(1, 4, (int)m);
        }
        return;
    }
    }
}
