/* NON_MATCHING func_L06_002EBBF8 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: BYTES 5/56 (91.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int func_L00_002DD2D0(void *);

void func_L06_002EBBF8(char *moby)
{
    unsigned int old;
    func_L00_002DD2D0(moby);
    old = ((unsigned char *)moby)[0x20];
    if (old != 0x10) {
        moby[0xBC] = old;
        moby[0x20] = 0x10;
    }
}
