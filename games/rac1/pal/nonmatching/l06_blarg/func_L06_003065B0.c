/* NON_MATCHING func_L06_003065B0 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 576 / retail 564, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws four transformed copies (matrices at sp+0x80 from a table at D_L06_001FEBE0, bobbing z), then calls func
 *   Best p4.c (BYTES 42/564): everything matches except the scheduling/register choice in the struct build (retail
 */
extern void func_001FA218(float *, float *);
extern void func_001FA1C0(float *, float);
extern void func_001FA540(void *, void *, void *);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F7868(void);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L06_00216B38(int *, float *);
extern float D_L06_001FEBE0[];
extern int D_L06_001622C4;
extern int D_L06_001622C8;
extern short D_L06_00162298;
extern short D_L06_00162254;
extern short D_L06_00162250;
extern short D_L06_00162258;
extern short D_L06_0016225C;
extern short D_L06_00162270;
extern short D_L06_00162274;
extern short D_L06_00162284;
extern short D_L06_00162288;

/* draws four copies of a model transformed by a table, then spawns a part for each */
void func_L06_003065B0(char *m) {
    int s[10];
    float mat[4][16];
    float b[16];
    float a[16];
    char *data;
    int i;
    float *m0;
    int *sp;
    float *pp;
    float *tp;
    float *mp;
    data = *(char **)(m + 0x78);
    func_001FA218(a, (float *)(m + 0x40));
    func_001FA1C0(b, *(float *)&D_L06_00162298);
    func_001FA540(a, a, b);
    func_00234C98(6, func_001F4868(0xE));
    func_00234C98(0x14, 0xFF9000000260L);
    func_00234C98(8, 0);
    func_00234C98(0x42, (long)*(int *)&D_L06_00162250 | ((long)*(int *)&D_L06_00162254 << 2) | ((long)*(int *)&D_L06_00162258 << 4) | ((long)*(int *)&D_L06_0016225C << 6) | 0x8000000000L);
    func_001F7868();
    m0 = mat[0];
    sp = s;
    pp = mat[0] + 12;
    tp = D_L06_001FEBE0;
    mp = m0;
    for (i = 0; i < 4; i++) {
        func_001FA218(mp, tp);
        tp += 4;
        func_001FA540(mp, a, mp);
        mp += 16;
        func_001F9BD8(pp, pp, m + 0x10);
        pp[3] = 1.0f;
        pp[2] += (float)i * 1.7f + 0.6f;
        pp += 16;
    }
    s[0] = D_L06_001622C4;
    s[2] = (int)(data + 0xF00);
    s[1] = D_L06_001622C8;
    s[3] = *(int *)&D_L06_00162270;
    s[4] = *(int *)&D_L06_00162274;
    *(float *)&s[7] = *(float *)&D_L06_00162284;
    *(float *)&s[8] = *(float *)&D_L06_00162288;
    s[5] = 0x1E;
    s[6] = (int)data;
    pp = m0;
    for (i = 3; i >= 0; i--) {
        func_L06_00216B38(sp, pp);
        pp += 16;
    }
}
