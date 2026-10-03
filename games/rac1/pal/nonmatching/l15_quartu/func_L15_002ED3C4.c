/* NON_MATCHING func_L15_002ED3C4 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 84 / retail 92, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern short D_L15_00160058;

int func_L15_002ED3C4(unsigned short *p, int count)
{
    unsigned char *base = *(unsigned char **)&D_L15_00160058;
    unsigned short id;
    do {
        unsigned char *moby;
        id = *p;
        moby = base + ((id & 0x7FFF) << 8);
        if (moby == 0 || moby[0x20] == 0xFE || moby[0x20] == 0xFD) {
            count++;
        }
        p++;
    } while ((short)id >= 0);
    return count;
}
