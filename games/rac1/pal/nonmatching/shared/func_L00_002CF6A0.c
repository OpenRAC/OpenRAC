/* NON_MATCHING func_L00_002CF6A0 -- src/overlays/shared/vendor_002C96D0.c
 * Best so far: SIZE ours 368 / retail 360, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a class 0x1DF moby, aims it at a fixed anchor (D_0013E633+0xE1D+0x80), then finds the first empty slot 
 *   Everything matches except the slot-search loop: retail keeps `b TEST` into a loop whose test re-reads d+0x54 a
 *   Unblock: a source shape that stops cse folding the initial i=0 through the `b TEST` jump (and keeps the sym+0x
 */
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_00251E30(void *);
extern void func_L00_0025E210(void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern char D_0013E633[];

/* spawns a class 0x1DF moby aimed at the anchor point and registers it in a slot table */
void *func_L00_002CF6A0(int a0, void *pos) {
    char *m = (char *)func_0020D348_m(0x1DF);
    if (m != 0) {
        char *d;
        char *g;
        char *pp;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        m[0x20] = 2;
        d = *(char **)(m + 0x78);
        m[0xBC] = 0;
        qcopy(m + 0x10, pos);
        *(int *)(d + 0x44) = a0;
        g = D_0013E633 + 0xE1D;
        g[0x1FF6]++;
        func_L00_00251E30(m);
        func_L00_0025E210(m);
        *(float *)(d + 0x28) = func_L00_001FF860(*(float *)(m + 0x10) - *(float *)(g + 0x80), *(float *)(m + 0x14) - *(float *)(g + 0x84));
        *(float *)(d + 0x24) = func_L00_001FF860(func_001F9D48(g + 0x80, m + 0x10), *(float *)(m + 0x18) - *(float *)(g + 0x88));
        *(float *)(d + 0x20) = func_001F9D48(m + 0x10, g + 0x80);
        *(float *)(d + 0x3C) = *(float *)(g + 0x88);
        for (*(short *)(d + 0x54) = 0; *(short *)(d + 0x54) < 6; *(unsigned short *)(d + 0x54) += 1) {
            pp = (char *)(((int *)(D_0013E633 + 0xE1D)) + 0x808 + *(short *)(d + 0x54));
            if (*(int *)pp == 0) {
                *(int *)pp = (int)m;
                break;
            }
        }
        *(short *)(d + 0x56) = 100;
        *(int *)(m + 0x94) = 0;
    }
    return m;
}
