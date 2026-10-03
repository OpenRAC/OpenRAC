/* NON_MATCHING func_L06_002F4908 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 356 / retail 360, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L06_002F4908: when the level counter D_L06_00161D78 differs from D_L06_0015F6B0, rebuilds the list D_L06_
 *   p0.c/p1.c: everything right but SIZE 356/360. Retail keeps the `D_0013E633 + 0xE1D` base as its own register (
 *   Would unblock: a source form that stops gcc folding sym+0xE1D+0x2080 into one address for a single use (retail
 */
extern int D_L06_0015F6B0 MACRO_ADDR;
extern char *D_L06_001AC500[];
extern char *D_L06_001DB1B0[];
extern short D_L06_00161D78;
extern short D_L06_00161D74;
extern char D_0013E633[];

/* rebuilds the list of live type-0x359 mobys when the level counter changes, then runs the per-moby pass over each */
void func_L06_002F4908(void *a0, int *a1, float f) {
    int st;
    float t;
    int i;
    char *base = D_0013E633 + 0xE1D;
    if (*(int *)&D_L06_00161D78 != D_L06_0015F6B0) {
        char **p = D_L06_001AC500;
        char *m = *p;
        *(int *)&D_L06_00161D78 = D_L06_0015F6B0;
        *(int *)&D_L06_00161D74 = 0;
        while (m != 0) {
            if (*(short *)(m + 0xA6) == 0x359 && ((unsigned char *)m)[0x20] >= 2 && ((unsigned char *)m)[0x20] != 9) {
                D_L06_001DB1B0[*(int *)&D_L06_00161D74] = m;
                *(int *)&D_L06_00161D74 = *(int *)&D_L06_00161D74 + 1;
            }
            p++;
            m = *p;
        }
    }
    st = 0;
    t = 999999.0f;
    for (i = 0; i < *(int *)&D_L06_00161D74; i++) {
        func_L06_002F4818(a0, *a1, D_L06_001DB1B0[i], &st, &t, f);
    }
    func_L06_002F4818(a0, *a1, *(char **)(base + 0x2080), &st, &t, f);
    *a1 = st;
}
