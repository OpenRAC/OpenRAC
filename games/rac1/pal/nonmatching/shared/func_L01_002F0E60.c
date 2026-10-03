/* NON_MATCHING func_L01_002F0E60 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: BYTES 17/112 (84.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int func_L00_002DCDA8(void *);

int func_L01_002F0E60(unsigned char *moby)
{
    int value = func_L00_002DCDA8(moby);
    if (value != 0) {
        if (*(short *)(moby + 0xA6) != 0x362) {
            value = 0;
        } else if (moby[0x20] == 8) {
            value = 0;
        } else {
            moby[0x20] = 0xE;
        }
    } else if (moby[0x20] == 0xE) {
        moby[0x20] = 1;
    }
    return value;
}
