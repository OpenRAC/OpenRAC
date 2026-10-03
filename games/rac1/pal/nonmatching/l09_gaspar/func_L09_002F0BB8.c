/* NON_MATCHING func_L09_002F0BB8 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: SIZE ours 320 / retail 324, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby (base+0x140), seeds three random floats, copies two stack vectors, two scaled angles via func_0021
 */
extern char *func_0020D348(int);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;
typedef int u128_2F0BB8 __attribute__((mode(TI)));

// Spawns a moby from class 0x140 + base, seeds random rotation and velocity data.
char *func_L09_002F0BB8(char *owner, void *pos, void *vec, int base, float f) {
    u128_2F0BB8 a = *(u128_2F0BB8 *)pos;
    u128_2F0BB8 b = *(u128_2F0BB8 *)vec;
    void *pa = &a;
    void *pb = &b;
    char *m = func_0020D348(base + 0x140);
    if (m != 0) {
        char *d = *(char **)(m + 0x78);
        *(char **)(d + 0x18) = owner;
        *(unsigned short *)(m + 0x32) = 0x7F;
        m[0x31] = 1;
        *(unsigned char *)(m + 0x30) = 0x7F;
        *(long *)(m + 0x38) = *(long *)(owner + 0x38);
        *(float *)(m + 0x40) = func_00214158();
        *(float *)(m + 0x44) = func_00214158();
        *(float *)(m + 0x48) = func_00214158();
        qcopy(m + 0x10, pa);
        qcopy(d, pb);
        {
            float x;
            x = D_0015EE6C * 9.0672f;
            *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * f;
            *(float *)(d + 0x10) = func_002140F8(-x, x);
        }
        {
            float y = D_0015EE6C * 6.2831855f;
            *(float *)(d + 0x14) = func_002140F8(-y, y);
        }
        *(int *)(d + 0x1C) = func_001F9850(func_L00_00258BC8(0x5A, 0x6E));
        func_L00_00251E30(m);
    }
    return m;
}
