/* NON_MATCHING func_L03_002965A0 -- src/overlays/l03_kerwan/vendor_00293720.c
 * Best so far: SIZE ours 268 / retail 264, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L03_002965A0(moby): UpdateMoby_1210; if flag 0x34&2 clear, initialises on state 0 (data+0x60.., func_L00_
 *   Best: p4.c (268 vs 264 bytes): data loaded before the flag test, lbu for state. Remaining: retail keeps data+0
 *   Would unblock: the source spelling that makes the two 0xFF stores share a register and flips s5/s6 allocation.
 */
extern void func_L00_00250800(void *, int, void *);
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);

// Update for a moby that tracks a position and orientation, initialising on state 0 and aiming each frame.
void func_L03_002965A0(char *moby) {
    float a[4];
    float v[4];
    char *data;
    char *p;
    char *q;
    char *d0;
    char *d1;
    data = *(char **)(moby + 0x78);
    if ((*(unsigned short *)(moby + 0x34) & 2) == 0) {
        p = moby + 0x10;
        d0 = data + 0xA0;
        d1 = data + 0xB0;
        q = moby + 0x40;
        if (((unsigned char *)moby)[0x20] == 0) {
            *(int *)(data + 0x60) = 0;
            *(short *)(data + 0x64) = 0;
            data[0x68] = 4;
            *(short *)(data + 0x7E) = 0xD;
            func_L00_00250800(moby, 0, v);
            *(float *)(data + 0xC4) = func_001F9D10(p, v);
            qcopy(d0, p);
            qcopy(d1, q);
            *(int *)(data + 0x5C) = 3;
            *(short *)(moby + 0x32) = ((unsigned char *)moby)[0x30] = 0xFF;
        }
        func_001F9BF0(a, p, d0);
        func_L00_002617B0(data + 0x20, a, d1, q);
        qcopy(d0, p);
        qcopy(d1, q);
    }
}
