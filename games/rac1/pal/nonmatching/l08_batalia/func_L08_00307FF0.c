/* NON_MATCHING func_L08_00307FF0 -- src/overlays/l08_batalia/vendor_002EAF48.c
 * Best so far: SIZE ours 120 / retail 112, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_L00_00211908(char *);

void func_L08_00307FF0(char *moby, int mode) {
    char *data = *(char **)(moby + 0x78);
    moby[0x20] = 1;
    switch (mode) {
    case -1:
        func_L00_00211908(moby);
        break;
    case 1:
        data[8] = 1;
        *(short *)(data + 0x36) = 3;
        break;
    case 0:
        data[8] = 1;
        *(short *)(data + 0x36) = 4;
        break;
    }
}
