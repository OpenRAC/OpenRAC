/* NON_MATCHING func_L00_002A6A38 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 1596 / retail 1600, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002A6A38: UpdateMoby_11 (pulsing beam moby, states 0-3): advances a shared phase D_L00_0016146C, colo
 *   Best candidate p5.c: SIZE 1596 vs retail 1600 (everything else matches by reading the asm: switch, movn, state
 *   Remaining difference: in the state-2 tail, retail does `lui $2; daddu $16,$2,$0; addiu $2,$2,%lo(D_0013E633+0x
 */
extern float func_001F9FA8(float);
extern void *func_0020D348(int oClass);
extern void func_0020D960(char *, int, void *);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F49B0(void (*)(void), void *);
extern void func_L00_001FFED8(void *, int, float);
extern float func_001FA748(float, float);
extern void func_L00_00251E30(void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern int func_00215F80(int, int);
extern void func_L00_0029BA20(void *);
extern void func_L00_002A62D0(void);
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_L00_0016146C;
extern short D_L00_00161464;
extern short D_L00_00161468;
extern char D_0013E633[];
extern char D_0013A5E0[];

// Per-frame update of a pulsing light-beam moby: spawns a child, fades it with distance to the player, draws it and may trigger the player's grab.
void func_L00_002A6A38(char *moby) {
    char *p = *(char **)(moby + 0x78);
    char *c;
    int i;
    float f;

    *(float *)&D_L00_0016146C += 0.05f;
    if (3.14f < *(float *)&D_L00_0016146C) *(float *)&D_L00_0016146C = -3.14f;
    f = func_001F9FA8(*(float *)&D_L00_0016146C);
    *(int *)(moby + 0x90) = (((int)(f * 48.0f) + 0x60) * 0x10101) | 0x80000000;

    switch (*(unsigned char *)(moby + 0x20)) {
    case 0:
        *(unsigned char *)(moby + 0x20) = 1;
        *(int *)(p + 0) = 0;
        *(int *)(p + 4) = 0;
        *(char **)(p + 0xC) = func_0020D348(0x477);
        *(unsigned short *)(*(char **)(p + 0xC) + 0x34) |= 2;
        *(short *)(*(char **)(p + 0xC) + 0x32) = 0x40;
        *(float *)(*(char **)(p + 0xC) + 0x10) = *(float *)(moby + 0x10);
        *(float *)(*(char **)(p + 0xC) + 0x14) = *(float *)(moby + 0x14);
        *(float *)(*(char **)(p + 0xC) + 0x18) = *(float *)(moby + 0x18) + 2.95f;
        c = *(char **)(p + 0xC);
        if (*(unsigned char *)(*(char **)(c + 0x24) + 6) != 0) *(unsigned char *)(c + 0x73) = 0x20;
        *(int *)(p + 0x90) = 0;
        for (i = 0; i < 2; i++) {
            func_0020D960(*(char **)(p + 0xC), i, p + 0x10 + i * 0x40);
        }
        func_L00_00251E30(*(char **)(p + 0xC));
        break;
    case 1:
        if (*(float *)(p + 0x90) > 0.0f) *(float *)(p + 0x90) -= 0.1f;
        if (*(float *)(p + 0x90) <= 0.0f) *(unsigned short *)(*(char **)(p + 0xC) + 0x34) |= 1;
        if ((D_L00_0015F6B0 & 7) == 0) {
            char *g = D_0013E633 + 0xE9D;
            if (func_001F9D48(moby + 0x10, g) <= 16.0f) {
                if (func_001F9B88(*(float *)(moby + 0x18) - *(float *)(g + 8)) <= 8.0f) {
                    *(unsigned char *)(moby + 0x20) = 2;
                    if (*(unsigned char *)(moby + 0x53) != 1) func_00213DE0(moby, 1, 0, 10);
                    *(unsigned short *)(*(char **)(p + 0xC) + 0x34) &= 0xFFFE;
                }
            }
        }
        func_001F49B0(func_L00_002A62D0, moby);
        func_L00_001FFED8(p + 0x20, 2, *(float *)&D_L00_00161464);
        func_L00_001FFED8(p + 0x60, 2, *(float *)&D_L00_00161468);
        *(float *)&D_L00_00161464 = func_001FA748(*(float *)&D_L00_00161464, 0.01f);
        *(float *)&D_L00_00161468 = func_001FA748(*(float *)&D_L00_00161468, -0.02f);
        *(float *)(*(char **)(p + 0xC) + 0x2C) = *(float *)(p + 0x90) * *(float *)(*(char **)(*(char **)(p + 0xC) + 0x24) + 0x24) * 2.5f;
        func_L00_00251E30(*(char **)(p + 0xC));
        break;
    case 2: {
        float dist;
        float dz;
        char *g;
        int a;
        int b;
        int r;
        int *h;
        char *h2;
        float ang;
        if (*(float *)(p + 0x90) < 1.0f) *(float *)(p + 0x90) += 0.1f;
        g = D_0013E633 + 0xE9D;
        dist = func_001F9D48(moby + 0x10, g);
        dz = func_001F9B88(*(float *)(moby + 0x18) - *(float *)(g + 8));
        if ((D_L00_0015F6B0 & 7) == 0) {
            if (18.0f < dist || 10.0f < dz) {
                *(unsigned char *)(moby + 0x20) = 1;
                if (*(unsigned char *)(moby + 0x53) != 0) func_00213DE0(moby, 0, 0, 10);
                return;
            }
        }
        func_001F49B0(func_L00_002A62D0, moby);
        a = 1;
        b = 1;
        func_L00_001FFED8(p + 0x20, 2, *(float *)&D_L00_00161464);
        func_L00_001FFED8(p + 0x60, 2, *(float *)&D_L00_00161468);
        *(float *)&D_L00_00161464 = func_001FA748(*(float *)&D_L00_00161464, 0.01f);
        *(float *)&D_L00_00161468 = func_001FA748(*(float *)&D_L00_00161468, -0.02f);
        *(float *)(*(char **)(p + 0xC) + 0x2C) = *(float *)(p + 0x90) * *(float *)(*(char **)(*(char **)(p + 0xC) + 0x24) + 0x24) * 2.5f;
        func_L00_00251E30(*(char **)(p + 0xC));
        h = (int *)(D_0013E633 + 0xE1D);
        if (h[0x208C / 4] != 0 && h[0x2084 / 4] != 3 && h[0x208C / 4] != 1) {
            a = 0;
            b = 0;
        }
        h = (int *)(D_0013E633 + 0xE1D);
        if (h[0x2084 / 4] == 0x1D || h[0x2084 / 4] == 0x32) {
            a = 0;
            b = 0;
        }
        h = (int *)(D_0013E633 + 0xE1D);
        if (((unsigned char *)h)[0x20A4] != 0) {
            a = 0;
            b = 0;
        }
        if (*(unsigned char *)(moby + 0x52) != 1) a = 0;
        if (4.0f < dist || 2.0f < dz) {
            a = 0;
            b = 0;
        }
        h2 = D_0013E633 + 0xE1D;
        ang = func_L00_001FF860(*(float *)(moby + 0x10) - *(float *)(h2 + 0x80), *(float *)(moby + 0x14) - *(float *)(h2 + 0x84));
        if (1.5707964f < func_001FA850(*(float *)(h2 + 0x98), ang)) {
            a = 0;
            b = 0;
        }
        r = 0;
        if (b != 0) r = func_00215F80(1, 0x53E8) != 0;
        if (*(int *)(D_0013A5E0 + 0x2604) & 0x10) {
            if (a != 0 && r != 0) {
                func_L00_0029BA20(moby);
                *(unsigned char *)(moby + 0x20) = 3;
            }
        }
        break;
    }
    case 3:
        c = *(char **)(p + 0xC);
        if (c != 0) {
            *(int *)(p + 0x90) = 0;
            *(float *)(c + 0x2C) = *(float *)(*(char **)(c + 0x24) + 0x24) * *(float *)(p + 0x90) * 2.5f;
            func_L00_00251E30(*(char **)(p + 0xC));
        }
        break;
    }
}
