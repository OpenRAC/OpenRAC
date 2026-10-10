/* NON_MATCHING func_L00_0025AA20 -- src/overlays/shared/mobyutil_00258BC8.c
 * Best so far: BYTES 6/160 (96.2% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Forwards its args to func_L00_0025A8E8 after func_L00_00250800(a, b, v) fills a stack vector; incoming (void*,
 *   Only difference: in the delay slot of the first jal retail has `daddu $s4,$t1` with `daddu $a2,$sp` hoisted ea
 */
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_0025A8E8(void *, void *, int, float, float, float, int, int, int);

/* builds a vector from two ints and forwards everything to the effect spawner */
void func_L00_0025AA20(void *a, int b, int c, int d, int e, int f, float x, float unused, float y) {
    long v[2];
    func_L00_00250800(a, b, v);
    func_L00_0025A8E8(a, v, c, x, (float)c, y, d, e, f);
}
