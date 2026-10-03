/* NON_MATCHING func_L00_00299E70 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 736 / retail 740, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00299E70: level teardown/reset: clamps a state halfword, sets flags, deletes the mobys of a table (de
 *   Best is p4.c (BYTES 44/740, sizes equal). Left: the delete loop's register shape (retail: ld in $s1, copy in $
 *   Tried: local copy, global re-referenced in the loop (gives the copy but cond folds into biv), explicit pointer
 */
extern void func_002348B8(void);
extern void func_001F4E08(int);
extern void func_L00_00299108(void);
extern void func_001F3140(void);
extern void func_0020D678(void *);
extern void func_L00_00222B80(int, int);
extern float func_00214358(void *, int, float);
extern float func_001F9B88(float);
extern void func_L00_00233950(void);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_00217AE8(int, int, int);
extern int func_001F9850(int);
extern void func_L00_002666C8(int v);
extern unsigned char D_0014171B[] NOT_SDA;
extern char D_0013E633[];
extern char D_L00_0016C960[];
extern int D_L00_0015F6BC MACRO_ADDR;
extern float D_L00_0016CBF0;
extern int D_L00_00167114 NOT_SDA;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_0015F4FC MACRO_ADDR;
extern char *D_L00_0016009C MACRO_ADDR;
extern char D_L00_0016C970[];
extern int D_L00_00179200[];
extern char D_L00_001BA070[] NOT_SDA;

// Tears down the level's objects and resets the per-level state before a reload.
void func_L00_00299E70(void) {
    char *h = (char *)D_0014171B + 0x100B5;
    char *ld;
    char *m;
    char *p;
    char *b;
    char *x;
    char *ld2;
    char **pp;
    int i;
    float t;
    if ((unsigned short)(*(unsigned short *)(h + 0x5A) - 6) >= 2) {
        *(short *)(h + 0x5A) = 5;
    }
    D_L00_0015F6BC = 1;
    func_002348B8();
    ld = D_L00_0016C960;
    func_001F4E08(12);
    func_L00_00299108();
    D_L00_00167114 = *(unsigned char *)(ld + 0x4B);
    D_L00_0016CBF0 = 0.63f;
    D_L00_0015F6A8 = 0;
    D_L00_0015F4FC = 0;
    func_001F3140();
    pp = (char **)(ld + 0x178);
    for (i = 0; i < *(short *)(D_L00_0016C960 + 0x44); i++, pp++) {
        char *mo = *pp;
        if (mo != 0) {
            (*(unsigned char **)(mo + 0x24))[0xC]--;
            ((int *)(*(unsigned char **)(mo + 0x24) + 0x48))[(*(unsigned char **)(mo + 0x24))[0xC]] = 0;
            func_0020D678(mo);
        }
    }
    for (m = D_L00_0016009C; (unsigned char)m[0x20] != 0xFF; m += 0x100) {
        if (((unsigned char)m[0x20] & 0x80) == 0) {
            short k = *(short *)(m + 0xA6);
            if (k == 0x4A || k == 0xCB) {
                *(unsigned short *)(m + 0x34) &= 0xFF7F;
            }
        }
    }
    func_L00_00222B80(0, 1);
    p = (char *)D_0013E633 + 0xE9D;
    t = func_00214358(p, 0, 0.5f);
    if (t > 2.0f) {
        p -= 0x80;
        if (func_001F9B88(*(float *)(p + 0x88) - t) < 4.5f) {
            *(float *)(p + 0x88) = t;
        }
    }
    b = D_0013E633 + 0xE1D;
    b[0x20A5] = 0;
    func_L00_00233950();
    ld2 = D_L00_0016C960;
    if (*(unsigned char *)(ld2 + 0x4A) != 0) {
        t = func_00214358(ld2 + 0x10, 0, 0.5f);
        if (2.0f < t) {
            if (func_001F9B88(*(float *)(ld2 + 0x18) - t) < 2.0f) {
                *(float *)(ld2 + 0x18) = t;
            }
        }
        func_L00_00217718(D_L00_0016C970, D_L00_0016C970 + 0x10, 0, 1);
    }
    if (D_L00_00179200[2] != 0) {
        *(unsigned short *)(D_L00_00179200[2] + 0x34) &= 0xFFFE;
    }
    x = D_L00_001BA070;
    if (*(int *)(x + 0xEC) != 0) {
        *(unsigned short *)(*(int *)(x + 0xEC) + 0x34) &= 0xFFFE;
        *(int *)(x + 0xEC) = 0;
    }
    if (D_L00_00179200[2] != 0) {
        int a = D_L00_00179200[2];
        int b = D_L00_00179200[3];
        D_L00_00179200[2] = 0;
        D_L00_00179200[3] = 0;
        func_00217AE8(a, b, 1);
    }
    func_L00_002666C8(func_001F9850(30));
}
