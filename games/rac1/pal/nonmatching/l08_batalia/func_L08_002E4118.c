/* NON_MATCHING func_L08_002E4118 -- src/overlays/l08_batalia/vendor_002E0258.c
 * Best so far: SIZE ours 84 / retail 80, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L08_00160058;

void func_L08_002E4118(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char **output = (char **)(data + 0x158);
    int *ids = (int *)(data + 0x130);
    int i;
    for (i = 9; i >= 0; i--, ids++) {
        char *other = D_L08_00160058 + (*ids << 8);
        if (*(short *)(other + 0xA6) == 0x279) {
            *output++ = other;
        }
    }
}
