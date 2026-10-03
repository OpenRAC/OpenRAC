/* NON_MATCHING func_L00_002D3330 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: BYTES 12/176 (93.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int D_L00_0015F6A8;
extern int D_L00_0016C990;
extern void func_0020DAF8(void *, int);
extern void func_001F49B0(void *, void *);
extern char func_L00_002D2E60[];

void func_L00_002D3330(unsigned char *moby)
{
    unsigned char *data = *(unsigned char **)(moby + 0x78);
    if (moby[0x20] == 0) {
        moby[0x20] = 1;
        moby[0xBC] = 1;
        *(int *)(data + 0x40) = 0;
    }
    {
    int state = D_L00_0015F6A8;
    if (state == 2 &&
        (D_L00_0016C990 < 3 || D_L00_0016C990 == 4)) {
        moby[0x31] = 0;
        *(unsigned short *)(moby + 0x34) |= 1;
    } else {
        moby[0x31] = 1;
        *(unsigned short *)(moby + 0x34) &= 0xFFFE;
    }
    }
    func_0020DAF8(moby, 0);
    if (moby[0x31] != 0) {
        func_001F49B0(func_L00_002D2E60, moby);
    }
}
