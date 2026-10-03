/* NON_MATCHING func_L14_002B4128 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: BYTES 6/132 (95.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Refreshes a moby's data block: if d[0x38] set, d[0x238] = func_001FA898(func_001F9878(func_002140F8(180,240)))
 *   Best p4.c (BYTES 6/132, func_001F9908 declared returning int to get the lw into $v1): only the float allocatio
 *   Wordings p4, p5, p6, p8 (local v, 6.0f+v, int temp, ternary) all compile to the same bytes: an allocator tie. 
 */
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9908(int *arg0);
extern short D_L14_0016158C;

/* Refreshes a moby's cached value from its data block and sets a float field. */
void func_L14_002B4128(char *moby) {
    char *d = *(char **)(moby + 0x78);
    float base;
    if (*(int *)(d + 0x38) != 0) {
        *(int *)(d + 0x238) = func_001FA898_r(func_001F9878(func_002140F8(180.0f, 240.0f)));
        *(int *)(d + 0x38) = 0;
    }
    func_001F9908((int *)(d + 0x238));
    base = *(float *)&D_L14_0016158C;
    *(float *)(d + 0x234) = *(int *)(d + 0x238) != 0 ? base + 6.0f : base;
}
