/* NON_MATCHING func_L05_0031E670 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: BYTES 15/140 (89.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *func_0020D348(int);
extern void func_L00_00251E30(void *);

char *func_L05_0031E670(char *owner)
{
    char *moby = func_0020D348(0x5EA);
    if (moby != 0) {
        char *srcvec = owner + 0x10;
        char *dstvec = moby + 0x10;
        moby[0x30] = owner[0x30];
        moby[0x31] = 1;
        *(unsigned short *)(moby + 0x32) = *(unsigned short *)(owner + 0x32);
        *(float *)(moby + 0x40) = *(float *)(owner + 0x40);
        *(float *)(moby + 0x44) = *(float *)(owner + 0x44);
        *(float *)(moby + 0x48) = *(float *)(owner + 0x48);
        *(long *)(moby + 0x38) = *(long *)(owner + 0x38);
        qcopy(dstvec, srcvec);
        func_L00_00251E30(moby);
    }
    return moby;
}
