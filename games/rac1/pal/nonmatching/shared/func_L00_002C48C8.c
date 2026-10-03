/* NON_MATCHING func_L00_002C48C8 -- src/overlays/shared/vendor_002C12B0.c
 * Best so far: BYTES 2/708 (99.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a projectile moby (class 0xCB or 0x76C), fills its data block, then claims the best mobys from the D_L0
 *   Best candidate p6.c: everything matches except the register for the constant 4 stored at d+0xC (ours $a0, reta
 *   Needed: for-loop as `for (i = 1; i <= list[0]; i++)`, unsigned char stores of 0xFF/0x80, float factor in a loc
 */
extern unsigned char D_0013E15A[] NOT_SDA;
extern float D_0015EE60 MACRO_ADDR;
extern int D_L00_00160098 MACRO_ADDR;
extern short D_L00_001B0BB0[];
extern char *func_0020D348(int);
extern void func_00213DE0(void *, int, int, int);
extern float func_002140F8(float, float);
extern void func_L00_00260878(void *, void *);
extern void func_L00_00251E30(void *);
extern void func_L00_0025E210(void *);
extern void func_L00_002607F8(int, short *, short);

/* spawns a projectile moby, then claims up to the five best nearby mobys */
char *func_L00_002C48C8(int a, void *pos, void *vel) {
    unsigned char *p = D_0013E15A + 0x4C6;
    int cls = 0xCB;
    char *m;
    char *d;
    float t;
    if (p[0x19] != 0) cls = 0x76C;
    m = func_0020D348(cls);
    if (m != 0) {
        *(unsigned char *)(m + 0x30) = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        d = *(char **)(m + 0x78);
        m[0x20] = 0;
        qcopy(m + 0x10, pos);
        qcopy(d + 0x10, vel);
        t = D_0015EE60 * -0.5f + 1.0f;
        *(int *)(d + 0x40) = a;
        *(short *)(d + 0x44) = 0;
        *(int *)(d + 0x54) = 0;
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * t;
        *(float *)(d + 0x48) = (func_001FA888(p[0x19]) + 1.0f) * 4.0f;
        if (*(unsigned char *)(m + 0x53) != 0) {
            func_00213DE0(m, 0, 0, func_001F9850(0));
        }
        *(int *)(m + 0x94) = 0;
        *(int *)(m + 0x98) = -1;
        *(float *)(m + 0x48) = func_002140F8(-3.1415927f, 3.1415927f);
        *(int *)(d + 0x58) = 0;
        *(int *)(d + 0x5C) = 0;
        while (D_L00_001B0BB0[0] >= 6) {
            char *best = 0;
            int bestv = 0;
            int i;
            for (i = 1; i <= D_L00_001B0BB0[0]; i++) {
                char *o = (char *)D_L00_00160098 + (D_L00_001B0BB0[i] << 8);
                if (o != 0 && (*(short *)(o + 0xA6) == 0xCB || *(short *)(o + 0xA6) == 0x76C) && (unsigned char)o[0x20] < 5) {
                    int v = *(int *)(*(char **)(o + 0x78) + 0x54);
                    if (v >= bestv) {
                        best = o;
                        bestv = v;
                    }
                }
            }
            if (best != 0) {
                func_L00_00260878(best, D_L00_001B0BB0);
                best[0x20] = 6;
            }
        }
        func_L00_00251E30(m);
        func_L00_0025E210(m);
        *(short *)(d + 0xC) = 4;
        *(short *)d = 0;
        d[8] = 0;
        d[9] = 0;
        *(unsigned char *)(d + 7) = 0x80;
        *(short *)(d + 0xE) = 0xF;
        func_L00_002607F8((int)m, D_L00_001B0BB0, 0x1F);
        {
            unsigned char *q = D_0013E633 + 0xE1D;
            if (q[0x20A5] != 0 || q[0x20AF] != 0) {
                *(unsigned short *)(m + 0x34) |= 0x41;
            }
        }
    }
    return m;
}
