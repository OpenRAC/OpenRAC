/* NON_MATCHING func_L16_002E4408 -- src/overlays/shared/vendor_002A1B58.c
 * Best so far: BYTES 37/172 (78.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks a path record (count at +0, 16-byte points from +0x10): point[i].w = func_L00_001FF860(dx, dy) to the ne
 *   p3.c has retail's instruction stream (separate (n-2)*16 and (n-1)*16 via two int locals, bounds re-read from p
 *   Unblock: a wording that gives the counter i a higher allocation priority than the pointer q; allocator tie not
 */
extern float func_L00_001FF860(float, float);
void func_L16_002E4408(char *base) {
    int i = 0;
    char *p = base + 0x10;
    if (*(int *)base - 1 > 0) {
        do {
            *(float *)(p + 0xC) = func_L00_001FF860(*(float *)(p + 0x10) - *(float *)p, *(float *)(p + 0x14) - *(float *)(p + 4));
            i++;
            p += 0x10;
        } while (i < *(int *)base - 1);
    }
    if (*(int *)base >= 2) {
        int n = *(int *)base;
        *(float *)(base + ((n - 1) << 4) + 0x1C) = *(float *)(base + ((n - 2) << 4) + 0x1C);
    }
}
