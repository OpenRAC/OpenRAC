/* NON_MATCHING func_L03_002DC3E8 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 380 / retail 376, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_898: advances a path timer, interpolates between 16-byte keyframes, composes into moby pos/matrix. 
 *   Left: idx lives in $s0 in retail (shares with the &w pointer), ours puts idx in $s1 and mp/off in s2/s1; the t
 */
typedef int u128 __attribute__((mode(TI)));
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA888(int);
extern void func_0020D678(void *);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern float D_L03_00161C40;

// Moves a moby along a path of 16-byte keyframes, interpolating its position.
void func_L03_002DC3E8(char *m) {
    float a[4];
    float v[4];
    float w[4];
    char *d = *(char **)(m + 0x78);
    char *path;
    int idx;
    float t;
    *(u128 *)a = *(u128 *)(m + 0x10);
    *(float *)(d + 0x64) = *(float *)(d + 0x64) + D_L03_00161C40;
    v[0] = func_001F9F90(*(float *)(m + 0x48)) * *(float *)(d + 0x64);
    v[1] = func_001F9FA8(*(float *)(m + 0x48)) * *(float *)(d + 0x64);
    v[2] = 0;
    func_001F9BD8(v, v, *(char **)(d + 0x60) + 0x10);
    idx = func_001FA898_r(*(float *)(d + 0x64) / *(float *)(*(char **)(d + 0x60) + 0x1C));
    path = *(char **)(d + 0x60);
    t = (*(float *)(d + 0x64) - func_001FA888(idx) * *(float *)(path + 0x1C)) / *(float *)(path + 0x1C);
    path = *(char **)(d + 0x60);
    if (idx >= *(int *)path - 2) {
        func_0020D678(m);
    } else {
        func_001F9BF0(w, path + idx * 16 + 0x20, path + idx * 16 + 0x10);
        func_001F9C30(w, w, t);
        func_001F9BD8(m + 0x10, w, *(char **)(d + 0x60) + idx * 16 + 0x10);
        func_001F9BF0(v, m + 0x10, a);
        func_L00_002617B0(d + 0x20, v, m + 0x40, m + 0x40);
    }
}
