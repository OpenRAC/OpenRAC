/* NON_MATCHING func_L09_00308F58 -- src/overlays/shared/vendor_002C6B30.c
 * Best so far: SIZE ours 440 / retail 448, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby 0x4EA (projectile) copying pos/rot to stack, setting fields, two func_L00_0026E940 calls.
 *   p1.c is 436 vs 448 bytes: retail keeps b address in s3 (4 saved s regs + s4=ticks), ours saves fewer; prologue
 *   Runs wasted re-running p1 for diffs; unblock by trying a pointer local for b and different qcopy order.
 */
extern void *func_0020D348(int);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern float func_002140F8(float, float);
extern float func_001F9D10(void *, void *);
extern char *func_L00_0026E940(char *, int, int, int, float);
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];

// Spawns moby 0x4EA (a projectile) with position, rotation and lifetime.
char *func_L09_00308F58(int owner, void *pos, void *rot, short ticks) {
    float a[4];
    float b[4];
    char *m;
    char *d;
    char *p;
    float x;
    int n;
    qcopy(a, pos);
    qcopy(b, rot);
    m = (char *)func_0020D348(0x4EA);
    if (m) {
        d = *(char **)(m + 0x78);
        *(int *)(d + 0x18) = owner;
        *(int *)(m + 0x40) = 0;
        *(float *)(m + 0x2C) = *(float *)(m + 0x2C) * 4.0f;
        p = m + 0x10;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        *(float *)(m + 0x44) = -func_L00_001FF860(func_001F9CE8(b), b[2]);
        *(float *)(m + 0x48) = func_L00_001FF860(b[0], b[1]);
        qcopy(p, a);
        qcopy(d, b);
        x = D_0015EE6C * 1.5707964f;
        *(float *)(d + 0x10) = func_002140F8(-x, x);
        *(int *)(d + 0x14) = 0;
        *(short *)(d + 0x1C) = ticks;
        *(short *)(d + 0x1E) = ticks;
        *(float *)(d + 0x20) = func_001F9D10(p, D_0013E633 + 0xE9D);
        *(int *)(d + 0x28) = 0;
        *(int *)(d + 0x24) = 0;
        n = ticks * 5 / 4;
        *(unsigned short *)(m + 0x34) |= 0x200;
        m[0x23] = 0x40;
        func_L00_0026E940(m, 0x2F7F4F4F, n, -1, 400000.0f);
        func_L00_0026E940(m, 0x4F7F7F7F, n, -1, 160000.0f);
        func_L00_00251E30(m);
    }
    return m;
}
