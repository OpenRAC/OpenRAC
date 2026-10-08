/* NON_MATCHING func_L02_002ED358 -- src/overlays/l02_aridia/vendor_002E21F8.c
 * Best so far: SIZE ours 772 / retail 776, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_2ed358 __attribute__((mode(TI)));
extern short D_L02_00161FE8, D_L02_00161FEC;
extern signed char D_L02_00161FF3;
extern char D_L02_001B2980[];
extern float D_0015EE70 MACRO_ADDR;
extern void func_0020D678(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214D28(float *p, float target, float maxstep);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern void func_001F9C30(void *, void *, float);
extern float func_002140F8(float, float);
extern int func_001FA898(float);
extern int func_001F9850(int);
extern void func_L00_00258DB0(float *, float, float);
extern int func_001F9908(int *);
extern unsigned char *func_L00_00272158(void *pos, float *vec, int s, int a, int col, int n, float x, float y, float z, float w, float pw);

/* Fireball: flies along its velocity (falling), pitched to its path, leaving two smoke puffs and a glow
 * each frame; deleted when its timer runs out (the timer is held while it is hidden). */
void func_L02_002ED358(char *m) {
    float p[4];
    float s1[4];
    float s2[4];
    float j[4];
    char *d = *(char **)(m + 0x78);
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        func_0020D678(m);
        break;
    case 1: {
        char *pos = m + 0x10;
        unsigned char *q;
        float *ps;
        float *pj;
        int i;
        func_001F9BD8(pos, pos, d);
        func_00214D28((float *)(d + 8), 0.0f, *(float *)&D_L02_00161FE8 * D_0015EE70);
        *(float *)(m + 0x44) = 1.5707964f - func_L00_001FF860(func_001F9CE8(d), *(float *)(d + 8));
        *(Q_2ed358 *)p = *(Q_2ed358 *)pos;
        func_001F9C30(s2, d, 0.5f);
        ps = s1;
        func_001F9C30(ps, d, -0.1f);
        for (i = 0; i < 2; i++) {
            int life = func_001F9850(func_001FA898(func_002140F8(20.0f, 60.0f)));
            int col = *(int *)&D_L02_00161FEC;
            q = func_L00_00272158(p, ps, life, col >> 24, col & 0xFFFFFF, 3, func_002140F8(300000.0f, 500000.0f), 10000.0f,
                                  1.0f, -0.0002f, 0.0f);
            if (q != 0) {
                char *t = D_L02_001B2980;
                q[2] = **(unsigned char **)(t + 0x5C);
                q[9] = func_001FA898(4.0f) - 0x80;
            }
            func_001F9BD8(p, p, s2);
            pj = j;
            func_L00_00258DB0(pj, 0.5f, 1.5f);
            func_001F9BD8(p, p, pj);
        }
        q = func_L00_00272158(pos, ps, func_001F9850(6), D_L02_00161FF3, *(int *)&D_L02_00161FEC & 0xFFFFFF, 3,
                              250000.0f, 5000.0f, 1.0f, -0.0004f, 0.0f);
        if (q != 0) {
            q[9] = func_001FA898(4.0f) - 0x80;
        }
        if (((unsigned char *)m)[0x31] == 0) {
            func_001F9908((int *)(d + 0x10));
        } else {
            *(int *)(d + 0x10) = func_001F9850(0x3C);
        }
        if (*(int *)(d + 0x10) == 0) {
            func_0020D678(m);
        }
        break;
    }
    }
}
