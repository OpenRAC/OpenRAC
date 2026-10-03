/* NON_MATCHING func_L00_0025F368 -- src/overlays/shared/mobyutil_00258BC8.c
 * Best so far: BYTES 2/84 (97.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Wraps an angle: r = func_L00_002001D8(&tmp, (x + pi) / 2pi); return r * 2pi - pi.
 *   Only 2 instructions differ (add.s dest $f12 vs retail $f0; div.s source $f12 vs $f0): the allocator puts the t
 *   Would need a wording that keeps the sum in a separate pseudo from the argument register; allocator tie.
 */
extern float func_L00_002001D8(void *, float);

float func_L00_0025F368(float angle)
{
    float scratch[4];
    float pi = 3.1415927f;
    float tau = 6.2831855f;
    float shifted = angle + pi;
    float normalized = shifted / tau;
    float y = func_L00_002001D8(scratch, normalized);
    return y * tau - pi;
}
