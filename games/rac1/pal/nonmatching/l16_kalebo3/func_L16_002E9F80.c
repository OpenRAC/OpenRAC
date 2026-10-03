/* NON_MATCHING func_L16_002E9F80 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 144 / retail 140, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern short *D_L16_001ABFC0[];
extern char *D_L16_00160098 MACRO_ADDR;
extern int func_L16_002C5A08(void *);

char *func_L16_002E9F80(int idx)
{
    short *p = D_L16_001ABFC0[idx];
    if (p != 0) {
        do {
            char *moby = D_L16_00160098 + ((*(unsigned short *)p & 0x7FFF) << 8);
            if (*(short *)(moby + 0xA6) == 0x10E && func_L16_002C5A08(moby) == 0) {
                return moby;
            }
        } while (*p++ >= 0);
    }
    return 0;
}
