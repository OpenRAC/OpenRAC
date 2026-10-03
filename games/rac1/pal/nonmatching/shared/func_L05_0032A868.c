/* NON_MATCHING func_L05_0032A868 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: SIZE ours 364 / retail 360, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sets up a vector from the level table, copies it, then clamps p[2] to a lower bound that depends on state (5 o
 *   p3.c matches except the tail: retail duplicates the p[2]<lo compare into each lo arm (jump to a shared compare
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(float *, float *);
extern void func_L00_001FF4B0(void *, void *, float);
extern char D_0013F450[];
extern char D_0013F6E0[];
extern int D_0015EE84 MACRO_ADDR;

/* Sets up a moby's vector from the level table and clamps its height for the current state. */
void func_L05_0032A868(char *moby) {
    float v[4];
    char *d = *(char **)(moby + 0x70);
    float *p = (float *)(d + 0x80);
    *(u128 *)v = 0;
    v[2] = 0.7f;
    func_001F9EE8(v, v, D_0013F450);
    func_001F9BD8(p, D_0013F450 + 0x80, v);
    qcopy(d + 0x1D0, p);
    if (D_0015EE84 == 5) {
        float w[4];
        float lo;
        *(u128 *)w = 0;
        w[0] = 286.16f;
        w[1] = 447.82f;
        w[2] = 63.37f;
        w[3] = 1.0f;
        if (func_001F9D48(w, p) < 48.0f) {
            lo = 62.4f;
        } else {
            lo = 67.2f;
        }
        if (p[2] < lo) {
            p[2] = lo;
        }
    } else if (D_0015EE84 == 0x10) {
        float lo = 77.5f;
        if (p[2] < lo) {
            p[2] = lo;
        }
    }
    func_L00_001FF4B0(p + 8, D_0013F6E0, -1.0f);
}
