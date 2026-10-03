/* NON_MATCHING func_L01_002E1F50 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 148 / retail 140, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int D_0015EE84 MACRO_ADDR;
extern char D_0013D355[];
extern char D_0013D49D;

void func_L01_002E1F50(char *moby)
{
    if (D_0015EE84 == 1 && *(short *)(moby + 0xB2) == 0x34 &&
            *(short *)(moby + 0xA6) == 0x118 &&
            (unsigned char)moby[0x20] == 5) {
            D_0013D355[0x147] = D_0015EE84;
    }
    if (D_0015EE84 == 1 && *(short *)(moby + 0xB2) == 0x35 &&
            *(short *)(moby + 0xA6) == 0x118 &&
            (unsigned char)moby[0x20] == 5) {
            D_0013D49D = D_0015EE84;
    }
}
