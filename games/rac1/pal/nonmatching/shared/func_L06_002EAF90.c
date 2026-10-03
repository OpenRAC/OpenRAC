/* NON_MATCHING func_L06_002EAF90 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 416 / retail 428, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Pushes a moby out of walls: builds a step vector from moby+0xE0, applies to vel and position, then collision-t
 *   All p*.c compile to one fewer saved register (416 vs 428 bytes): retail keeps the scratch vector b (sp+0x10) i
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9CB8(void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_001F9BF0(void *, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_L00_00262DF0(float, void *, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern u128 D_L06_001746E0;
extern u128 D_L06_001746F0;

/* Pushes a moby out of the walls along a velocity step, then optionally notifies its owner. */
void func_L06_002EAF90(char *moby, char *vel, int flag, float s, float r) {
    float v[16];
    float *a = v;
    float *b;
    float *c = v + 8;
    float *d = v + 12;
    float *c2;
    char *p = moby + 0x10;
    int i;
    func_L00_001FF4B0(c, moby + 0xE0, s);
    func_L00_001FF4B0(d, c, -D_0015EE6C);
    func_001F9BD8(vel, vel, d);
    *(u128 *)a = *(u128 *)p;
    func_001F9BD8(a, a, c2 = c);
    func_001F9BD8(p, p, vel);
    func_001F9BD8(b = v + 4, p, c);
    if (r < func_001F9CB8(vel)) {
        if (func_L00_001EFFF0(a, b, 0, (int)moby, 0)) {
            *(u128 *)b = D_L06_001746E0;
            func_001F9BF0(p, b, c2);
        }
    }
    for (i = 0; i < 6; i++) {
        if (!func_L00_001F10E0(r, b, 4, moby)) {
            break;
        }
        *(u128 *)b = D_L06_001746F0;
        func_001F9BF0(p, b, c2);
    }
    if (flag != 0) {
        func_L00_00262DF0(0.5f, *(void **)(*(char **)(moby + 0x78) + 0x1EC), p, p);
    }
}
