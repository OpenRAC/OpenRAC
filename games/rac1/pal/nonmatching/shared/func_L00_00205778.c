/* NON_MATCHING func_L00_00205778 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 1/8 (87.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L00_00205778
 *   This is a fragment: the function reads $v1 without setting it. The assembly loads from 0x4E0($v1) but $v1 is n
 *   Wall: fragment (reads register never set).
 */
// Loads from offset 0x4E0
int func_L00_00205778(void *arg) {
    return *(int *)((char *)arg + 0x4E0);
}
