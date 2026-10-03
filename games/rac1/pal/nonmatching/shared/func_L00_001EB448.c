/* NON_MATCHING func_L00_001EB448 -- src/overlays/shared/stub_001EB430.c
 * Best so far: SIZE ours 92 / retail 88, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Copies a five-float record src -> dst, the first float replaced by func_001FA748(src[0], t) with t in $f12.
 *   Difference: SIZE only (ours 92, retail 88). Retail's listing ends at `jr $31` without its delay slot (`addiu $
 */
extern float func_001FA748(float, float);

/* Copies a five-float record, replacing the first float by func_001FA748(src[0], t). */
void func_L00_001EB448(float *dst, float *src, float t) {
    dst[0] = func_001FA748(src[0], t);
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
}
