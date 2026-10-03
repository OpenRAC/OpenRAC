/* NON_MATCHING func_L18_002FB080 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 656 / retail 660, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Function: Veldin 2 effect drawer. Resets D_001625F0..FC when ML+0x34 matches func_001F9850(gp+1), builds basis
 *   (9EC0, 9BD8, 9BF0, 1FF4B0, 9CA0), then two 4-iteration loops (9C30 + 9EE8) into a 0x70+ byte struct passed to 
 *   No `sq $zero`: the 128-bit zero at 0xD0 is `por $2; sq $2`, which `t4 = (QVec){{0,0,0,0}}` reproduces exactly.
 *   What mattered: MACRO_ADDR on D_001625F0/F4/F8/FC and D_001625B4 (gives retail's `lui $at` stores and fresh lui
 *   ML accessed via int alias (file defines ML later, so can't redeclare). Store order of the 0x44..0x60 block via
 *   Remaining diff (SIZE 656 vs 660, p8.c): retail uses 5 saved regs: counter `li s2,3` and a second copy of &t0 (
 *   to before the first calls, `a` sits in s3; ours has only 4 (copy made late / no extra reg). Variants (i=3 hois
 *   Also second-block lui for D_001DFE50 / B4 / gp load ordering differs slightly. Runs used 13 of 16 (3 wasted on
 */
typedef struct { float v[4]; } __attribute__((aligned(16))) QVb080;
typedef struct {
    float m[16];
    unsigned int c[4];
    float f50, f54;
    int i58;
    float f5c, f60;
    int i64, i68, i6c;
    long l70, l78, l80, l88;
    QVb080 t0, t1, t2, t3, t4;
} S;
extern int func_001F9850(int);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern int D_L18_0016D2E0_w[] __asm__("D_L18_0016D2E0");
extern float D_L18_00167840[];
extern float D_L18_001625F8 MACRO_ADDR;
extern float D_L18_001625FC MACRO_ADDR;
extern float D_L18_001625F0 MACRO_ADDR;
extern float D_L18_001625F4 MACRO_ADDR;
extern int D_L18_001625B4 MACRO_ADDR;
extern float D_L18_001DFE50[];
extern short D_L18_0016254C, D_L18_00162594, D_L18_00162598, D_L18_0016259C, D_L18_001625A0;

void func_L18_002FB080(char *a) {
    S s;
    float *src;
    float *dst;
    int i;
    float *m = (float *)&s.t0;
    if (D_L18_0016D2E0_w[13] == func_001F9850(*(int *)&D_L18_0016254C + 1)) {
        D_L18_001625F8 = -45.0f;
        D_L18_001625FC = 1.0f;
        D_L18_001625F0 = 0;
        D_L18_001625F4 = 0;
    }
    i = 3;
    s.l78 = func_001F4868(11);
    s.l80 = 0xFF9000000260L;
    s.l88 = 0x8000000048L;
    s.l70 = 0;
    func_00234C98(0x4A, 0);
    func_00234C98(0x47, 0x51001);
    func_001F9EC0(&s.t3, &D_L18_001625F0, a + 0xC0);
    func_001F9BD8(&s.t3, &s.t3, a + 0x10);
    func_001F9BF0(m, &s.t3, D_L18_00167840);
    func_L00_001FF4B0(m, m, 1.0f);
    s.t4 = (QVb080){{0, 0, 0, 0}};
    s.t4.v[2] = 1.0f;
    s.t4.v[3] = 1.0f;
    func_001F9CA0(&s.t1, m, &s.t4);
    func_L00_001FF4B0(&s.t1, &s.t1, 1.0f);
    func_001F9CA0(&s.t2, &s.t1, m);
    {
        int col = *(int *)&D_L18_00162594 | (D_L18_001625B4 << 24);
        s.c[1] = col;
        s.c[2] = col;
        s.c[3] = col;
        s.i6c = 0;
        s.i68 = 0;
        s.i64 = 0;
        s.f5c = 1.0f;
        s.i58 = 0;
        s.f54 = 1.0f;
        s.c[0] = col;
        s.f50 = 1.0f;
        s.f60 = 1.0f;
    }
    src = D_L18_001DFE50;
    dst = s.m;
    for (; i >= 0; i--) {
        func_001F9C30(dst, src, *(float *)&D_L18_00162598);
        src += 4;
        func_001F9EE8(dst, dst, m);
        dst += 4;
    }
    func_L00_001FD1D8(&s, 0, 0);
    s.c[0] = s.c[1] = s.c[2] = s.c[3] = *(int *)&D_L18_001625A0 | (D_L18_001625B4 << 24);
    src = D_L18_001DFE50;
    dst = s.m;
    for (i = 3; i >= 0; i--) {
        func_001F9C30(dst, src, *(float *)&D_L18_0016259C);
        src += 4;
        func_001F9EE8(dst, dst, m);
        dst += 4;
    }
    func_L00_001FD1D8(&s, 0, 0);
}
