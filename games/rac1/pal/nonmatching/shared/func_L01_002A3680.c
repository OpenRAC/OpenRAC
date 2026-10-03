/* NON_MATCHING func_L01_002A3680 -- src/overlays/shared/space_002A3680.c
 * Best so far: BYTES 6/52 (88.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char D_0013DE6E[];

void func_L01_002A3680(void)
{
    char *moby = *(char **)(D_0013DE6E + 0x2C2);
    if (moby != 0) {
        *(unsigned short *)(moby + 0x34) &= 0xFFFC;
        moby = *(char **)(D_0013DE6E + 0x2C2);
        *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
    }
}
