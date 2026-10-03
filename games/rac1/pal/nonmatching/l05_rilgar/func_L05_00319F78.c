/* NON_MATCHING func_L05_00319F78 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: SIZE ours 296 / retail 300, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   SetWaterPlane / UpdateMoby_982: state 0 initialises the moby; state 1 latches the camera plane (qcopy to D_L05
 *   Best: p6.c (296 vs 300 bytes; switch on state, `if (func_00215570(..)) {B} else {D=0}`, D_L05_0016139C/D_L05_0
 *   Would need a reason the lui is not eligible for the slot in retail (source order swap of qcopy/store did not m
 */
extern char D_0013E633[];
extern int func_00215570(void *, int);
extern float func_001F9FA8(float);
extern int D_L05_0016139C MACRO_ADDR;
extern char D_L05_001613A0[];
extern int D_L05_0015F6B0 MACRO_ADDR;
extern float D_L05_001613AC MACRO_ADDR;
extern float D_L05_001613A8 MACRO_ADDR;

/* update a water-plane moby: init, then latch the plane from the camera volume */
void func_L05_00319F78(char *moby) {
    int st = ((unsigned char *)moby)[0x20];
    char *data = *(char **)(moby + 0x78);
    switch (st) {
    case 0:
        moby[0x20] = 1;
        moby[0x30] = 0xFF;
        *(unsigned short *)(moby + 0x34) |= 1;
        *(int *)(moby + 0x94) = 0;
        break;
    case 1: {
        char *p = D_0013E633 + 0xE9D;
        if (func_00215570(p, *(int *)data) != 0) {
            qcopy(D_L05_001613A0, p);
            D_L05_0016139C = st;
            *(short *)(p + 0x15E) = 5;
            D_L05_001613AC = 48.0f;
            D_L05_001613A8 = func_001F9FA8((float)(D_L05_0015F6B0 % 360) * 0.017453292f - 3.14f) * 0.25f + 59.5f;
        } else {
            D_L05_0016139C = 0;
        }
        break;
    }
    }
}
