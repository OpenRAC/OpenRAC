/* NON_MATCHING func_L15_002F9FF8 -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: BYTES 10/104 (90.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L15_00167480;
extern char *D_L15_0015F050 MACRO_ADDR;
extern char D_0013E633[];
extern void func_L15_002F9D38(void *);

int func_L15_002F9FF8(char *moby)
{
    char *entry = D_L15_0015F050 + (*(short *)(moby + 0x84) << 5);
    char *other = D_L15_00167480;
    char *sub = *(char **)(entry + 0x1C);
    if (*(short *)(other + 0x86) == 0 && *(short *)(sub + 0x20) >= 0 && (unsigned char)D_0013E633[0x2EC1] == 2) {
        func_L15_002F9D38(moby);
    }
    return -1;
}
