/* NON_MATCHING func_L07_00310920 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 180 / retail 172, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int func_L00_0025A208(void **, int, int, int);
extern int func_L00_0025A2F0(void **, void *, int, int);

int func_L07_00310920(unsigned char *moby, int lower, int upper) {
    unsigned char *other = 0;
    if (!func_L00_0025A208((void **)&other, moby[0x21], 0, 0)) {
        if (other) {
            do {
                if (other != moby) {
                    int state = other[0x20];
                    if (state >= lower) {
                        if (upper < state) {
                        } else {
                            return 1;
                        }
                    }
                }
                if (func_L00_0025A2F0((void **)&other, other, 0, 0)) break;
            } while (other);
        }
    }
    return 0;
}
