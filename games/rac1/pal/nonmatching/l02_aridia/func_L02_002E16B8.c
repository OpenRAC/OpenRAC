/* NON_MATCHING func_L02_002E16B8 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: BYTES 6/452 (98.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_755: state 1 spawns 3 debris particles (func_L00_0026A7F8), state 2 spawns/readies child at d+0x60.
 *   p4.c: only difference is the order of two loop-setup instructions (addiu s1,sp,0x10 vs addiu s2,0,2 at +88/+8c
 *   Budget spent. Unblock: some way to reorder the loop counter init vs the &v pointer hoist.
 */
extern void func_001F9BC0(void *);
extern int func_L00_0025D6F0(void *, void *);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern void func_00213DE0(void *, int, int, int);
extern void func_L00_00251E30(void *);
extern void func_0020D678(void *);
extern short D_L02_00161D2C;
extern short D_L02_00161D30;

// UpdateMoby_755: debris burst while state 1, spawn child on state 2.
void func_L02_002E16B8(char *moby) {
    float u[4];
    float v[4];
    char *d = *(char **)(moby + 0x78);
    int i;
    int r;
    int s;
    char *c;
    switch (((unsigned char *)moby)[0x20]) {
    case 1:
        func_001F9BC0(u);
        if (func_L00_0025D6F0(moby, d) & 1) {
            moby[0x20] = 2;
        }
        i = 2;
        do {
            qcopy(v, moby + 0x10);
            v[2] += func_002140F8(0.0f, 0.5f);
            v[0] += func_002140F8(-0.15f, 0.15f);
            v[1] += func_002140F8(-0.15f, 0.15f);
            r = func_001F9850(func_L00_00258BC8(0x14, 0x28));
            s = func_L00_00258BC8(0x14, 0x28);
            func_L00_0026A7F8(v, u, *(int *)&D_L02_00161D2C, *(int *)&D_L02_00161D30, r, 0x28, s, 1);
            i--;
        } while (i >= 0);
        break;
    case 2:
        c = *(char **)(d + 0x60);
        qcopy(c + 0x10, moby + 0x10);
        c[0x20] = 0x11;
        func_00213DE0(*(char **)(d + 0x60), 5, 0, 1);
        *(int *)(*(char **)(d + 0x60) + 0x94) = *(int *)(*(char **)(*(char **)(d + 0x60) + 0x24) + 0x10);
        *(float *)(*(char **)(d + 0x60) + 0x58) = 1.0f;
        func_L00_00251E30(*(char **)(d + 0x60));
        func_0020D678(moby);
        break;
    }
}
