/* NON_MATCHING func_L18_002FABE0 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 784 / retail 792, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   MACRO_ADDR the prologue/ML hoisting matches retail exactly, BUT tools/ps2eeas_nops.py refuses ("object has 0 m
 *   source `li.s $f0,-0.15 ; $L5: s.s $f0,D_1625E8` counts as an li.s read site, while in the object a macro `lui 
 *   That is a tool limitation (macro store after a label following li.s), not a code problem. p4.c = that variant 
 *   p5/p6 leave D_1625E8 plain (non-MACRO) so the tool passes: that costs 4 insns (lui in bne delay slot, no nop) 
 *   Remaining diffs in p6: (1) the E8 lui $at / bne nop above; (2) addiu $s2,$sp,0x90 computed before the call to 
 *   in retail (ours later) and addiu $s0,-0x210 in its delay slot; (3) 0x44/0x48/0x4C colour stores come after the
 *   -mno-split-addresses was tried (run 6/9): wrong code shape (la hoisting) and the same tool refusal; not retail
 *   Suggest: orchestrator relax ps2eeas_nops.move_sites for a macro store (s.s/sw sym) after a label, then p4.c ne
 */
#include "common.h"
typedef struct { char pad[0x30]; int st; unsigned int n; } ML2;
extern ML2 D_L18_0016D2E0_b __asm__("D_L18_0016D2E0");
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
extern float D_L18_001625E0 MACRO_ADDR;
extern int D_L18_001625E4 MACRO_ADDR;
extern float D_L18_001625E8;
extern float D_L18_001625EC MACRO_ADDR;
extern char D_L18_00167A50[];
extern char D_L18_001DFE50[];
extern float D_L18_001625B0 MACRO_ADDR;
extern short D_L18_00162528;
extern short D_L18_00162588;
extern short D_L18_0016258C;

typedef struct { float v[4]; } __attribute__((aligned(16))) QVabe0;

void func_L18_002FABE0(void) {
    QVabe0 m[4];
    int colors[4];
    float uv[4][2];
    unsigned long pkt[4];
    QVabe0 v0, v1, v2, v3;
    int i;
    float x;
    float y;
    if (D_L18_0016D2E0_b.n == func_001F9850(*(int *)&D_L18_00162528 + 1)) {
        D_L18_001625E0 = 1.0f;
        *(int *)&D_L18_001625E4 = 0;
        y = 0.15f;
        if (D_L18_0016D2E0_b.st == 7) y = -0.15f;
        D_L18_001625E8 = y;
        D_L18_001625EC = 1.0f;
    }
    x = 1.0f;
    pkt[1] = func_001F4868(11);
    pkt[2] = 0xFF9000000260;
    pkt[0] = 0;
    pkt[3] = 0x8000000048;
    func_00234C98(0x4A, 0);
    func_00234C98(0x47, 0x51001);
    func_001F9EC0(&v3, &D_L18_001625E0, D_L18_00167A50);
    func_001F9BD8(&v3, &v3, D_L18_00167A50 - 0x210);
    func_001F9BF0(&v0, &v3, D_L18_00167A50 - 0x210);
    func_L00_001FF4B0(&v0, &v0, x);
    {
    QVabe0 zv;
    *(long long *)&zv = 0;
    zv.v[2] = x;
    zv.v[3] = x;
    func_001F9CA0(&v1, &v0, &zv);
    }
    func_L00_001FF4B0(&v1, &v1, x);
    func_001F9CA0(&v2, &v1, &v0);
    colors[0] = colors[1] = colors[2] = colors[3] = *(int *)&D_L18_00162588;
    uv[0][0] = x; uv[0][1] = x; uv[1][0] = 0; uv[1][1] = x;
    uv[2][0] = x; uv[2][1] = 0; uv[3][0] = 0; uv[3][1] = 0;
    for (i = 0; i < 4; i++) {
        func_001F9C30(&m[i], D_L18_001DFE50 + i * 16, D_L18_001625B0);
        func_001F9EE8(&m[i], &m[i], &v0);
    }
    func_L00_001FD1D8(m, 0, 0);
    x = 0.75f;
    colors[0] = colors[1] = colors[2] = colors[3] = *(int *)&D_L18_0016258C;
    for (i = 0; i < 4; i++) {
        func_001F9C30(&m[i], D_L18_001DFE50 + i * 16, D_L18_001625B0 * x);
        func_001F9EE8(&m[i], &m[i], &v0);
    }
    func_L00_001FD1D8(m, 0, 0);
    x = 0.5f;
    for (i = 0; i < 4; i++) {
        func_001F9C30(&m[i], D_L18_001DFE50 + i * 16, D_L18_001625B0 * x);
        func_001F9EE8(&m[i], &m[i], &v0);
    }
    func_L00_001FD1D8(m, 0, 0);
}
