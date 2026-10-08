/* NON_MATCHING func_L00_00247478 -- src/overlays/shared/map_002465F8.c
 * Best so far: BYTES 4/100 (96.0% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the first occupied slot with key -1, scanning forward or reverse. p1, p2, and p4 compile to identical by
 */
extern int D_L00_00184568[];
/* finds an occupied slot with an unset key in the requested order */
int func_L00_00247478(int reverse) {
    int i = 0;
    char *base = (char *)D_L00_00184568;
    char *keys = base + 20;
    do {
        int slot = 4 - i;
        unsigned int offset;
        if (!reverse) slot = i;
        offset = (unsigned int)slot * 4;
        if (*(int *)(base + offset) && *(int *)(keys + offset) == -1) return slot;
        ++i;
    } while (i < 5);
    return -1;
}
