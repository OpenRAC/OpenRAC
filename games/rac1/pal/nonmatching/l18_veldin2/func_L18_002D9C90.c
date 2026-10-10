/* NON_MATCHING func_L18_002D9C90 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 7/24 (70.8% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
/* Second entry of the state check at func_L18_002D9C78: moves the moby to state 3
 * and records the two values the caller passed in the moby's data. */
int func_L18_002D9C90(char *moby, int a, int b, char *data) {
    moby[0x20] = 3;
    *(int *)(data + 0x7C) = a;
    *(int *)(data + 0x78) = b;
    return 1;
}
