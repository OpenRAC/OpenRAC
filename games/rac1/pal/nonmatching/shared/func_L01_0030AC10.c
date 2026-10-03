/* NON_MATCHING func_L01_0030AC10 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 4/96 (95.8% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   Clears the parent and three child active flags. The C in p1.c compiles to 96 bytes. With literal `1` for both 
 */
void func_L01_0030AC10(L01Moby *moby) {
    char *linked = *(char **)((char *)moby + 0x24);
    L01Moby **slots = moby->data->slots;
    int i;
    moby->field94 = *(int *)(linked + 0x10);
    moby->flags &= ~1;
    moby->state = 1;
    for (i = 2; i >= 0; i--, slots++) {
        if (*slots != 0) {
            (*slots)->flags &= ~1;
            ((char *)*slots)[0x31] = 1;
        }
    }
}
