/* NON_MATCHING func_L00_0026C0D0 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: SIZE ours 716 / retail 720, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0026C0D0 (PartType12Spawn): allocates a particle, copies pos/vel, fills it from one of two gp-address
 *   Best is p8.c (BYTES 53/720, sizes equal): only the order inside the pre-call block of each arm differs (retail
 *   Found: flags is unsigned char, constants as locals one/zero (f21/f20 order), cvt args in locals fb,fa, no f4 l
 */
extern char *func_L00_00268760(int, int);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern int func_002140B0(int);
extern float func_L00_00258C80(float lo, float hi);
extern unsigned func_L00_0025D140(unsigned c, int mask);
extern unsigned char *D_L00_001B2460;
extern unsigned char *D_L00_001B2430;
extern unsigned char D_0013E15A[] NOT_SDA;
extern short D_L00_00160298;
extern short D_L00_001602A4;
extern short D_L00_001602A0;
extern short D_L00_00160288;
extern short D_L00_0016028C;
extern short D_L00_00160290;
extern short D_L00_00160294;
extern short D_L00_00160284;
extern short D_L00_00160278;
extern short D_L00_00160280;
extern short D_L00_00160268;
extern short D_L00_0016026C;
extern short D_L00_00160270;
extern short D_L00_00160274;

// Spawns a type 12 particle at pos with velocity vel and the given flags.
char *func_L00_0026C0D0(void *pos, void *vel, unsigned char flags, float size) {
    char *p = func_L00_00268760(12, 0);
    if (p != 0) {
        char *q = p + 0x20;
        unsigned char *g;
        int f4;
        qcopy(p + 0x10, pos);
        qcopy(q, vel);
        if (flags & 1) {
            float one = 1.0f;
            float zero = 0.0f;
            *(float *)(p + 0xC) = *(float *)&D_L00_00160298 * 210000.0f;
            f4 = flags & 4;
            *(int *)(p + 4) = *(int *)&D_L00_00160288;
            *(short *)(p + 0xA) = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L00_001602A0, (float)*(int *)&D_L00_001602A4)));
            *(int *)(q + 0x14) = func_001FA8A8(*(int *)&D_L00_00160288, *(int *)&D_L00_0016028C, func_002140F8(zero, one));
            *(int *)(q + 0x18) = func_001FA8A8(*(int *)&D_L00_00160290, *(int *)&D_L00_00160294, func_002140F8(zero, one));
            p[2] = *D_L00_001B2460;
            p[3] = 0x48;
        } else {
            int v = 0;
            float one = 1.0f;
            float zero = 0.0f;
            *(float *)(p + 0xC) = *(float *)&D_L00_00160278 * 210000.0f;
            *(int *)(p + 4) = *(int *)&D_L00_00160268;
            *(short *)(p + 0xA) = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L00_00160280, (float)*(int *)&D_L00_00160284)));
            *(int *)(q + 0x14) = func_001FA8A8(*(int *)&D_L00_00160268, *(int *)&D_L00_0016026C, func_002140F8(zero, one));
            *(int *)(q + 0x18) = func_001FA8A8(*(int *)&D_L00_00160270, *(int *)&D_L00_00160274, func_002140F8(zero, one));
            if ((f4 = flags & 4) != 0) {
                v = 8;
                if (D_0013E15A[0x4D6] == 0) {
                    v = 0;
                }
            }
            p[2] = D_L00_001B2430[func_002140B0(8) + v];
            p[3] = 0x44;
        }
        p[9] = func_001FA898_r(4.0f) + 0x40;
        p[1] = 0;
        p[8] = func_002140B0(0xFF);
        q[0x1E] = func_001FA898_r(func_L00_00258C80(1.0f, 4.0f));
        *(float *)(q + 0x10) = size;
        q[0x1F] = flags;
        if (f4) {
            g = D_0013E15A + 0x4C6;
            *(short *)(p + 0xA) = *(short *)(p + 0xA) * (g[0x10] + 2) / 2;
            func_L00_0025D140(*(unsigned *)(p + 4), g[0x10]);
            func_L00_0025D140(*(unsigned *)(q + 0x14), g[0x10]);
            func_L00_0025D140(*(unsigned *)(q + 0x18), g[0x10]);
        }
        q[0x1D] = p[0xA];
    }
    return p;
}
