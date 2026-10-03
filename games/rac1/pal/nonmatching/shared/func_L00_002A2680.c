/* NON_MATCHING func_L00_002A2680 -- src/overlays/shared/vuchain_002A21A8.c
 * Best so far: BYTES 30/328 (90.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002A2680(char *a, float f): builds a 4-bit mask, testing f against two wrapped ranges per loop iterat
 *   (float pairs at a+0x38/0x40.. and a+0x28/0x30..), then if the mask is 0 tests f against a[0x18]/a[0x1C] (+1.0)
 *   Body and control flow match (p0, only 30 bytes differ, all register names): retail keeps the mask in $a1 and t
 *   bit in $v0 and ends with `daddu $v0,$a1` in the jr delay slot; ours puts the mask in $v0 and the bit in $a1.
 *   Tried declaration order, int/unsigned, do-while vs for, early `return r`, extra hidden param: same allocation.
 *   Unblock: something that raises the bit variable's allocator priority over the result, unknown.
 */
/* classifies a point's angle against four wrapped ranges and two bounds; returns a bitmask */
int func_L00_002A2680(char *a, float f) {
    int v = 1;
    int r = 0;
    int i;
    float *p = (float *)(a + 0x30);
    for (i = 1; i >= 0; i--) {
        float lo = p[4];
        float hi = p[6];
        if (lo <= hi) {
            if (lo <= f && f <= hi) r |= v;
        } else if (lo <= f || f <= hi) {
            r |= v;
        }
        lo = p[-2];
        hi = p[0];
        v <<= 1;
        if (lo <= hi) {
            if (lo <= f && f <= hi) r |= v;
        } else if (lo <= f || f <= hi) {
            r |= v;
        }
        v <<= 1;
        p++;
    }
    if (r != 0) return r;
    {
        float x = *(float *)(a + 0x18);
        float y;
        if (f < x || x + 1.0f < f) r = v;
        y = *(float *)(a + 0x1C);
        v <<= 1;
        if (f < y || y + 1.0f < f) r |= v;
    }
    return r;
}
