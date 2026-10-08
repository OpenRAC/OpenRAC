/* NON_MATCHING func_L03_002DD6D8 -- src/overlays/shared/vendor_00292AC0.c
 * Best so far: BYTES 8/112 (92.9% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L03_002DD6D8 (UpdateMoby_915): calls func_001F9D10(m+0x10, hero blob+0x80), computes atan-like func_L00_0
 *   Best is p1.c (16 -> 8 bytes differ): an unused `float pad[12];` local reproduces the 0x60 frame; `g = D_0013E6
 *   Remaining: retail loads m.x into $f0 first and hero.x into $f12 (sub.s $f12,$f12,$f0; sub.s $f13,$f1,$f13); ou
 *   mini7 p9/p11/p12: three distinct arithmetic/compound-assignment/pointer wordings give identical 16-byte differ
 *   Actual hero symbol D_0013F450 replaces unrelated-symbol addend. Without artificial unused padding, frame is 0x
 *   Would need the real reason for the retail local frame and another FP allocator decision.
 */
extern unsigned char D_0013F450[];
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern void func_0020D678(void *);

/* Steers the moby toward the hero's position, then deletes it. */
void func_L03_002DD6D8(char *m) {
    float pad[12];
    char *g = (char *)D_0013F450 + 0x80;
    float r;
    func_001F9D10(m + 0x10, g);
    g -= 0x80;
    r = func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(m + 0x10), *(float *)(g + 0x84) - *(float *)(m + 0x14));
    func_001FA850(r, *(float *)(m + 0x48));
    func_0020D678(m);
}
