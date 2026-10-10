/* NON_MATCHING func_L11_00310058 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 620 / retail 624, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1178: integrates angular velocity d[0x60] from two gp constants * (pi/180) * frame scale, with extr
 *   Best p1.c..p4.c (all identical output, 620 vs 624 bytes): control flow and calls match. Left: expression evalu
 */
extern char D_0013E633[];
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001FA4A0(void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern float func_001F9B88(float);
extern void func_L11_0024E240(int, int);
extern float func_001FA748(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern float D_0015EE70 MACRO_ADDR;
extern short D_L11_00161FF8;
extern short D_L11_00161FFC;
extern short D_L11_00162000;
extern short D_L11_00162004;
extern short D_L11_00162008;
extern short D_L11_0016200C;
extern short D_L11_00162010;
extern short D_L11_00162014;

// Updates a swinging/rotating moby: integrates its angular velocity, with extra tilt handling while the hero holds it.
void func_L11_00310058(char *moby) {
    float u[4];
    float w[4];
    float w2[4];
    float v3[4];
    char *d = *(char **)(moby + 0x78);
    char *g;
    float x;
    float t1, t2;
    float k = 0.017453292f;
    func_001F9C30(u, moby + 0x10, -1.0f);
    qcopy(w, moby + 0x40);
    g = (char *)D_0013E633 + 0xE1D;
    *(int *)(d + 0x5C) = 1;
    t1 = *(float *)&D_L11_0016200C * k * D_0015EE70 * (*(float *)&D_L11_00161FFC * *(float *)(moby + 0x44));
    t2 = *(float *)&D_L11_00162010 * k * D_0015EE70 * (*(float *)&D_L11_00162000 * *(float *)(d + 0x60));
    *(float *)(d + 0x60) = *(float *)(d + 0x60) + t1 + t2;
    if (*(char **)(g + 0x2FC) == moby && *(short *)(g + 0x30E) == 0) {
        {
            func_001F9BF0(v3, g + 0x80, moby + 0x10);
            func_001FA4A0(w2, moby + 0xC0);
            func_001F9EE8(v3, v3, w2);
            x = *(float *)&D_L11_00162008 * k * D_0015EE70 * (*(float *)&D_L11_00161FF8 * v3[0]);
            if (*(float *)(d + 0x60) * x < 0.0f) {
                if (*(float *)(moby + 0x44) * x < 0.0f) x = x + x;
            }
            if (x < 0.0f) {
                x = x - *(float *)&D_L11_00162004 * 0.017453292f * D_0015EE70;
            } else {
                x = x + *(float *)&D_L11_00162004 * 0.017453292f * D_0015EE70;
            }
            *(float *)(d + 0x60) = *(float *)(d + 0x60) + x;
            if (*(float *)&D_L11_00162014 * 0.017453292f < func_001F9B88(*(float *)(moby + 0x44))) {
                char *g2 = (char *)D_0013E633 + 0xE1D;
                if (*(unsigned int *)(g2 + 0x208C) < 2) func_L11_0024E240(6, 1);
            }
        }
    }
    *(float *)(moby + 0x44) = func_001FA748(*(float *)(moby + 0x44), *(float *)(d + 0x60));
    func_001F9BD8(u, u, moby + 0x10);
    func_L00_002617B0(d + 0x20, u, w, moby + 0x40);
}
