/* NON_MATCHING func_L06_002EBA10 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 192 / retail 196, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Toggles a moby (state byte 0x20) into state 0x10 (saving old state at 0xBC) when func_L00_002DCDA8 is nonzero 
 *   Best try (p7, 192 vs 196): everything matches except retail loads the state byte into $2, copies it to $4 (dad
 */
extern int func_L00_002DCDA8(void *);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);

// Toggles a moby between its normal states and state 0x10, saving the old state.
int func_L06_002EBA10(unsigned char *moby) {
    int r = func_L00_002DCDA8(moby);
    if (r != 0) {
        int s = moby[0x20];
        int d = s - 8;
        if ((unsigned)d < 2 || (unsigned char)s == 0xC || (unsigned char)s == 0xD || (unsigned char)s == 0x10) {
            if (s != 0x10) {
                moby[0x20] = 0x10;
                moby[0xBC] = s;
            }
        } else {
            r = 0;
        }
    } else if (moby[0x20] == 0x10) {
        moby[0x20] = 8;
        if (moby[0x53] != 0) {
            func_00213DE0(moby, 0, 0, func_001F9850(10));
        }
    }
    return r;
}
