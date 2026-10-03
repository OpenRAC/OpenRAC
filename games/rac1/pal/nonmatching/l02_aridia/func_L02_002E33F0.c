/* NON_MATCHING func_L02_002E33F0 -- src/overlays/l02_aridia/vendor_002E21F8.c
 * Best so far: SIZE ours 608 / retail 612, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby state machine (d = moby[0x78]; d[0x180]: 0 starts two func_001FFB38 handles when func_00215570 says so, 1
 *   Best p5.c (608 vs 612 bytes): only left is retail computing `table + (idx<<7)` twice (two addu, first +0x30 fo
 */
extern unsigned char D_0013E633[];
extern int func_00215570(void *arg0, int arg1);
extern void func_001FFDA0(int arg0, int arg1);
extern int func_001F9938(void *);
extern int func_001F9850(int);
extern void func_001F4E08(int);
extern float func_L00_001FF860(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BC0(float *);
extern float func_001FA748(float, float);
extern void func_L00_00217718(void *, void *, int, int);
extern int func_001FFB38(int, int, int, int, int, int, int);
extern void func_L00_0023B0F8(void);
extern void func_L02_0023D600(void);
extern void func_L02_0023D6E0(void);
extern char *D_L02_0016016C;
extern char *D_L02_00160058;

// State machine for a pair of effect handles on a moby: starts on trigger, spawns the effect and flags the mobys.
void func_L02_002E33F0(char *moby) {
    char *d = *(char **)(moby + 0x78);
    if (*(int *)(d + 0x180) == 1) {
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(d + 0x148)) == 0) {
            *(int *)(d + 0x180) = 0;
            func_001FFDA0(*(int *)(d + 0x160), 0);
            *(int *)(d + 0x160) = -1;
            func_001FFDA0(*(int *)(d + 0x164), 0);
            *(int *)(d + 0x164) = -1;
        } else {
            float v[4];
            float w[4];
            char *e;
            char *m;
            int i1, i2;
            float a;
            if (*(int *)(d + 0x14C) != 0) return;
            if (*(int *)(d + 0x150) != 0) return;
            if (func_001F9938(d + 0x168) == 0) return;
            *(int *)(d + 0x180) = 2;
            func_001F4E08(func_001F9850(0x10));
            func_001FFDA0(*(int *)(d + 0x160), 0);
            *(int *)(d + 0x160) = -1;
            func_001FFDA0(*(int *)(d + 0x164), 0);
            e = D_L02_0016016C + (*(int *)(d + 0x154) << 7);
            *(int *)(d + 0x164) = -1;
            qcopy(moby + 0x10, e + 0x30);
            a = func_L00_001FF860(*(float *)e, *(float *)(e + 4));
            *(float *)(moby + 0x48) = a;
            qcopy(v, moby + 0x10);
            v[0] = v[0] + func_001F9F90(a) * 1.5f;
            v[1] = v[1] + func_001F9FA8(*(float *)(moby + 0x48)) * 1.5f;
            func_001F9BC0(w);
            w[2] = func_001FA748(*(float *)(moby + 0x48), 3.1415927f);
            func_L00_00217718(v, w, 0, 1);
            i1 = *(int *)(d + 0x158);
            m = D_L02_00160058;
            i2 = *(int *)(d + 0x15C);
            *(char *)(m + (i1 << 8) + 0xBC) = 1;
            *(char *)(m + (i2 << 8) + 0xBC) = 1;
            *(short *)(d + 0x36) = 2;
            d[8] = 1;
        }
    } else if (*(int *)(d + 0x180) == 0) {
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(d + 0x148)) != 0) {
            *(int *)(d + 0x180) = 1;
            *(int *)(d + 0x164) = func_001FFB38(0x15, 0x7D0, (int)func_L00_0023B0F8, (int)func_L02_0023D600, (int)func_L02_0023D6E0, (int)(d + 0x14C), 0x64);
            *(int *)(d + 0x160) = func_001FFB38(0x17, 0x7D1, (int)func_L00_0023B0F8, (int)func_L02_0023D600, (int)func_L02_0023D6E0, (int)(d + 0x150), 7);
        }
    }
}
