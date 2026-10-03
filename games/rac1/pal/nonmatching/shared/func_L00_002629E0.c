/* NON_MATCHING func_L00_002629E0 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: SIZE ours 468 / retail 476, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Ray vs segment-table test: builds a basis from (v2-v1), transforms each table point, returns 1 on the first si
 *   Best p4.c (468 vs 476 bytes): control flow and calls right; retail shares s16 between the D pointer and the F 
 *   Would need the right locals shape for the pointers (D/F/e) so the allocator picks s16/s21.
 */
typedef int u128_2629E0 __attribute__((mode(TI)));
extern int *D_L00_001B0830[];
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BC0(void *);
extern void func_001F9EC0(void *, void *, void *);

// Tests whether a ray crosses a table of line segments and returns 1 on the first hit.
int func_L00_002629E0(int idx, void *v1, void *v2) {
    float a[4];
    float b[4];
    float c[4];
    float d[4];
    float f[4];
    int i;
    int off;
    float *p = d;
    float *e;
    func_001F9BF0(a, v2, v1);
    *(int *)&a[2] = 0;
    func_L00_001FF4B0(a, a, 1.0f / func_001F9CB8(a));
    func_001F9BC0(b);
    b[0] = a[1];
    b[1] = -a[0];
    func_001F9BC0(c);
    idx = (int)&D_L00_001B0830[idx];
    c[2] = 1.0f;
    func_001F9BF0(p, (char *)*(int **)idx + 0x10, v1);
    func_001F9EC0(p, p, a);
    e = p;
    p = f;
    off = 0x20;
    for (i = 1; i < **(int **)idx; i++) {
        func_001F9BF0(p, (char *)*(int **)idx + off, v1);
        func_001F9EC0(p, p, a);
        if (d[3] != 0.0f || f[3] != 0.0f) {
            if (f[1] * d[1] < 0.0f) {
                float t = (d[0] - f[0]) / (d[1] - f[1]) * -f[1] + f[0];
                if (t > 0.0f && t < 1.0f) return 1;
            }
        }
        qcopy(e, p);
        off += 0x10;
    }
    return 0;
}
