/* NON_MATCHING func_L00_00262BC0 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: SIZE ours 552 / retail 548, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the nearest crossing of a polyline (vertices in D_L00_001B0830[idx], count at +0, 16-byte vertices from 
 *   Best candidate p9.c: same size (548), loop shape right (i=$19 and giv j=$18 via `i * 0x10 + 0x10` in one paren
 *   Would unblock: a source form where the `t[3]/u[3] != 0` zero is a hoisted constant but the `< 0`/`x > 0` zero 
 */
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BC0(void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9C08(void *, void *, void *, float);
extern char *D_L00_001B0830[];

/* Finds where a polyline crosses the y=0 plane nearest the start; returns whether it did. */
int func_L00_00262BC0(int idx, void *a, void *b, void *c) {
    float v[4], w[4], z[4], t[4], u[4];
    float best;
    float z0 = 0;
    int found = 0;
    int i;
    func_001F9BF0(v, b, a);
    v[2] = 0;
    func_L00_001FF4B0(v, v, 1.0f / func_001F9CB8(v));
    best = 1.0f;
    func_001F9BC0(w);
    w[0] = v[1];
    w[1] = -v[0];
    func_001F9BC0(z);
    z[2] = 1.0f;
    func_001F9BF0(t, D_L00_001B0830[idx] + 0x10, a);
    func_001F9EC0(t, t, v);
    for (i = 1; i < *(int *)D_L00_001B0830[idx]; i++) {
        func_001F9BF0(u, D_L00_001B0830[idx] + (i * 0x10 + 0x10), a);
        func_001F9EC0(u, u, v);
        if (t[3] != z0 || u[3] != z0) {
            if (u[1] * t[1] < 0) {
                float x = u[0] + (t[0] - u[0]) / (t[1] - u[1]) * -u[1];
                if (x > 0 && x < best) {
                    best = x;
                    found = 1;
                }
            }
        }
        qcopy(t, u);
    }
    func_001F9C08(c, a, b, best);
    return found;
}
