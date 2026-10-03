/* NON_MATCHING func_L00_002E9D78 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: BYTES 8/76 (89.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_001F9BF0(float *, float *, float *);
extern float func_L00_002E9B60(char *, float *, float);

void func_L00_002E9D78(char *moby, int arg, float f)
{
    float scratch[4];
    char *data = *(char **)(moby + 0x70);
    func_L00_002E9B60(moby, (func_001F9BF0(scratch, (float *)arg, (float *)(data + 0x40)), scratch), f);
}
